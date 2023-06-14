#include "../music_utility.h"
#include "srsr.h"

/* The domain for SRSR must be names of functions whose
   function calls will be mutated. */
bool SRSR::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool SRSR::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void SRSR::setRange(std::set<std::string> &range) {}

// Return True if the mutant operator can mutate this expression
bool SRSR::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  if (isa<DeclStmt>(s) || isa<NullStmt>(s))
    return false;

  const Stmt* parent = GetParentOfStmt(s, context->comp_inst_);

  if (!parent)
    return false;

  if (!isa<CompoundStmt>(parent))
    return false;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = GetLocationAfterSemicolon(
      src_mgr, 
      TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
}

void SRSR::Mutate(clang::Stmt *s, MusicContext *context)
{ 
  // cout << "SRSR:\n" << ConvertToString(s, context->comp_inst_->getLangOpts()) << endl;
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = GetLocationAfterSemicolon(
      src_mgr, 
      TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));
  // cout << "cp1\n";
  string token{ConvertToString(s, context->comp_inst_->getLangOpts())};

  // cout << (*(context->getSymbolTable()->getReturnStmtList())).size() << endl;

  for (auto stmt: (*(context->getSymbolTable()->getReturnStmtList()))[context->getFunctionId()])
  {
    // cout << "cp2\n";

    string mutated_token{ConvertToString(stmt, context->comp_inst_->getLangOpts())};
    if (mutated_token.compare(token) == 0)
      continue;

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());
  }    

  // cout << "SRSR: end\n";
}
