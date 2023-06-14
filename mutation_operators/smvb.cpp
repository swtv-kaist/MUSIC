#include "../music_utility.h"
#include "smvb.h"

/* The domain for SMVB must be names of functions whose
   function calls will be mutated. */
bool SMVB::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool SMVB::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void SMVB::setRange(std::set<std::string> &range) {}

// Return True if the mutant operator can mutate this expression
bool SMVB::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  if (CompoundStmt *cs = dyn_cast<CompoundStmt>(s))
  {
    const Stmt* parent = GetParentOfStmt(s, context->comp_inst_);
    if (!parent)
      return false;

    if (isa<StmtExpr>(parent))
      return false;

    SourceManager &src_mgr = context->comp_inst_->getSourceManager();
    SourceLocation start_loc = cs->getLBracLoc();
    SourceLocation end_loc = cs->getRBracLoc().getLocWithOffset(1);

    // Do NOT apply SMVB to the then-body if there is an else-body
    if (isa<IfStmt>(parent))
    {
      auto if_stmt = cast<IfStmt>(parent);
      // cout << "if\n";
      // PrintLocation(src_mgr, if_stmt->getThen()->getBeginLoc());
      // PrintLocation(src_mgr, s->getBeginLoc());
      // cout << (!if_stmt->getElse()) << endl;

      if (if_stmt->getThen()->getBeginLoc() == s->getBeginLoc() &&
          if_stmt->getElse())
        return false;
    }

    // PrintLocation(src_mgr, start_loc);
    // PrintLocation(src_mgr, parent->getBeginLoc());
    // cout << parent->getStmtClassName() << endl;

    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
  }

  return false;
}

void SMVB::Mutate(clang::Stmt *s, MusicContext *context)
{
  CompoundStmt *cs;
  if (!(cs = dyn_cast<CompoundStmt>(s)))
    return;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = cs->getLBracLoc();
  SourceLocation end_loc = cs->getRBracLoc().getLocWithOffset(1);
  string token{ConvertToString(s, context->comp_inst_->getLangOpts())};

  // CASE 1: MOVE BRACE UP 1 STMT = EXCLUDE LAST STATEMENT
  auto last_stmt = cs->body_begin();
  auto it = cs->body_begin();
  string mutated_token1{"{\n"};
  const Stmt* parent = GetParentOfStmt(s, context->comp_inst_);

  if (it == cs->body_end())
    goto case2;

  it++;

  for (; it != cs->body_end(); it++)
  {
    Stmt *temp = *last_stmt;
    mutated_token1 += ConvertToString(temp, context->comp_inst_->getLangOpts());

    if (!(isa<CompoundStmt>(temp) || isa<ForStmt>(temp) || isa<IfStmt>(temp) ||
          isa<NullStmt>(temp) || isa<SwitchCase>(temp) || 
          isa<SwitchStmt>(temp) || isa<WhileStmt>(temp) || isa<LabelStmt>(temp))) 
      mutated_token1 += ";";

    mutated_token1 += "\n";    
    last_stmt++;
  }

  if (isa<CaseStmt>(*last_stmt) || isa<SwitchCase>(*last_stmt) || 
      isa<LabelStmt>(*last_stmt))
    goto case2;

  // PrintLocation(src_mgr, start_loc);
  // PrintLocation(src_mgr, (*last_stmt)->getBeginLoc());
  // cout << (*last_stmt)->getStmtClassName() << endl;

  mutated_token1 += "}\n";
  mutated_token1 += ConvertToString(*last_stmt, 
                                    context->comp_inst_->getLangOpts());
  if (!(isa<CompoundStmt>(*last_stmt) || isa<ForStmt>(*last_stmt) || 
        isa<IfStmt>(*last_stmt) || isa<WhileStmt>(*last_stmt) ||
        isa<NullStmt>(*last_stmt) || isa<SwitchCase>(*last_stmt) || 
        isa<SwitchStmt>(*last_stmt) || isa<LabelStmt>(*last_stmt)))
    mutated_token1 += ";";
  
  mutated_token1 += "\n";

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token1, 
      context->getStmtContext().getProteumStyleLineNum(), "UP");

  // CASE 2: MOVE BRACE DOWN 1 STMT = INCLUDE LAST STATEMENT
  case2:
  const Stmt *second_level_parent = GetSecondLevelParent(s, context->comp_inst_);
  if (!second_level_parent)
    return;

  if (!isa<CompoundStmt>(second_level_parent))
    return;

  auto compound_grandparent = cast<CompoundStmt>(second_level_parent);
  
  // find the location of the parent
  auto it2 = compound_grandparent->body_begin();
  auto next_stmt = compound_grandparent->body_begin();
  next_stmt++;

  for (; it2 != compound_grandparent->body_end(); it2++)
  {
    if (!((*it2)->getBeginLoc() != parent->getBeginLoc() ||
          (*it2)->getEndLoc() != parent->getEndLoc()))
      break;

    next_stmt++;
  }

  if (next_stmt == compound_grandparent->body_end())
    return;

  end_loc = GetLocationAfterSemicolon(
      src_mgr, 
      TryGetEndLocAfterBracketOrSemicolon((*next_stmt)->getEndLoc(), context->comp_inst_));

  string mutated_token2{ConvertToString(s, context->comp_inst_->getLangOpts())};
  size_t closing_brace_idx = mutated_token2.find_last_of("}");
  mutated_token2 = mutated_token2.substr(0, closing_brace_idx);
  mutated_token2 += ConvertToString((*next_stmt), 
                                    context->comp_inst_->getLangOpts());

  if (!(isa<CompoundStmt>(*next_stmt) || isa<ForStmt>(*next_stmt) || 
        isa<IfStmt>(*next_stmt) || isa<WhileStmt>(*next_stmt) ||
        isa<SwitchCase>(*next_stmt) || 
        isa<SwitchStmt>(*next_stmt) || isa<LabelStmt>(*next_stmt)))
    mutated_token2 += ";";
  
  mutated_token2 += "\n}\n";

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token2, 
      context->getStmtContext().getProteumStyleLineNum(), "DOWN");
}
