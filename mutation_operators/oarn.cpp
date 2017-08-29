#include "../comut_utility.h"
#include "oarn.h"

bool OARN::ValidateDomain(const std::set<std::string> &domain)
{
	set<string> valid_domain{"+", "-", "*", "/", "%"};

	for (auto it: domain)
  	if (valid_domain.find(it) == valid_domain.end())
    	// cannot find input domain inside valid domain
      return false;

  return true;
}

bool OARN::ValidateRange(const std::set<std::string> &range)
{
	set<string> valid_range{">", "<", "<=", ">=", "==", "!="};

	for (auto it: range)
  	if (valid_range.find(it) == valid_range.end())
    	// cannot find input range inside valid range
      return false;

  return true;
}

void OARN::setDomain(std::set<std::string> &domain)
{
	if (domain.empty())
		domain_ = {"+", "-", "*", "/", "%"};
	else
		domain_ = domain;
}

void OARN::setRange(std::set<std::string> &range)
{
	if (range.empty())
		range_ = {">", "<", "<=", ">=", "==", "!="};
	else
		range_ = range;
}

bool OARN::CanMutate(clang::Expr *e, ComutContext *context)
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
		if (!Range1IsPartOfRange2(
				SourceRange(start_loc, end_loc), 
				SourceRange(*(context->userinput->getStartOfMutationRange()),
										*(context->userinput->getEndOfMutationRange()))) ||
				context->is_inside_array_decl_size ||
				context->is_inside_enumdecl ||
				LocationIsInRange(start_loc, *(context->typedef_range)) ||
				domain_.find(binary_operator) == domain_.end())
			return false;

		// If the expected result of the original expression is pointer then skip.
		// because mutating to relational operator causes the expr to return int
		if (!ExprIsPointer(e->IgnoreImpCasts()))
			return true;
	}

	return false;
}

bool OARN::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void OARN::Mutate(clang::Expr *e, ComutContext *context)
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

	SourceLocation start_of_expr = e->getLocStart();
	SourceLocation end_of_expr = GetEndLocOfExpr(e, context->comp_inst);
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());
	string lhs = rewriter.ConvertToString(bo->getLHS());
	string rhs = rewriter.ConvertToString(bo->getRHS());

	for (auto mutated_token: range_)
		if (token.compare(mutated_token) != 0)
		{
			string mutated_expr = "((" + lhs + ") " + mutated_token + " (" + rhs + "))";
			GenerateMutantFile(context, start_loc, end_loc, mutated_token);

			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																		token, mutated_token);
		}
}

void OARN::Mutate(clang::Stmt *s, ComutContext *context)
{}