#include "../comut_utility.h"
#include "obng.h"

bool OBNG::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool OBNG::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool OBNG::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		SourceLocation start_loc = e->getLocStart();
    SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);
    StmtContext &stmt_context = context->getStmtContext();

    // OPPO can mutate binary bitwise expression in mutation range,
    // outside array decl size and enum declaration.
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
	    		 bo->isBitwiseOp() &&
	    		 !stmt_context.IsInArrayDeclSize() &&
					 !stmt_context.IsInEnumDecl();
	}

	return false;
}



void OBNG::Mutate(clang::Expr *e, ComutContext *context)
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



void OBNG::GenerateMutantByNegation(Expr *e, ComutContext *context)
{
  SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst); 

  Rewriter rewriter;
  rewriter.setSourceMgr(
  		context->comp_inst->getSourceManager(),
  		context->comp_inst->getLangOpts());
  string token{rewriter.ConvertToString(e)};    

  string mutated_token = "~(" + token + ")";

  GenerateMutantFile(context, start_loc, end_loc, mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																token, mutated_token);
}