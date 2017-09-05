#include "../comut_utility.h"
#include "crcr.h"

bool CRCR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool CRCR::ValidateRange(const std::set<std::string> &range)
{
	for (auto num: range)
		if (!StringIsANumber(num))
			return false;

	return true;
}

void CRCR::setRange(std::set<std::string> &range)
{
	range_integral_ = {"0", "1", "-1"};
  range_float_ = {"0.0", "1.0", "-1.0"};

  for (auto num: range)
  	if (NumIsFloat(num))
  		range_float_.insert(num);
  	else
  		range_integral_.insert(num);
}


// Return True if the mutant operator can mutate this expression
bool CRCR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsScalarReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	StmtContext &stmt_context = context->getStmtContext();

	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInEnumDecl() &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInLhsOfAssignmentRange(e) &&
				 !stmt_context.IsInAddressOpRange(e) &&
				 !stmt_context.IsInUnaryIncrementDecrementRange(e);
}

void CRCR::Mutate(clang::Expr *e, ComutContext *context)
{
	Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(e)};
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	if (ExprIsIntegral(context->comp_inst, e))
	{
		for (auto num: range_integral_)
		{
			string mutated_token{"(" + num + ")"};

			GenerateMutantFile(context, start_loc, end_loc, mutated_token);
			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																		token, mutated_token);
		}

		return;
	}

	if (ExprIsFloat(e))
	{
		for (auto num: range_float_)
		{
			string mutated_token{"(" + num + ")"};

			GenerateMutantFile(context, start_loc, end_loc, mutated_token);
			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																		token, mutated_token);
		}

		return;
	}
}