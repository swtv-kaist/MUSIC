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
	// cout << name_ << " is mutating\n";
	
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		string binary_operator{bo->getOpcodeStr()};
		SourceLocation start_loc = bo->getOperatorLoc();
		SourceManager &src_mgr = context->comp_inst_->getSourceManager();

		// cout << "cp oarn\n";
		SourceLocation end_loc = src_mgr.translateLineCol(
				src_mgr.getMainFileID(),
				GetLineNumber(src_mgr, start_loc),
				GetColumnNumber(src_mgr, start_loc) + binary_operator.length());
		StmtContext &stmt_context = context->getStmtContext();

		// Return True if expr is in mutation range, NOT inside array decl size
		// and NOT inside enum declaration.
		if (!context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) ||
				stmt_context.IsInArrayDeclSize() ||
				stmt_context.IsInEnumDecl() ||
				stmt_context.IsInTypedefRange(e) ||
				domain_.find(binary_operator) == domain_.end())
			return false;

		// If the expected result of the original expression is pointer then skip.
		// because mutating to relational operator causes the expr to return int
		if (!ExprIsPointer(e->IgnoreImpCasts()))
			return true;
	}

	return false;
}



void OARN::Mutate(clang::Expr *e, ComutContext *context)
{
	BinaryOperator *bo;
	if (!(bo = dyn_cast<BinaryOperator>(e)))
		return;

	string token{bo->getOpcodeStr()};
	SourceLocation start_loc = bo->getOperatorLoc();
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();

	// cout << "cp oarn 2\n";
	SourceLocation end_loc = src_mgr.translateLineCol(
			src_mgr.getMainFileID(),
			GetLineNumber(src_mgr, start_loc),
			GetColumnNumber(src_mgr, start_loc) + token.length());

	SourceLocation start_of_expr = e->getLocStart();
	SourceLocation end_of_expr = GetEndLocOfExpr(e, context->comp_inst_);
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());
	string lhs = ConvertToString(bo->getLHS(), context->comp_inst_->getLangOpts());
	string rhs = ConvertToString(bo->getRHS(), context->comp_inst_->getLangOpts());

	for (auto mutated_token: range_)
		if (token.compare(mutated_token) != 0)
		{
			string mutated_expr = "((" + lhs + ") " + mutated_token + " (" + rhs + "))";
			context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
		}
}

