#include "../comut_utility.h"
#include "olng.h"

bool OLNG::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool OLNG::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool OLNG::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		SourceLocation start_loc = e->getLocStart();
    SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

    // OPPO can mutate binary logical expression in mutation range,
    // outside array decl size and enum declaration.
    return Range1IsPartOfRange2(
				SourceRange(start_loc, end_loc), 
				SourceRange(*(context->userinput->getStartOfMutationRange()),
										*(context->userinput->getEndOfMutationRange()))) &&
    		bo->isLogicalOp() &&
    		!context->is_inside_array_decl_size &&
    		!context->is_inside_enumdecl;
	}

	return false;
}

// Return True if the mutant operator can mutate this statement
bool OLNG::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void OLNG::Mutate(clang::Expr *e, ComutContext *context)
{
	BinaryOperator *bo;
	if (!(bo = dyn_cast<BinaryOperator>(e)))
		return;

	// Generate mutant by negating the whole expression
	GenerateMutantByNegation(e, context);

	// Generate mutant by negating right hand side of expr
	GenerateMutantByNegation(bo->getRHS()->IgnoreImpCasts(), context);

	// Generate mutant by negating left hand side of expr
	GenerateMutantByNegation(bo->getLHS()->IgnoreImpCasts(), context);
}

void OLNG::Mutate(clang::Stmt *s, ComutContext *context)
{}

void OLNG::GenerateMutantByNegation(Expr *e, ComutContext *context)
{
  SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst); 

  Rewriter rewriter;
  rewriter.setSourceMgr(
  		context->comp_inst->getSourceManager(),
  		context->comp_inst->getLangOpts());
  string token{rewriter.ConvertToString(e)};    

  string mutated_token = "!(" + token + ")";

  GenerateMutantFile(context, start_loc, end_loc, mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																token, mutated_token);
}