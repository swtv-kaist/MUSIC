#include "../comut_utility.h"
#include "vlpf.h"

bool VLPF::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VLPF::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VLPF::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (CallExpr *ce = dyn_cast<CallExpr>(e))
	{
		SourceLocation start_loc = ce->getLocStart();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is pointer type.
		return (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
            !context->getStmtContext().IsInEnumDecl() &&
						ExprIsPointer(e));
	}

	return false;
}



void VLPF::Mutate(clang::Expr *e, ComutContext *context)
{
	CallExpr *ce;
	if (!(ce = dyn_cast<CallExpr>(e)))
		return;

	SourceLocation start_loc = ce->getLocStart();

  // getRParenLoc returns the location before the right parenthesis
  SourceLocation end_loc = ce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{rewriter.ConvertToString(e)};

	// get all variable declaration that VLSR can mutate this expr to.
  VarDeclList range(
      (*(context->getSymbolTable()->getLocalPointerVarDeclList()))[context->getFunctionId()]);

  GetRange(e, context, &range);

  // type of the entity pointed to by pointer variable
  string pointee_type{
  		getPointerType(e->getType().getCanonicalType())};

  for (auto vardecl: range)
  {
    string mutated_token{GetVarDeclName(vardecl)};

    if (pointee_type.compare(getPointerType(vardecl->getType())) == 0)
    {
      context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
    }
  }
}



void VLPF::GetRange(Expr *e, ComutContext *context, VarDeclList *range)
{
  SourceLocation start_loc = e->getLocStart();
  Rewriter rewriter;
  rewriter.setSourceMgr(context->comp_inst_->getSourceManager(), 
                        context->comp_inst_->getLangOpts());

  string token{rewriter.ConvertToString(e)};
  
  // cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = \
      context->getStmtContext().IsInSwitchStmtConditionRange(e);

  // remove all vardecl appear after expr
  for (auto it = range->begin(); it != range->end(); )
  {
    if (!((*it)->getLocStart() < start_loc))
    {
      range->erase(it, range->end());
      break;
    }

    if (skip_float_vardecl && IsVarDeclFloating(*it))
    {
      it = range->erase(it);
      continue;
    }

    ++it;
  }

  for (auto scope: *(context->scope_list_))
  {
    // all vardecl after expr are removed.
    // Hence no need to consider scopes after expr as well.
    if (LocationBeforeRangeStart(start_loc, scope))
      break;

    if (!LocationIsInRange(start_loc, scope))
      for (auto it = range->begin(); it != range->end();)
      {
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