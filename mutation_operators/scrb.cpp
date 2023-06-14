#include "../music_utility.h"
#include "scrb.h"

#include <sstream>

#include "clang/Lex/PreprocessingRecord.h"

bool SCRB::ValidateDomain(const set<string> &domain)
{
  return domain.empty();
}

bool SCRB::ValidateRange(const set<string> &range)
{
  return range.empty();
}

bool SCRB::IsMutationTarget(Stmt *s, MusicContext *context)
{
  if (!isa<ContinueStmt>(s))
    return false;

  SourceLocation start_loc = s->getBeginLoc();
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation end_loc = src_mgr.translateLineCol(
        src_mgr.getMainFileID(), 
        GetLineNumber(src_mgr, start_loc),
        GetColumnNumber(src_mgr, start_loc) + 8);

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
}

void SCRB::Mutate(Stmt *s, MusicContext *context)
{
  SourceLocation start_loc = s->getBeginLoc();
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation end_loc = src_mgr.translateLineCol(
        src_mgr.getMainFileID(), 
        GetLineNumber(src_mgr, start_loc),
        GetColumnNumber(src_mgr, start_loc) + 8);

  string mutated_token{"break"};
  string token{"continue"};

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum());
}
