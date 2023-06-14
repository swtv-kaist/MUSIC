#include "../music_utility.h"
#include "retstadel.h"

#include <sstream>

#include "clang/Lex/PreprocessingRecord.h"

bool RetStaDel::ValidateDomain(const set<string> &domain)
{
  return domain.empty();
}

bool RetStaDel::ValidateRange(const set<string> &range)
{
  return range.empty();
}

bool RetStaDel::IsMutationTarget(Stmt *s, MusicContext *context)
{
  if (!isa<ReturnStmt>(s))
    return false;

  SourceLocation start_loc = s->getBeginLoc();
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation end_loc = s->getEndLoc();

  ReturnStmt *rs = dyn_cast<ReturnStmt>(s);
  if (Expr *ret_expr = rs->getRetValue()) 
  {
    end_loc = GetEndLocOfExpr(ret_expr, context->comp_inst_);
  }
  else
    end_loc = src_mgr.translateLineCol(
        src_mgr.getMainFileID(), 
        GetLineNumber(src_mgr, start_loc),
        GetColumnNumber(src_mgr, start_loc) + 6);

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
}

void RetStaDel::Mutate(Stmt *s, MusicContext *context)
{
  SourceLocation start_loc = s->getBeginLoc();
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation end_loc = s->getEndLoc();

  ReturnStmt *rs = dyn_cast<ReturnStmt>(s);
  if (Expr *ret_expr = rs->getRetValue()) 
  {
    end_loc = GetEndLocOfExpr(ret_expr, context->comp_inst_);
  }
  else
    end_loc = src_mgr.translateLineCol(
        src_mgr.getMainFileID(), 
        GetLineNumber(src_mgr, start_loc),
        GetColumnNumber(src_mgr, start_loc) + 6);

  string token{ConvertToString(s, context->comp_inst_->getLangOpts())};
  string mutated_token{""};
  
  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum());
}
