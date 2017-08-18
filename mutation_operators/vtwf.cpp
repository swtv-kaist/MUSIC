#include "../comut_utility.h"
#include "vtwf.h"

bool VTWF::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VTWF::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}


// Return True if the mutant operator can mutate this expression
bool VTWF::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (CallExpr *ce = dyn_cast<CallExpr>(e))
	{
		SourceLocation start_loc = ce->getLocStart();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is scalar type.
		return (Range1IsPartOfRange2(
				SourceRange(start_loc, end_loc), 
				SourceRange(*(context->userinput->getStartOfMutationRange()),
										*(context->userinput->getEndOfMutationRange()))) &&
						!context->is_inside_enumdecl &&
						ExprIsScalar(e));
	}

	return false;
}


// Return True if the mutant operator can mutate this statement
bool VTWF::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}


void VTWF::Mutate(clang::Expr *e, ComutContext *context)
{
	CallExpr *ce;
	if (!(ce = dyn_cast<CallExpr>(e)))
		return;

	SourceLocation start_loc = ce->getLocStart();

  // getRParenLoc returns the location before the right parenthesis
  SourceLocation end_loc = ce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(e)};
	string mutated_token = "(" + token + "+1)";

	GenerateMutantFile(context, start_loc, end_loc, mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, token, 
																mutated_token);

	mutated_token = "(" + token + "-1)";
	GenerateMutantFile(context, start_loc, end_loc, mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, token, 
																mutated_token);
}

void VTWF::Mutate(clang::Stmt *s, ComutContext *context)
{}