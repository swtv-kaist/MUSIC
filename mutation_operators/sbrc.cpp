#include "../music_utility.h"
#include "sbrc.h"

#include <sstream>

#include "clang/Lex/PreprocessingRecord.h"

bool SBRC::ValidateDomain(const set<string> &domain)
{
  return domain.empty();
}

bool SBRC::ValidateRange(const set<string> &range)
{
  return range.empty();
}

bool SBRC::IsMutationTarget(Stmt *s, MusicContext *context)
{
  if (!isa<BreakStmt>(s))
    return false;

  SourceLocation start_loc = s->getBeginLoc();
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation end_loc = src_mgr.translateLineCol(
        src_mgr.getMainFileID(), 
        GetLineNumber(src_mgr, start_loc),
        GetColumnNumber(src_mgr, start_loc) + 5);

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         context->getStmtContext().IsInLoopRange(start_loc);
}

void SBRC::Mutate(Stmt *s, MusicContext *context)
{
  SourceLocation start_loc = s->getBeginLoc();
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation end_loc = src_mgr.translateLineCol(
        src_mgr.getMainFileID(), 
        GetLineNumber(src_mgr, start_loc),
        GetColumnNumber(src_mgr, start_loc) + 5);

  string mutated_token{"continue"};
  string token{"break"};

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum());
}
