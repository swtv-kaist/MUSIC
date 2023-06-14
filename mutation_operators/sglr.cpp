#include "../music_utility.h"
#include "sglr.h"

/* The domain for SGLR must be names of functions whose
   function calls will be mutated. */
bool SGLR::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool SGLR::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void SGLR::setRange(std::set<std::string> &range) {}

// Return True if the mutant operator can mutate this expression
bool SGLR::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  if (GotoStmt *gs = dyn_cast<GotoStmt>(s))
  {
    SourceManager &src_mgr = context->comp_inst_->getSourceManager();
    SourceLocation start_loc = s->getBeginLoc();
    string label_name{gs->getLabel()->getName()};
    SourceLocation end_loc = gs->getLabelLoc().getLocWithOffset(
        label_name.length());
    
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
  }

  return false;
}

void SGLR::Mutate(clang::Stmt *s, MusicContext *context)
{
  GotoStmt *gs;
  if (!(gs = dyn_cast<GotoStmt>(s)))
    return;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  string label_name{gs->getLabel()->getName()};
  SourceLocation end_loc = gs->getLabelLoc().getLocWithOffset(
      label_name.length());

  string token{ConvertToString(s, context->comp_inst_->getLangOpts())};

  for (auto label: (*(context->getSymbolTable()->getLabelList()))[context->getFunctionId()])
  {
    string mutated_label_name{label->getDecl()->getName()};
    if (label_name.compare(mutated_label_name) == 0)
      continue;

    string mutated_token{"goto " + mutated_label_name};

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());
  }    
}
