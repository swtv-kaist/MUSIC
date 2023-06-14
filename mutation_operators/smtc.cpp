#include "../music_utility.h"
#include "smtc.h"

#include <sstream>

#include "clang/Lex/PreprocessingRecord.h"

bool SMTC::ValidateDomain(const set<string> &domain)
{
  return true;
}

bool SMTC::ValidateRange(const set<string> &range)
{
  for (auto e: range)
    for (auto c: e)
      if (!isdigit(c))
        return false;

  return true;
}

void SMTC::setRange(std::set<std::string> &range)
{
  range_ = range;
  range_.insert("2");
}

bool SMTC::IsMutationTarget(Stmt *s, MusicContext *context)
{
  // Do NOT delete declaration statement.
  // Deleting null statement causes equivalent mutants.
  if (!(isa<ForStmt>(s) || isa<WhileStmt>(s) || isa<DoStmt>(s)))
    return false;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = GetLocationAfterSemicolon(
      src_mgr, 
      TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
}

void SMTC::Mutate(Stmt *s, MusicContext *context)
{
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  Stmt *body = nullptr;

  if (ForStmt *fs = dyn_cast<ForStmt>(s))
    body = fs->getBody();
  else if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
    body = ws->getBody();
  else if (DoStmt *ds = dyn_cast<DoStmt>(s))
    body = ds->getBody();
  else
    return;

  if (!body)
    return;

  SourceLocation start_loc = body->getBeginLoc();
  SourceLocation end_loc = GetLocationAfterSemicolon(
      src_mgr, 
      TryGetEndLocAfterBracketOrSemicolon(body->getEndLoc(), context->comp_inst_));

  if (CompoundStmt *c = dyn_cast<CompoundStmt>(body))
  {
    start_loc = c->getLBracLoc();
    end_loc = c->getRBracLoc().getLocWithOffset(1);
  }

  string token{ConvertToString(body, context->comp_inst_->getLangOpts())};
  SourceLocation loop_end_loc = end_loc;
  if (DoStmt *ds = dyn_cast<DoStmt>(s))
    loop_end_loc = ds->getRParenLoc().getLocWithOffset(1);

  vector<string> extra_tokens{"", ""};
  vector<string> extra_mutated_tokens{"{\nint MUSIC_var = 1;\n", ";}\n"};
  vector<SourceLocation> extra_start_locs{s->getBeginLoc(), loop_end_loc};
  vector<SourceLocation> extra_end_locs{s->getBeginLoc(), loop_end_loc};

  for (auto e: range_)
  {
    string mutated_token{"{\nif (MUSIC_var++ > " + e + ") continue;\n" + \
                         token + ";}\n"};

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum(), "",
      extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);
  }
}
