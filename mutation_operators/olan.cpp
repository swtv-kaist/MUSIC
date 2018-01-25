#include "../music_utility.h"
#include "olan.h"

bool OLAN::ValidateDomain(const std::set<std::string> &domain)
{
	set<string> valid_domain{"&&", "||"};

	for (auto it: domain)
  	if (valid_domain.find(it) == valid_domain.end())
    	// cannot find input domain inside valid domain
      return false;

  return true;
}

bool OLAN::ValidateRange(const std::set<std::string> &range)
{
	set<string> valid_range{"+", "-", "*", "/", "%"};

	for (auto it: range)
  	if (valid_range.find(it) == valid_range.end())
    	// cannot find input range inside valid range
      return false;

  return true;
}

void OLAN::setDomain(std::set<std::string> &domain)
{
	if (domain.empty())
		domain_ = {"&&", "||"};
	else
		domain_ = domain;
}

void OLAN::setRange(std::set<std::string> &range)
{
	if (range.empty())
		range_ = {"+", "-", "*", "/", "%"};
	else
		range_ = range;
}

bool OLAN::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		string binary_operator{bo->getOpcodeStr()};
		SourceLocation start_loc = bo->getOperatorLoc();
		SourceManager &src_mgr = context->comp_inst_->getSourceManager();

		// cout << "cp olan\n";
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

		return true;
	}

	return false;
}



void OLAN::Mutate(clang::Expr *e, MusicContext *context)
{
	BinaryOperator *bo;
	if (!(bo = dyn_cast<BinaryOperator>(e))) return;

	string token{bo->getOpcodeStr()};
	SourceLocation start_loc = bo->getOperatorLoc();
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	
	// cout << "cp olan\n";
	SourceLocation end_loc = src_mgr.translateLineCol(
			src_mgr.getMainFileID(),
			GetLineNumber(src_mgr, start_loc),
			GetColumnNumber(src_mgr, start_loc) + token.length());

	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	for (auto mutated_token: range_)
	{
		if (token.compare(mutated_token) == 0)
			continue;

		if (!IsMutationTarget(bo, mutated_token, context))
			continue;

		context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
	}
}



bool OLAN::IsMutationTarget(BinaryOperator *bo, string mutated_token,
										 MusicContext *context)
{
	Expr *lhs = GetLeftOperandAfterMutation(
			bo->getLHS()->IgnoreImpCasts(), TranslateToOpcode(mutated_token));
	Expr *rhs = GetRightOperandAfterMutation(
			bo->getRHS()->IgnoreImpCasts(), TranslateToOpcode(mutated_token));

	// multiplication and division takes integral or floating operands
	if (mutated_token.compare("/") == 0 || mutated_token.compare("*") == 0)
		return ExprIsScalar(lhs) && ExprIsScalar(rhs);

	// modulo only takes integral operands
	if (mutated_token.compare("%") == 0)
		return ExprIsIntegral(context->comp_inst_, lhs) && 
			 		 ExprIsIntegral(context->comp_inst_, rhs);

	// mutated_token is additive (+ or -)
	// for cases that one of operand is pointer, only the followings are allowed
	// 		(int + ptr), (ptr - ptr), (ptr + int), (ptr - int)
	// Also, only ptr of same type can subtract each other
	if (ExprIsPointer(lhs))
	{
		string lhs_type{getPointerType(lhs->getType())};
		if (ExprIsPointer(rhs) &&
				lhs_type.compare(getPointerType(rhs->getType())) == 0)
			return (mutated_token.compare("-") == 0);

		if (ExprIsIntegral(context->comp_inst_, rhs))
			return true;

		// rhs is neither pointer nor integral -> not mutatable
		return false;
	}

	if (ExprIsPointer(rhs))
	{
		if (ExprIsIntegral(context->comp_inst_, lhs))
			return (mutated_token.compare("+") == 0);

		return false;
	}

	return true;
}