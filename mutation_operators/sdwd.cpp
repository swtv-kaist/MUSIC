#include "../music_utility.h"
#include "sdwd.h"

/* The domain for SDWD must be names of functions whose
   function calls will be mutated. */
bool SDWD::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool SDWD::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void SDWD::setRange(std::set<std::string> &range) {}

// Return True if the mutant operator can mutate this expression
bool SDWD::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  if (DoStmt *ds = dyn_cast<DoStmt>(s))
  {
    // if there is no condition, then this mutant is equivalent.
    if (ds->getCond() == nullptr)
      return false;

    SourceManager &src_mgr = context->comp_inst_->getSourceManager();
    SourceLocation start_loc = s->getBeginLoc();
    SourceLocation end_loc = GetLocationAfterSemicolon(
        src_mgr, 
        TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
  }

  return false;
}

void SDWD::Mutate(clang::Stmt *s, MusicContext *context)
{
  DoStmt *ds;
  if (!(ds = dyn_cast<DoStmt>(s)))
    return;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = GetLocationAfterSemicolon(
        src_mgr, 
        TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

  Stmt *body = ds->getBody();
  string body_string{ConvertToString(body, context->comp_inst_->getLangOpts())};

  Expr *cond = ds->getCond();
  string cond_string{ConvertToString(cond, context->comp_inst_->getLangOpts())};

  string token{ConvertToString(s, context->comp_inst_->getLangOpts())};
  string mutated_token{"while (" + cond_string + ") {\n" + body_string + ";}\n"};

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum());
}
