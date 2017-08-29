#include "../comut_utility.h"
#include "orln.h"

bool ORLN::ValidateDomain(const std::set<std::string> &domain)
{
	set<string> valid_domain{">", "<", "<=", ">=", "==", "!="};

	for (auto it: domain)
  	if (valid_domain.find(it) == valid_domain.end())
    	// cannot find input domain inside valid domain
      return false;

  return true;
}

bool ORLN::ValidateRange(const std::set<std::string> &range)
{
	set<string> valid_range{"&&", "||"};

	for (auto it: range)
  	if (valid_range.find(it) == valid_range.end())
    	// cannot find input range inside valid range
      return false;

  return true;
}

void ORLN::setDomain(std::set<std::string> &domain)
{
	if (domain.empty())
		domain_ = {">", "<", "<=", ">=", "==", "!="};
	else
		domain_ = domain;
}

void ORLN::setRange(std::set<std::string> &range)
{
	if (range.empty())
		range_ = {"&&", "||"};
	else
		range_ = range;
}

bool ORLN::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		string binary_operator{bo->getOpcodeStr()};
		SourceLocation start_loc = bo->getOperatorLoc();
		SourceManager &src_mgr = context->comp_inst->getSourceManager();
		SourceLocation end_loc = src_mgr.translateLineCol(
				src_mgr.getMainFileID(),
				GetLineNumber(src_mgr, start_loc),
				GetColumnNumber(src_mgr, start_loc) + binary_operator.length());

		// Return True if expr is in mutation range, NOT inside array decl size
		// and NOT inside enum declaration.
		if (Range1IsPartOfRange2(
				SourceRange(start_loc, end_loc), 
				SourceRange(*(context->userinput->getStartOfMutationRange()),
										*(context->userinput->getEndOfMutationRange()))) &&
				!context->is_inside_array_decl_size &&
				!context->is_inside_enumdecl &&
				domain_.find(binary_operator) != domain_.end())
			return true;
	}

	return false;
}

bool ORLN::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void ORLN::Mutate(clang::Expr *e, ComutContext *context)
{
	BinaryOperator *bo;
	if (!(bo = dyn_cast<BinaryOperator>(e)))
		return;

	string token{bo->getOpcodeStr()};
	SourceLocation start_loc = bo->getOperatorLoc();
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	SourceLocation end_loc = src_mgr.translateLineCol(
			src_mgr.getMainFileID(),
			GetLineNumber(src_mgr, start_loc),
			GetColumnNumber(src_mgr, start_loc) + token.length());

	for (auto mutated_token: range_)
		if (token.compare(mutated_token) != 0)
		{
			GenerateMutantFile(context, start_loc, end_loc, mutated_token);

			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																		token, mutated_token);
		}
}

void ORLN::Mutate(clang::Stmt *s, ComutContext *context)
{}