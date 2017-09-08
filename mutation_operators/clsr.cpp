#include "../comut_utility.h"
#include "clsr.h"

bool CLSR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool CLSR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool CLSR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsDeclRefExpr(e) || !ExprIsScalar(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);
	StmtContext &stmt_context = context->getStmtContext();

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
				 stmt_context.IsInCurrentlyParsedFunctionRange(e);
}

void CLSR::Mutate(clang::Expr *e, ComutContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(e)};

	// cannot mutate the variable in switch condition or 
  // array subscript to a floating-type variable
  StmtContext &stmt_context = context->getStmtContext();
  bool skip_float_literal = stmt_context.IsInArraySubscriptRange(e) ||
                            stmt_context.IsInSwitchStmtConditionRange(e) ||
                            stmt_context.IsInSwitchCaseRange(e);

  for (auto it: (*(context->getSymbolTable()->getLocalScalarConstantList()))[context->getFunctionId()])
  {
  	if (skip_float_literal && ExprIsFloat(it))
              continue;

    string mutated_token{rewriter.ConvertToString(it)};

    if (mutated_token.front() == '\'' && mutated_token.back() == '\'')
    	mutated_token = ConvertCharStringToIntString(mutated_token);


    context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
  }
}