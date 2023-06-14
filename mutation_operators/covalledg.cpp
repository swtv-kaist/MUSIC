#include "../music_utility.h"
#include "covalledg.h"

bool CovAllEdg::ValidateDomain(const std::set<std::string> &domain)
{
  return domain.empty();
}

bool CovAllEdg::ValidateRange(const std::set<std::string> &range)
{
  return range.empty();
}

// Return True if the mutant operator can mutate this statement
bool CovAllEdg::IsMutationTarget(clang::Stmt *s, MusicContext *context)
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
  else if (isa<SwitchStmt>(s))
  {
    start_loc = s->getBeginLoc();
    end_loc = GetLocationAfterSemicolon(
        context->comp_inst_->getSourceManager(), 
        TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
  }
  else
    return false;

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) && 
         condition != nullptr;

  invalid_start_loc:
  cout << "CovAllEdg: cannot retrieve start loc for condition of ";
  ConvertToString(s, context->comp_inst_->getLangOpts());
  cout << endl;
  return false;
}

void CovAllEdg::Mutate(clang::Stmt *s, MusicContext *context)
{
  if (isa<SwitchStmt>(s))
    HandleSwitchStmt(s, context);
  else
    HandleConditionalStmt(s,context);
}

void CovAllEdg::HandleConditionalStmt(clang::Stmt *s, MusicContext *context)
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
      context->getStmtContext().getProteumStyleLineNum(), "",
      extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);

  string mutated_token2{"(" + token + ") ? 1 : kill(getpid(), 9)"};

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token2, 
      context->getStmtContext().getProteumStyleLineNum(), "",
      extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);
}

void CovAllEdg::HandleSwitchStmt(clang::Stmt *s, MusicContext *context)
{
  SwitchStmt *ss = nullptr;
  if (!(ss = dyn_cast<SwitchStmt>(s)))
    return;

  SwitchCase *sc = ss->getSwitchCaseList();
  if (sc == nullptr)
    return;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();

  SourceLocation start_of_file = src_mgr.getLocForStartOfFile(src_mgr.getMainFileID());
  vector<string> extra_tokens{""};
  vector<string> extra_mutated_tokens{"#include <sys/types.h>\n#include <signal.h>\n#include <unistd.h>\n"};
  vector<SourceLocation> extra_start_locs{start_of_file};
  vector<SourceLocation> extra_end_locs{start_of_file};

  while (sc != nullptr)
  {
    SourceLocation start_loc = sc->getBeginLoc();
    SourceLocation end_loc = sc->getColonLoc().getLocWithOffset(1);
    SourceLocation temp_loc = start_loc;
    string token{""};

    // Getting token this way because ConvertToString may include the sub-stmt
    while (temp_loc != end_loc)
    {
      token += (*(src_mgr.getCharacterData(temp_loc)));
      temp_loc = temp_loc.getLocWithOffset(1);
    }

    string mutated_token{token + " kill(getpid(), 9); MUSIC_SSWM:;\n"};

    SwitchCase *next_sc = sc->getNextSwitchCase();
    if (next_sc != nullptr)
      mutated_token = "goto MUSIC_SSWM;\n" + mutated_token;

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum(), "",
        extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);

    sc = sc->getNextSwitchCase();
  }
}
