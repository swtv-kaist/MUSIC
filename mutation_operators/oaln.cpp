#include "../music_utility.h"
#include "oaln.h"

extern set<string> arithemtic_operators;
extern set<string> logical_operators;

bool OALN::ValidateDomain(const std::set<std::string> &domain)
{
	for (auto it: domain)
  	if (arithemtic_operators.find(it) == arithemtic_operators.end())
    	// cannot find input domain inside valid domain
      return false;

  return true;
}

bool OALN::ValidateRange(const std::set<std::string> &range)
{
	for (auto it: range)
  	if (logical_operators.find(it) == logical_operators.end())
    	// cannot find input range inside valid range
      return false;

  return true;
}

void OALN::setDomain(std::set<std::string> &domain)
{
	if (domain.empty())
		domain_ = arithemtic_operators;
	else
		domain_ = domain;
}

void OALN::setRange(std::set<std::string> &range)
{
	if (range.empty())
		range_ = logical_operators;
	else
		range_ = range;
}

bool OALN::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	// cout << name_ << " is mutating\n";
	
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		string binary_operator{bo->getOpcodeStr()};
		SourceLocation start_loc = bo->getOperatorLoc();
		SourceManager &src_mgr = context->comp_inst_->getSourceManager();

		// cout << "cp oaln\n";
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

		if (!ExprIsPointer(e->IgnoreImpCasts()))
			return true;
	}

	return false;
}



void OALN::Mutate(clang::Expr *e, MusicContext *context)
{
	BinaryOperator *bo;
	if (!(bo = dyn_cast<BinaryOperator>(e)))
		return;

	string token{bo->getOpcodeStr()};
	SourceLocation start_loc = bo->getOperatorLoc();
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();

	// cout << "cp oaln\n";
	SourceLocation end_loc = src_mgr.translateLineCol(
			src_mgr.getMainFileID(),
			GetLineNumber(src_mgr, start_loc),
			GetColumnNumber(src_mgr, start_loc) + token.length());

	SourceLocation start_of_expr = e->getBeginLoc();
	SourceLocation end_of_expr = GetEndLocOfExpr(e, context->comp_inst_);
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());
	string lhs = ConvertToString(bo->getLHS(), context->comp_inst_->getLangOpts());
	string rhs = ConvertToString(bo->getRHS(), context->comp_inst_->getLangOpts());

	for (auto mutated_token: range_)
		if (token.compare(mutated_token) != 0)
		{
			string mutated_expr = "((" + lhs + ") " + mutated_token + " (" + rhs + "))";
			context->mutant_database_.AddMutantEntry(context->getStmtContext(),
					name_, start_loc, end_loc, token, mutated_token, 
					context->getStmtContext().getProteumStyleLineNum(), token+mutated_token);
		}
}

