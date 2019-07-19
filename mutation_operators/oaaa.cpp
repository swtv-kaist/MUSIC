#include "../music_utility.h"
#include "oaaa.h"

extern set<string> arith_assignment_operators;

bool OAAA::ValidateDomain(const std::set<std::string> &domain)
{
	for (auto it: domain)
  	if (arith_assignment_operators.find(it) == arith_assignment_operators.end())
    	// cannot find input domain inside valid domain
      return false;

  return true;
}

bool OAAA::ValidateRange(const std::set<std::string> &range)
{
	for (auto it: range)
  	if (arith_assignment_operators.find(it) == arith_assignment_operators.end())
    	// cannot find input range inside valid range
      return false;

  return true;
}

void OAAA::setDomain(std::set<std::string> &domain)
{
	if (domain.empty())
		domain_ = arith_assignment_operators;
	else
		domain_ = domain;
}

void OAAA::setRange(std::set<std::string> &range)
{
	if (range.empty())
		range_ = arith_assignment_operators;
	else
		range_ = range;
}

bool OAAA::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		string binary_operator{bo->getOpcodeStr()};
		SourceLocation start_loc = bo->getOperatorLoc();
		SourceManager &src_mgr = context->comp_inst_->getSourceManager();

		// cout << "cp oaaa\n";
		SourceLocation end_loc = src_mgr.translateLineCol(
				src_mgr.getMainFileID(),
				GetLineNumber(src_mgr, start_loc),
				GetColumnNumber(src_mgr, start_loc) + binary_operator.length());
		StmtContext &stmt_context = context->getStmtContext();

		// if (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)))
		// {
		// 	cout << "OAAA checking: \n";
		// 	cout << ConvertToString(e, context->comp_inst_->getLangOpts()) << endl;
		// 	cout << stmt_context.IsInArrayDeclSize() << endl;
		// 	cout << stmt_context.IsInEnumDecl() << endl;
		// 	cout << (domain_.find(binary_operator) == domain_.end()) << endl;
		// 	cout << IsMutationTarget(bo) << endl;
		// }

		// Return False if expr is not in mutation range, inside array decl size
		// and inside enum declaration.
		if (!context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) ||
				stmt_context.IsInArrayDeclSize() ||
				stmt_context.IsInEnumDecl() ||
				domain_.find(binary_operator) == domain_.end())
			return false;

		// OAAA can only be applied under the following cases
		// 		- left and right side are both scalar
		// 		- only subtraction between pointers is allowed
		// 		- only ptr+int and ptr-int are allowed
		if (IsMutationTarget(bo))
			return true;
	}

	return false;
}

void OAAA::Mutate(clang::Expr *e, MusicContext *context)
{
	BinaryOperator *bo;
	if (!(bo = dyn_cast<BinaryOperator>(e)))
		return;

	string token{bo->getOpcodeStr()};
	SourceLocation start_loc = bo->getOperatorLoc();
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();

	// cout << "cp oaaa 2\n";
	SourceLocation end_loc = src_mgr.translateLineCol(
			src_mgr.getMainFileID(),
			GetLineNumber(src_mgr, start_loc),
			GetColumnNumber(src_mgr, start_loc) + token.length());

	for (auto mutated_token: range_)
	{
		// exact same token -> duplicate mutant
		if (token.compare(mutated_token) == 0)
			continue;

		if (only_plus_minus_ && mutated_token.compare("-=") != 0 &&
				mutated_token.compare("+=") != 0)
			continue;

		// modulo only takes integral operands
		if (mutated_token.compare("%=") == 0)
		{
			Expr *lhs = bo->getLHS()->IgnoreImpCasts();
			Expr *rhs = bo->getRHS()->IgnoreImpCasts();

			if (!ExprIsIntegral(context->comp_inst_, lhs) ||
					!ExprIsIntegral(context->comp_inst_, rhs))
					continue;
		}

		context->mutant_database_.AddMutantEntry(context->getStmtContext(),
				name_, start_loc, end_loc, token, mutated_token, 
				context->getStmtContext().getProteumStyleLineNum(), token+mutated_token);
	}

	only_plus_minus_ = false;
}

bool OAAA::IsMutationTarget(clang::BinaryOperator * const bo)
{
	Expr *lhs = bo->getLHS()->IgnoreImpCasts();
	Expr *rhs = bo->getRHS()->IgnoreImpCasts();

	if (ExprIsScalar(lhs) && ExprIsScalar(rhs))
		return true;

	// if both lhs and rhs are pointers then 
	// the operator have to be and can only be -=
	if (ExprIsPointer(lhs))
		if (ExprIsPointer(rhs))
			return false;
		else if (ExprIsScalar(rhs))
		{
			only_plus_minus_ = true;
			return true;
		}

	return false;
}