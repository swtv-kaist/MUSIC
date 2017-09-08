#include "../comut_utility.h"
#include "clcr.h"

bool CLCR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool CLCR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool CLCR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!isa<CharacterLiteral>(e) && !isa<FloatingLiteral>(e) &&
			!isa<IntegerLiteral>(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);
	StmtContext &stmt_context = context->getStmtContext();

	// CLCR can mutate this constant literal if it is in mutation range,
	// outside array decl range, outside enum decl range, outside
	// field decl range and inside a function (local range)
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInEnumDecl() &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInFieldDeclRange(e) &&
				 stmt_context.IsInCurrentlyParsedFunctionRange(e);
}

void CLCR::Mutate(clang::Expr *e, ComutContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(e)};

	// if token is char, then convert to int string for later comparison
	// to avoid mutating to same value constant.
	string int_string{rewriter.ConvertToString(e)};

  if (int_string.front() == '\'' && int_string.back() == '\'')
    int_string = ConvertCharStringToIntString(int_string);

	// cannot mutate the variable in switch condition, case value, 
  // array subscript to a floating-type variable because
  // these location requires integral value.
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

    if (int_string.compare(mutated_token) == 0)
    	continue;

    // Mitigate mutation from causing duplicate-case-label error.
    // If this constant is in range of a case label
    // then check if the replacing token is same with any other label.
    if (stmt_context.IsInSwitchCaseRange(e) &&
    		IsDuplicateCaseLabel(mutated_token, context->switchstmt_info_list))
    	continue;

    context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
	}
}

bool CLCR::IsDuplicateCaseLabel(
		string new_label, SwitchStmtInfoList *switchstmt_list)
{
	// Convert char value to int for precise comparison
  if (new_label.front() == '\'' && new_label.back() == '\'')
    new_label = ConvertCharStringToIntString(new_label);

  for (auto case_value: (*switchstmt_list).back().second)
    if (new_label.compare(case_value) == 0)
	    return true;

	return false;
}