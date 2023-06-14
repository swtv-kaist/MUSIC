#include "../music_utility.h"
#include "ssdl.h"

#include <sstream>

#include "clang/Lex/PreprocessingRecord.h"

bool SSDL::ValidateDomain(const set<string> &domain)
{
  for (auto e: domain)
    if (e.empty() || e.find_first_not_of("0123456789-") != string::npos ||
        e.find("-") == string::npos)
      return false;

  return true;
}

bool SSDL::ValidateRange(const set<string> &range)
{
  return range.empty();
}

bool SSDL::IsMutationTarget(Stmt *s, MusicContext *context)
{
  // Do NOT delete declaration statement.
  // Deleting null statement causes equivalent mutants.
  if (isa<DeclStmt>(s) || isa<NullStmt>(s))
    return false;

  // Only delete COMPLETE statements whose parent is a CompoundStmt.
  const Stmt* parent = GetParentOfStmt(s, context->comp_inst_);

  if (!parent)
    return false;

  if (!isa<CompoundStmt>(parent))
    return false;

  auto c = cast<CompoundStmt>(parent);

  const Stmt *second_level_parent = GetSecondLevelParent(s, context->comp_inst_);

  // Do NOT delete last stmt of a StmtExpr
  if (second_level_parent && isa<StmtExpr>(second_level_parent))
  {
    // find the a child stmt of parent that match 
    // the parameter stmt s's start and end locations
    auto it = c->body_begin();
    for (; it != c->body_end(); it++)
    {
      if (!((*it)->getBeginLoc() != s->getBeginLoc() ||
            (*it)->getEndLoc() != s->getEndLoc()))
        break;
    }

    // Return false if this is the last stmt
    ++it;
    if (it == c->body_end())
    {
      context->getStmtContext().setIsInStmtExpr(false);
      return false;
    }
  }

  return true;
}

void SSDL::Mutate(Stmt *s, MusicContext *context)
{
  // llvm::iterator_range<PreprocessingRecord::iterator> directive_list = \
  //     context->comp_inst_->getPreprocessor().getPreprocessingRecord()->getPreprocessedEntitiesInRange(
  //         SourceRange(s->getBeginLoc(), s->getEndLoc()));

  // cout << s->getBeginLoc().printToString(context->comp_inst_->getSourceManager()) << endl;

  // int count = 0;

  // for (PreprocessingRecord::iterator it = directive_list.begin(); it != directive_list.end(); ++it)
  // {
  //   cout << (*it)->getKind() << endl;
  //   cout << (*it)->getSourceRange().getBegin().printToString(context->comp_inst_->getSourceManager()) << endl;
  //   cout << (*it)->getSourceRange().getEnd().printToString(context->comp_inst_->getSourceManager()) << endl;
  //   count++;
  // }

  // cout << count << endl;

  DeleteStatement(s, context);
}

void SSDL::DeleteStatement(Stmt *s, MusicContext *context)
{
  // cout << "DeleteStatement called\n";

	// Do NOT delete declaration statement.
  // Deleting null statement causes equivalent mutants.
  if (isa<DeclStmt>(s) || isa<NullStmt>(s))
    return; 

  // Do not delete the label, case, default 
  // but only the statement right below them.
  if (HandleStmtWithSubStmt(s, context)) return;

  if (isa<IfStmt>(s) || isa<DoStmt>(s) || 
  		isa<WhileStmt>(s) || isa<ForStmt>(s))
  	HandleStmtWithBody(s, context);

  Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(s, context->comp_inst_->getLangOpts())};
	SourceLocation start_loc = s->getBeginLoc();
	SourceLocation end_loc = GetLocationAfterSemicolon(
      src_mgr, 
      TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

  // cout << "returned from GetLocationAfterSemicolon\n";

  if (end_loc.isInvalid())
  {
    cout << "DeleteStatement: cannot get end loc of stmt at ";
    PrintLocation(src_mgr, start_loc);
    cout << start_loc.printToString(src_mgr) << endl;
    cout << ConvertToString(s, context->comp_inst_->getLangOpts()) << endl;
    return;
  }

	// make replacing token without changing the line or column of other
  string mutated_token{";"};
  mutated_token.append(
    GetLineNumber(src_mgr, end_loc) - GetLineNumber(src_mgr, start_loc),
	  '\n');
	
	if (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
      IsInSpecifiedDomain(src_mgr, start_loc, end_loc) &&
      NoUnremovableLabelInsideRange(src_mgr,
      															SourceRange(start_loc, end_loc),
      															context->label_to_gotolist_map_))
	{
		context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());
	}

  // cout << "returning fron DeleteStatement\n";
}

