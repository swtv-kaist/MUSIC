#include "../music_utility.h"
#include "oaan.h"

extern set<string> arithemtic_operators;

bool OAAN::ValidateDomain(const std::set<std::string> &domain)
{
	for (auto it: domain)
  	if (arithemtic_operators.find(it) == arithemtic_operators.end())
    	// cannot find input domain inside valid domain
      return false;

  return true;
}

bool OAAN::ValidateRange(const std::set<std::string> &range)
{
	for (auto it: range)
  	if (arithemtic_operators.find(it) == arithemtic_operators.end())
    	// cannot find input range inside valid range
      return false;

  return true;
}

void OAAN::setDomain(std::set<std::string> &domain)
{
	if (domain.empty())
		domain_ = arithemtic_operators;
	else
		domain_ = domain;
}

void OAAN::setRange(std::set<std::string> &range)
{
	if (range.empty())
		range_ = arithemtic_operators;
	else
		range_ = range;
}

bool OAAN::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		string binary_operator{bo->getOpcodeStr()};
		SourceLocation start_loc = bo->getOperatorLoc();
		SourceManager &src_mgr = context->comp_inst_->getSourceManager();

		// cout << "cp oaan\n";
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

void OAAN::Mutate(clang::Expr *e, MusicContext *context)
{
	// cout << name_ << " is mutating\n";
	
	BinaryOperator *bo;
	if (!(bo = dyn_cast<BinaryOperator>(e))) return;

	string token{bo->getOpcodeStr()};
	SourceLocation start_loc = bo->getOperatorLoc();
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();

	// cout << "cp oaan 2\n";
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

		context->mutant_database_.AddMutantEntry(context->getStmtContext(),
				name_, start_loc, end_loc, token, mutated_token, 
				context->getStmtContext().getProteumStyleLineNum(), token+mutated_token);
	}
}

bool OAAN::IsMutationTarget(BinaryOperator *bo, string mutated_token,
										 MusicContext *context)
{
	Expr *lhs = bo->getLHS()->IgnoreImpCasts();
	Expr *rhs = bo->getRHS()->IgnoreImpCasts();
	string binary_operator{bo->getOpcodeStr()};

	if (bo->isMultiplicativeOp())
	{
		if (mutated_token.compare("%") == 0 &&
				(!ExprIsIntegral(context->comp_inst_, lhs) ||
				 !ExprIsIntegral(context->comp_inst_, rhs)))
			return false;

		return true;
	}

	// Mutating additive operator to multiplicative operator
	if (TranslateToOpcode(mutated_token) >= BO_Mul &&
	 		TranslateToOpcode(mutated_token) <= BO_Rem)
	{
		lhs = GetLeftOperandAfterMutationToMultiplicativeOp(lhs);
		rhs = GetRightOperandAfterMutationToMultiplicativeOp(rhs);

		if (ExprIsPointer(lhs) || ExprIsPointer(rhs) ||
				ExprIsArray(lhs) || ExprIsArray(rhs))
			return false;

		// cout << "lhs is: " << ConvertToString(lhs, context->comp_inst_->getLangOpts()) << endl;
		// cout << "lhs type: " << lhs->getType().getCanonicalType().getAsString() << endl;
		// cout << "lhs type: " << lhs->getType().getCanonicalType().getTypePtr()->isArrayType() << endl;

		if (mutated_token.compare("%") == 0 &&
				(!ExprIsIntegral(context->comp_inst_, lhs) ||
				 !ExprIsIntegral(context->comp_inst_, rhs)))
			return false;

		return true;
	}

	// Mutating additive operator to additive operator
	// If rhs is pointer, then only (int+ptr) and (ptr-ptr) is allowed
	if (ExprIsPointer(rhs) || ExprIsArray(rhs)) return false;

	// If lhs is pointer and rhs is not pointer, then only (ptr+-int) is allowed
	if ((ExprIsPointer(lhs) || ExprIsArray(lhs)) && 
			mutated_token.compare("+") != 0 && mutated_token.compare("-") != 0)
		return false;

	return true;
}