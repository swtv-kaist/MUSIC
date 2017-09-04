#include "../comut_utility.h"
#include "ssdl.h"

bool SSDL::CanMutate(Expr *e, ComutContext *context)
{
  return false;
}

void SSDL::Mutate(clang::Expr *e, ComutContext *context) 
{}

bool SSDL::ValidateDomain(const set<string> &domain)
{
  if (domain.empty())
    return true;

  return false;
}

bool SSDL::ValidateRange(const set<string> &range)
{
  if (range.empty())
    return true;

  return false;
}

bool SSDL::CanMutate(Stmt *s, ComutContext *context)
{
	// If s is a CompoundStmt, then apply mutant to each stmt inside
	// I did not check if the compound stmt is in mutation range here because
	// SSDL is actually applied to each stmt inside not the whole CompoundStmt.
	// Hence it is better to check in SSDL::Mutate function.

	if (dyn_cast<CompoundStmt>(s))
		return true;

	return false;
}

void SSDL::Mutate(Stmt *s, ComutContext *context)
{
	CompoundStmt *c;
	if (!(c = dyn_cast<CompoundStmt>(s)))
		return;

  for (CompoundStmt::body_iterator it = c->body_begin(); 
       it != c->body_end(); ++it)
  {
    // Do not apply SSDL to the last statement of statement expression
    if (context->is_inside_stmtexpr)
    {
      CompoundStmt::body_iterator next_stmt = it;
      ++next_stmt;

      if (next_stmt == c->body_end())  
      {
        // this is the last statement of a statement expression
        context->is_inside_stmtexpr = false;

        // no SDL mutants generated for this statement
        continue;   
      }
    }

    DeleteStatement(*it, context);
  }
}

void SSDL::DeleteStatement(Stmt *s, ComutContext *context)
{
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
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(s)};
	SourceLocation start_loc = s->getLocStart();
	SourceLocation end_loc = GetLocationAfterSemicolon(
    src_mgr, GetEndLocOfStmt(s->getLocEnd(), context->comp_inst));

	// make replacing token
  string mutated_token{";"};
  mutated_token.append(
    GetLineNumber(src_mgr, end_loc) - GetLineNumber(src_mgr, start_loc),
	  '\n');

	if (end_loc.isInvalid())
	{
		cout << "DeleteStatement: cannot get end loc of stmt at";
		PrintLocation(src_mgr, start_loc);
	}
	else if (Range1IsPartOfRange2(
  		SourceRange(start_loc, end_loc), 
  		SourceRange(*(context->userinput->getStartOfMutationRange()),
								  *(context->userinput->getEndOfMutationRange()))) &&
					 NoUnremovableLabelInsideRange(src_mgr,
					 															 SourceRange(start_loc, end_loc),
					 															 context->label_to_gotolist_map))
	{
		GenerateMutantFile(context, start_loc, end_loc, mutated_token);

		WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, /*new_end_loc,*/ token, mutated_token);
	}
}

// an unremovable label is a label defined inside range stmtRange,
// but goto-ed outside of range stmtRange.
// Deleting such label can cause goto-undefined-label error.
// Return True if there is no such label inside given range
bool SSDL::NoUnremovableLabelInsideRange(
	SourceManager &src_mgr, SourceRange range, 
	LabelStmtToGotoStmtListMap *label_map)
{
  for (auto element: *label_map)
  {
    SourceLocation loc = src_mgr.translateLineCol(
    	src_mgr.getMainFileID(),
      element.first.first,
      element.first.second);

    // check only those labels inside range
    if (LocationIsInRange(loc, range))
    {
      // check if this label is goto-ed from outside of the statement
      // if yes, then the label is unremovable, return False
      for (auto e: element.second)  
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

bool SSDL::HandleStmtWithSubStmt(Stmt *s, ComutContext *context)
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

void SSDL::HandleStmtWithBody(Stmt *s, ComutContext *context)
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

void SSDL::DeleteCompoundStmtContent(CompoundStmt *c, ComutContext *context)
{
	// No point deleting a CompoundStmt full of NullStmt
  // If there is only 1 non-NullStmt, then this mutant is equivalent to deleting that single statement.
  if (CountNonNullStmtInCompoundStmt(c) <= 1)
    return;

  SourceLocation start_loc = c->getLBracLoc();
  SourceLocation end_loc = c->getRBracLoc().getLocWithOffset(1);

  Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

  if (!NoUnremovableLabelInsideRange(src_mgr, 
  																	 SourceRange(start_loc, end_loc),
  																	 context->label_to_gotolist_map))
    return;

  string token{rewriter.ConvertToString(c)};
  
  // make replacing token
  string mutated_token{"{"};
  mutated_token.append(
      GetLineNumber(src_mgr, end_loc) - GetLineNumber(src_mgr, start_loc), '\n');
  mutated_token.append(1, '}');


  if (Range1IsPartOfRange2(
		SourceRange(start_loc, end_loc), 
		SourceRange(*(context->userinput->getStartOfMutationRange()),
								*(context->userinput->getEndOfMutationRange()))))
  {
  	GenerateMutantFile(context, start_loc, end_loc, mutated_token);

		WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, /*new_end_loc,*/ 
																	token, mutated_token);
  }
}