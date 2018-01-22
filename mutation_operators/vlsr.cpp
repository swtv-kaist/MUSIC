#include "../comut_utility.h"
#include "vlsr.h"

bool VLSR::ValidateDomain(const std::set<std::string> &domain)
{
  for (auto e: domain)
    if (!IsValidVariableName(e))
      return false;

  return true;

	// return domain.empty();
}

bool VLSR::ValidateRange(const std::set<std::string> &range)
{
  for (auto e: range)
    if (!IsValidVariableName(e))
      return false;

  return true;

	// return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VLSR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsScalarReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
  StmtContext &stmt_context = context->getStmtContext();

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

	// VLSR can mutate this expression only if it is a scalar expression
	// inside mutation range and NOT inside array decl size or enum declaration
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         !stmt_context.IsInArrayDeclSize() &&
         !stmt_context.IsInEnumDecl() &&
         stmt_context.IsInCurrentlyParsedFunctionRange(e) &&
         is_in_domain;
}

void VLSR::Mutate(clang::Expr *e, ComutContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

  // cout << start_loc.printToString(context->comp_inst_->getSourceManager()) << endl;
  // cout << e->getLocEnd().printToString(context->comp_inst_->getSourceManager()) << endl;

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  // get all variable declaration that VLSR can mutate this expr to.
  VarDeclList range(
  		(*(context->getSymbolTable()->getLocalScalarVarDeclList()))[context->getFunctionId()]);

  GetRange(e, context, &range);

  for (auto vardecl: range)
  {
  	string mutated_token{GetVarDeclName(vardecl)};

  	context->mutant_database_.AddMutantEntry(
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());
  }
}

void VLSR::GetRange(Expr *e, ComutContext *context, VarDeclList *range)
{
  SourceLocation start_loc = e->getLocStart();
  Rewriter rewriter;
  rewriter.setSourceMgr(context->comp_inst_->getSourceManager(), 
                        context->comp_inst_->getLangOpts());

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  StmtContext &stmt_context = context->getStmtContext();

	// cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = stmt_context.IsInSwitchStmtConditionRange(e) ||
                            stmt_context.IsInArraySubscriptRange(e);

  // cannot mutate a variable in lhs of assignment to a const variable
  bool skip_const_vardecl = stmt_context.IsInLhsOfAssignmentRange(e);

  bool skip_register_vardecl = stmt_context.IsInAddressOpRange(e);

	// remove all VarDecl appearing after expr, 
  // const/float/register VarDecl (if necessary) and
  // VarDecl not inside range (if range is not empty)
	for (auto it = range->begin(); it != range->end(); )
	{
		if (!((*it)->getLocStart() < start_loc))
		{
			range->erase(it, range->end());
			break;
		}

    bool is_in_range = range_.empty() ? true :
                       IsStringElementOfSet(GetVarDeclName(*it), range_);

		if ((skip_const_vardecl && IsVarDeclConst((*it))) ||
				(skip_float_vardecl && IsVarDeclFloating((*it))) ||
				(skip_register_vardecl && 
          (*it)->getStorageClass() == SC_Register) ||
				(token.compare(GetVarDeclName(*it)) == 0) ||
        !is_in_range)
		{
			it = range->erase(it);
			continue;
		}

		++it;
	}

  // Remove all VarDecl not inside the same scope as expr E.
	for (auto scope: *(context->scope_list_))
	{
		// all vardecl after expr are removed.
		// No need to consider scopes after expr as well.
		if (LocationBeforeRangeStart(start_loc, scope))
			break;

		if (!LocationIsInRange(start_loc, scope))
			for (auto it = range->begin(); it != range->end();)
			{
        // We are only considering variable inside this scope.
        // The rest of the loop are VarDecl after scope.
				if (LocationAfterRangeEnd((*it)->getLocStart(), scope))
					break;

				if (LocationIsInRange((*it)->getLocStart(), scope))
				{
					it = range->erase(it);
					continue;
				}

				++it;
			}
	}


}