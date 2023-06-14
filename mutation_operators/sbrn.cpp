#include "../music_utility.h"
#include "sbrn.h"

/* The domain for SBRN must be names of functions whose
   function calls will be mutated. */
bool SBRN::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool SBRN::ValidateRange(const std::set<std::string> &range)
{
  for (auto e: range)
    for (auto c: e)
      if (!isdigit(c))
        return false;

  return true;
}

void SBRN::setRange(std::set<std::string> &range) 
{
  range_ = range;
  range_.insert("2");
}

// Return True if the mutant operator can mutate this expression
bool SBRN::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  if (!isa<BreakStmt>(s))
    return false;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(), 
      GetLineNumber(src_mgr, start_loc),
      GetColumnNumber(src_mgr, start_loc) + 5);

  // BREAK STMT case: SBRN only mutates break stmt inside at least 2 for-loops
  if (!context->getStmtContext().IsInLoopRange(start_loc))
    return false;

  // remove switch statements that are already passed
  while (!context->switchstmt_info_list_->empty() && 
         !LocationIsInRange(start_loc, context->switchstmt_info_list_->back().first))
    context->switchstmt_info_list_->pop_back();

  // the current statement is not inside a switch statement
  if (context->switchstmt_info_list_->empty())
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));

  // Make sure the statement is immediately enclosed by a loop, not a switch
  if (Range1IsPartOfRange2(context->switchstmt_info_list_->back().first,
                           context->getStmtContext().loop_scope_list_->back().second))
    return false;

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
}

void SBRN::Mutate(clang::Stmt *s, MusicContext *context)
{ 
  SourceManager &src_mgr = context->comp_inst_->getSourceManager(); 
  StmtContext &stmt_context = context->getStmtContext();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(), 
      GetLineNumber(src_mgr, start_loc),
      GetColumnNumber(src_mgr, start_loc) + 5);

  string token{ConvertToString(s, context->comp_inst_->getLangOpts())};
  string mutated_token{"goto MUSIC_SBRn"};

  // Find the enclosing loop
  for (auto e: range_)
  {
    int idx = stmt_context.loop_scope_list_->size();
    int count = 0;
    int target;

    if (!ConvertStringToInt(e, target))
    {
      cout << "SBRn Warning: " << e << " cannot be converted to an integer\n";
      continue;
    }

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
      cout << "SBRn Warning: loop stmt at ";
      PrintLocation(src_mgr, loop_stmt->getBeginLoc());
      cout << " has no body\n";
      continue;
    }

    SourceLocation body_start_loc = body->getBeginLoc();
    SourceLocation body_end_loc = GetLocationAfterSemicolon(
        src_mgr, 
        TryGetEndLocAfterBracketOrSemicolon(body->getEndLoc(), context->comp_inst_));

    vector<string> extra_tokens{"", ""};
    vector<string> extra_mutated_tokens{"{\nif (0) MUSIC_SBRn: break;\n", ";}\n"};
    vector<SourceLocation> extra_start_locs{body_start_loc, body_end_loc};
    vector<SourceLocation> extra_end_locs{body_start_loc, body_end_loc};

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum(), "",
        extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);
  }
}
