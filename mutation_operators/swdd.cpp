#include "../music_utility.h"
#include "swdd.h"

/* The domain for SWDD must be names of functions whose
   function calls will be mutated. */
bool SWDD::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool SWDD::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void SWDD::setRange(std::set<std::string> &range) {}

// Return True if the mutant operator can mutate this expression
bool SWDD::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
  {
    // if there is no condition, then this mutant is equivalent.
    if (ws->getCond() == nullptr)
      return false;

    SourceManager &src_mgr = context->comp_inst_->getSourceManager();
    SourceLocation start_loc = s->getBeginLoc();
    SourceLocation end_loc = GetLocationAfterSemicolon(
        src_mgr, 
        TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

    Stmt *body = ws->getBody();
    if (body)
      if (CompoundStmt *c = dyn_cast<CompoundStmt>(body))
        end_loc = c->getRBracLoc().getLocWithOffset(1);

    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
  }

  return false;
}

void SWDD::Mutate(clang::Stmt *s, MusicContext *context)
{
  WhileStmt *ws;
  if (!(ws = dyn_cast<WhileStmt>(s)))
    return;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = GetLocationAfterSemicolon(
        src_mgr, 
        TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

  Stmt *body = ws->getBody();
  string body_string{ConvertToString(body, context->comp_inst_->getLangOpts())};
  if (body)
  {
    if (CompoundStmt *c = dyn_cast<CompoundStmt>(body))
      end_loc = c->getRBracLoc().getLocWithOffset(1);
  }
  else
    body_string = ";";

  Expr *cond = ws->getCond();
  string cond_string{ConvertToString(cond, context->comp_inst_->getLangOpts())};

  string token{ConvertToString(s, context->comp_inst_->getLangOpts())};
  string mutated_token{"do {\n" + body_string + ";\n} while (" + cond_string + ");"};

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum());
}
