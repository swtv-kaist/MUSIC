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
    SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
    StmtContext &stmt_context = context->getStmtContext();

    // OPPO can mutate binary logical expression in mutation range,
    // outside array decl size and enum declaration.
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
    		bo->isLogicalOp() &&
    		!stmt_context.IsInArrayDeclSize() &&
				!stmt_context.IsInEnumDecl();
	}

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



void OLNG::GenerateMutantByNegation(Expr *e, ComutContext *context)
{
  SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_); 

  Rewriter rewriter;
  rewriter.setSourceMgr(
  		context->comp_inst_->getSourceManager(),
  		context->comp_inst_->getLangOpts());
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};    

  string mutated_token = "!(" + token + ")";

  context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
}