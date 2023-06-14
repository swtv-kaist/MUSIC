#include "../music_utility.h"
#include "stri.h"

bool STRI::ValidateDomain(const std::set<std::string> &domain)
{
  return domain.empty();
}

bool STRI::ValidateRange(const std::set<std::string> &range)
{
  return range.empty();
}

// Return True if the mutant operator can mutate this statement
bool STRI::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  SourceLocation start_loc, end_loc;
  Expr *condition{nullptr};

  if (IfStmt *is = dyn_cast<IfStmt>(s))
  {
    // empty condition
    if (is->getCond() == nullptr)
      return false;

    condition = is->getCond()->IgnoreImpCasts();
    start_loc = condition->getBeginLoc();

    if (start_loc.isInvalid())
      goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
  }
  else if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
  {
    // empty condition
    if (ws->getCond() == nullptr)
      return false;

    condition = ws->getCond()->IgnoreImpCasts();
    start_loc = condition->getBeginLoc();

    if (start_loc.isInvalid())
      goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
  }
  else if (DoStmt *ds = dyn_cast<DoStmt>(s))
  {
    // empty condition
    if (ds->getCond() == nullptr)
      return false;

    condition = ds->getCond()->IgnoreImpCasts();
    start_loc = condition->getBeginLoc();

    if (start_loc.isInvalid())
      goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
  }
  else if (ForStmt *fs = dyn_cast<ForStmt>(s))
  {
    // empty condition
    if (fs->getCond() == nullptr)
      return false;

    condition = fs->getCond()->IgnoreImpCasts();
    start_loc = condition->getBeginLoc();

    if (start_loc.isInvalid())
      goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
  }
  else if (
      AbstractConditionalOperator *aco = dyn_cast<AbstractConditionalOperator>(s))
  {
    // empty condition
    if (aco->getCond() == nullptr)
      return false;

    condition = aco->getCond()->IgnoreImpCasts();
    start_loc = condition->getBeginLoc();

    if (start_loc.isInvalid())
      goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
  }
  else
    return false;

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) && 
         condition != nullptr;

  invalid_start_loc:
  cout << "STRI: cannot retrieve start loc for condition of ";
  ConvertToString(s, context->comp_inst_->getLangOpts());
  cout << endl;
  return false;
}

void STRI::Mutate(clang::Stmt *s, MusicContext *context)
{
  Expr *condition{nullptr};

  if (IfStmt *is = dyn_cast<IfStmt>(s))
    condition = is->getCond()->IgnoreImpCasts();
  else if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
    condition = ws->getCond()->IgnoreImpCasts();
  else if (DoStmt *ds = dyn_cast<DoStmt>(s))
    condition = ds->getCond()->IgnoreImpCasts();
  else if (ForStmt *fs = dyn_cast<ForStmt>(s))
    condition = fs->getCond()->IgnoreImpCasts();
  else if (
      AbstractConditionalOperator *aco = dyn_cast<AbstractConditionalOperator>(s))
    condition = aco->getCond()->IgnoreImpCasts();
  else
    return;

  if (condition == nullptr)
    return;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = condition->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
  
  string token{ConvertToString(condition, context->comp_inst_->getLangOpts())};
  string mutated_token1{"(" + token + ") ? kill(getpid(), 9) : 0"};

  SourceLocation start_of_file = src_mgr.getLocForStartOfFile(src_mgr.getMainFileID());
  vector<string> extra_tokens{""};
  vector<string> extra_mutated_tokens{"#include <sys/types.h>\n#include <signal.h>\n#include <unistd.h>\n"};
  vector<SourceLocation> extra_start_locs{start_of_file};
  vector<SourceLocation> extra_end_locs{start_of_file};

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token1, 
      context->getStmtContext().getProteumStyleLineNum(), "TRUE",
      extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);

  string mutated_token2{"(" + token + ") ? 1 : kill(getpid(), 9)"};

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token2, 
      context->getStmtContext().getProteumStyleLineNum(), "FALSE",
      extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);
}
