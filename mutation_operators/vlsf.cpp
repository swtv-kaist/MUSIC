#include "../music_utility.h"
#include "vlsf.h"

bool VLSF::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VLSF::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VLSF::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (CallExpr *ce = dyn_cast<CallExpr>(e))
	{
		SourceLocation start_loc = ce->getLocStart();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is scalar type.
		return (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
            !context->getStmtContext().IsInEnumDecl() &&
						ExprIsScalar(e));
	}

	return false;
}



void VLSF::Mutate(clang::Expr *e, MusicContext *context)
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

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

	// get all variable declaration that VLSR can mutate this expr to.
  VarDeclList range(
  		(*(context->getSymbolTable()->getLocalScalarVarDeclList()))[context->getFunctionId()]);

  GetRange(e, context, &range);

  for (auto vardecl: range)
  {
  	string mutated_token{GetVarDeclName(vardecl)};

  	context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
  }
}



void VLSF::GetRange(Expr *e, MusicContext *context, VarDeclList *range)
{
  SourceLocation start_loc = e->getLocStart();
  Rewriter rewriter;
  rewriter.setSourceMgr(context->comp_inst_->getSourceManager(), 
                        context->comp_inst_->getLangOpts());

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

	// cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = \
      context->getStmtContext().IsInSwitchStmtConditionRange(e) ||
      context->getStmtContext().IsInNonFloatingExprRange(e);

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