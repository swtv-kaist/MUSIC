#include "../music_utility.h"
#include "scrn.h"

/* The domain for SCRN must be names of functions whose
   function calls will be mutated. */
bool SCRN::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool SCRN::ValidateRange(const std::set<std::string> &range)
{
  for (auto e: range)
    for (auto c: e)
      if (!isdigit(c))
        return false;

  return true;
}

void SCRN::setRange(std::set<std::string> &range) 
{
  range_ = range;
  range_.insert("2");
}

// Return True if the mutant operator can mutate this expression
bool SCRN::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  if (!isa<ContinueStmt>(s))
    return false;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(), 
      GetLineNumber(src_mgr, start_loc),
      GetColumnNumber(src_mgr, start_loc) + 8);

  // BREAK STMT case: SCRN only mutates break stmt inside at least 2 for-loops
  if (!context->getStmtContext().IsInLoopRange(start_loc))
    return false;

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
}

void SCRN::Mutate(clang::Stmt *s, MusicContext *context)
{ 
  SourceManager &src_mgr = context->comp_inst_->getSourceManager(); 
  StmtContext &stmt_context = context->getStmtContext();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(), 
      GetLineNumber(src_mgr, start_loc),
      GetColumnNumber(src_mgr, start_loc) + 8);

  string token{ConvertToString(s, context->comp_inst_->getLangOpts())};
  string mutated_token{"goto MUSIC_SCRN"};

  // Find the enclosing loop
  for (auto e: range_)
  {
    int idx = stmt_context.loop_scope_list_->size();
    int count = 0;
    int target;

    if (!ConvertStringToInt(e, target))
    {
      cout << "SCRN Warning: " << e << " cannot be converted to an integer\n";
      continue;
    }

    // find the targeted enclosing loop
    // for ()     <---- 2nd enclosing loop
    //   for ()   <---- 1st enclosing loop
    //     break; <---- target
    while (idx != 0 && count != target)
    {
      idx--;
      if (LocationIsInRange(start_loc, stmt_context.loop_scope_list_->at(idx).second))
        count++;
      else
        break;
    }

    if (count != target)
      continue;

    Stmt *loop_stmt = stmt_context.loop_scope_list_->at(idx).first;
    Stmt *body = nullptr;

    if (ForStmt *fs = dyn_cast<ForStmt>(loop_stmt))
      body = fs->getBody();
    else if (WhileStmt *ws = dyn_cast<WhileStmt>(loop_stmt))
      body = ws->getBody();
    else if (DoStmt *ds = dyn_cast<DoStmt>(loop_stmt))
      body = ds->getBody();

    if (!body)
    {
      cout << "SCRN Warning: loop stmt at ";
      PrintLocation(src_mgr, loop_stmt->getBeginLoc());
      cout << " has no body\n";
      continue;
    }

    SourceLocation body_start_loc = body->getBeginLoc();
    SourceLocation body_end_loc = GetLocationAfterSemicolon(
        src_mgr, 
        TryGetEndLocAfterBracketOrSemicolon(body->getEndLoc(), context->comp_inst_));

    vector<string> extra_tokens{"", ""};
    vector<string> extra_mutated_tokens{"{\nif (0) MUSIC_SCRN: continue;\n", ";}\n"};
    vector<SourceLocation> extra_start_locs{body_start_loc, body_end_loc};
    vector<SourceLocation> extra_end_locs{body_start_loc, body_end_loc};

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum(), "",
        extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);
  }
}
