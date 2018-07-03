#include "../music_utility.h"
#include "clsr.h"

bool CLSR::ValidateDomain(const std::set<std::string> &domain)
{
	for (auto e: domain)
    if (!IsValidVariableName(e))
      return false;

  return true;
}

bool CLSR::ValidateRange(const std::set<std::string> &range)
{
  // validation check might improve runtime if user specifies a lot.
  // However, currently not necessary.
	return true;
}

// Return True if the mutant operator can mutate this expression
bool CLSR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (!ExprIsDeclRefExpr(e) || !ExprIsScalar(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
	StmtContext &stmt_context = context->getStmtContext();

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

	// CLSR can mutate scalar-type Declaration Reference Expression
	// inside mutation range, outside enum declaration, array decl size
	// (vulnerable to different uncompilable cases) and outside 
	// lhs of assignment, unary increment/decrement/addressop (these
	// cannot take constant literal as their target)
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInEnumDecl() &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInLhsOfAssignmentRange(e) &&
				 !stmt_context.IsInUnaryIncrementDecrementRange(e) &&
				 !stmt_context.IsInAddressOpRange(e) &&
				 stmt_context.IsInCurrentlyParsedFunctionRange(e) &&
         is_in_domain;
}

void CLSR::Mutate(clang::Expr *e, MusicContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

	// cannot mutate the variable in switch condition or 
  // array subscript to a floating-type variable
  StmtContext &stmt_context = context->getStmtContext();
  bool skip_float_literal = stmt_context.IsInArraySubscriptRange(e) ||
                            stmt_context.IsInSwitchStmtConditionRange(e) ||
                            stmt_context.IsInSwitchCaseRange(e) ||
                            stmt_context.IsInNonFloatingExprRange(e);

  for (auto it: (*(context->getSymbolTable()->getLocalScalarConstantList()))[context->getFunctionId()])
  {
    string mutated_token{
        ConvertToString(it, context->comp_inst_->getLangOpts())};

    if (!range_.empty() && !IsStringElementOfSet(mutated_token, range_))
      continue;

  	if (skip_float_literal && ExprIsFloat(it))
      continue;

    // This is for same mutant removal procedure during AddMutantEntry
    if (mutated_token.front() == '\'' && mutated_token.back() == '\'')
    	mutated_token = ConvertCharStringToIntString(mutated_token);

    context->mutant_database_.AddMutantEntry(
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());
  }
}