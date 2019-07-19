#include "../music_utility.h"
#include "obrn.h"

extern set<string> bitwise_operators;
extern set<string> relational_operators;

bool OBRN::ValidateDomain(const std::set<std::string> &domain)
{
	for (auto it: domain)
  	if (bitwise_operators.find(it) == bitwise_operators.end())
    	// cannot find input domain inside valid domain
      return false;

  return true;
}

bool OBRN::ValidateRange(const std::set<std::string> &range)
{
	for (auto it: range)
  	if (relational_operators.find(it) == relational_operators.end())
    	// cannot find input range inside valid range
      return false;

  return true;
}

void OBRN::setDomain(std::set<std::string> &domain)
{
	if (domain.empty())
		domain_ = bitwise_operators;
	else
		domain_ = domain;
}

void OBRN::setRange(std::set<std::string> &range)
{
	if (range.empty())
		range_ = relational_operators;
	else
		range_ = range;
}

bool OBRN::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		string binary_operator{bo->getOpcodeStr()};
		SourceLocation start_loc = bo->getOperatorLoc();
		SourceManager &src_mgr = context->comp_inst_->getSourceManager();

		// cout << "cp obrn\n";
		SourceLocation end_loc = src_mgr.translateLineCol(
				src_mgr.getMainFileID(),
				GetLineNumber(src_mgr, start_loc),
				GetColumnNumber(src_mgr, start_loc) + binary_operator.length());
		StmtContext &stmt_context = context->getStmtContext();

		// Return True if expr is in mutation range, NOT inside array decl size
		// and NOT inside enum declaration.
		if (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				!stmt_context.IsInArrayDeclSize() &&
				!stmt_context.IsInEnumDecl() &&
				domain_.find(binary_operator) != domain_.end())
			return true;
	}

	return false;
}

void OBRN::Mutate(clang::Expr *e, MusicContext *context)
{
	BinaryOperator *bo;
	if (!(bo = dyn_cast<BinaryOperator>(e)))
		return;

	string token{bo->getOpcodeStr()};
	SourceLocation start_loc = bo->getOperatorLoc();
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	
	// cout << "cp obrn 2\n";
	SourceLocation end_loc = src_mgr.translateLineCol(
			src_mgr.getMainFileID(),
			GetLineNumber(src_mgr, start_loc),
			GetColumnNumber(src_mgr, start_loc) + token.length());

	for (auto mutated_token: range_)
		if (token.compare(mutated_token) != 0)
		{
			context->mutant_database_.AddMutantEntry(context->getStmtContext(),
					name_, start_loc, end_loc, token, mutated_token, 
					context->getStmtContext().getProteumStyleLineNum(), token+mutated_token);
		}
}

