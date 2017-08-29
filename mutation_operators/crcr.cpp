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

	return Range1IsPartOfRange2(
			SourceRange(start_loc, end_loc), 
			SourceRange(*(context->userinput->getStartOfMutationRange()),
									*(context->userinput->getEndOfMutationRange()))) &&
				 !context->is_inside_enumdecl &&
				 !context->is_inside_array_decl_size &&
				 !LocationIsInRange(start_loc, *(context->lhs_of_assignment_range)) &&
				 !LocationIsInRange(start_loc, *(context->addressop_range)) &&
				 !LocationIsInRange(start_loc, *(context->unary_inc_dec_range));
}

// Return True if the mutant operator can mutate this statement
bool CRCR::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
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

void CRCR::Mutate(clang::Stmt *s, ComutContext *context)
{}