// an unremovable label is a label defined inside range stmtRange,
// but goto-ed outside of range stmtRange.
// Deleting such label can cause goto-undefined-label error.
// Return True if there is no such label inside given range
bool SSDL::NoUnremovableLabelInsideRange(
	SourceManager &src_mgr, SourceRange range, 
	LabelStmtToGotoStmtListMap *label_map)
{
  auto it = label_map->begin();

  for (; it != label_map->end(); ++it)
  {
    // cout << "cp ssdl\n";

    SourceLocation loc = src_mgr.translateLineCol(
    	src_mgr.getMainFileID(),
      it->first.first,
      it->first.second);

    // check only those labels inside range
    if (LocationIsInRange(loc, range))
    {
      // check if this label is goto-ed from outside of the statement
      // if yes, then the label is unremovable, return False
      for (auto e: it->second)  
      {   
        // the goto is outside of the statement
        if (!LocationIsInRange(e, range))  
          return false;
      }
    }

    // the LabelStmtToGotoStmtListMap is traversed in the order of 
    // increasing location. If the location is after the range then 
    // all the rest is outside statement range
    if (LocationAfterRangeEnd(loc, range))
      return true;
  }

  return true;
}

bool SSDL::HandleStmtWithSubStmt(Stmt *s, MusicContext *context)
{
  if (SwitchCase *sc = dyn_cast<SwitchCase>(s))
  {
    DeleteStatement(sc->getSubStmt(), context); 
    return true;
  }
  else if (DefaultStmt *ds = dyn_cast<DefaultStmt>(s))
  {
    DeleteStatement(ds->getSubStmt(), context);
    return true;
  }
  else if (LabelStmt *ls = dyn_cast<LabelStmt>(s))
  {
    DeleteStatement(ls->getSubStmt(), context);
    return true;
  }

  return false;
}

void SSDL::HandleStmtWithBody(Stmt *s, MusicContext *context)
{
	Stmt *body = 0;

	if (ForStmt *fs = dyn_cast<ForStmt>(s))
		body = fs->getBody();
	else if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
		body = ws->getBody();
	else if (DoStmt *ds = dyn_cast<DoStmt>(s))
		body = ds->getBody();
	else if (IfStmt *is = dyn_cast<IfStmt>(s))
	{
		body = is->getThen();

		if (Stmt *else_stmt = is ->getElse())
			if (CompoundStmt *c = dyn_cast<CompoundStmt>(else_stmt))
				DeleteCompoundStmtContent(c, context);
			else
				DeleteStatement(else_stmt, context);
	}

	if (body)
		if (CompoundStmt *c = dyn_cast<CompoundStmt>(body))
			DeleteCompoundStmtContent(c, context);
		else
			DeleteStatement(body, context);
}

void SSDL::DeleteCompoundStmtContent(CompoundStmt *c, MusicContext *context)
{
	// No point deleting a CompoundStmt full of NullStmt
  // If there is only 1 non-NullStmt, then this mutant is equivalent to deleting that single statement.
  if (CountNonNullStmtInCompoundStmt(c) <= 1)
    return;

  SourceLocation start_loc = c->getLBracLoc();
  SourceLocation end_loc = c->getRBracLoc().getLocWithOffset(1);

  Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

  if (!NoUnremovableLabelInsideRange(src_mgr, 
  																	 SourceRange(start_loc, end_loc),
  																	 context->label_to_gotolist_map_))
    return;

  string token{ConvertToString(c, context->comp_inst_->getLangOpts())};
  
  // make replacing token
  string mutated_token{"{"};
  mutated_token.append(
      GetLineNumber(src_mgr, end_loc) - GetLineNumber(src_mgr, start_loc), '\n');
  mutated_token.append(1, '}');


  if (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
      IsInSpecifiedDomain(src_mgr, start_loc, end_loc))
  {
  	context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());
  }
}

bool SSDL::IsInSpecifiedDomain(SourceManager &src_mgr,
                              clang::SourceLocation start_loc,
                              clang::SourceLocation end_loc)
{
  if (domain_.empty())
    return true;

  int start_line = GetLineNumber(src_mgr, start_loc);
  int start_col = GetColumnNumber(src_mgr, start_loc);
  int end_line = GetLineNumber(src_mgr, end_loc);
  int end_col = GetColumnNumber(src_mgr, end_loc);

  for (auto e: domain_)
  {
    vector<string> range;
    SplitStringIntoVector(e, range, string("-"));

    int range_start;
    stringstream(range[0]) >> range_start;

    int range_end;
    stringstream(range[1]) >> range_end;

    if (start_line < range_start || start_line > range_end ||
        end_line < range_start || end_line > range_end ||
        (end_line == range_end && end_col > 1))
      continue;

    return true;
  }

  return false;
}
