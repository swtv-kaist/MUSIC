#include "../comut_utility.h"
#include "cgcr.h"

bool CGCR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool CGCR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool CGCR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!isa<CharacterLiteral>(e) && !isa<FloatingLiteral>(e) &&
			!isa<IntegerLiteral>(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	// CGCR can mutate this constant literal if it is in mutation range,
	// outside array decl range, outside enum decl range and outside
	// field decl range.
	return Range1IsPartOfRange2(
			SourceRange(start_loc, end_loc), 
			SourceRange(*(context->userinput->getStartOfMutationRange()),
									*(context->userinput->getEndOfMutationRange()))) &&
				 !context->is_inside_enumdecl &&
				 !context->is_inside_array_decl_size &&
				 !LocationIsInRange(start_loc, *(context->fielddecl_range));
}

// Return True if the mutant operator can mutate this statement
bool CGCR::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void CGCR::Mutate(clang::Expr *e, ComutContext *context)
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
  bool skip_float_literal = LocationIsInRange(
  		start_loc, *(context->arraysubscript_range)) ||
                            LocationIsInRange(
      start_loc, *(context->switchstmt_condition_range)) ||
                            LocationIsInRange(
      start_loc, *(context->switchcase_range));

	for (auto it: *(context->global_scalarconstant_list))
	{
		if (skip_float_literal && it.second)
      continue;

    string mutated_token{it.first};

    // Avoid mutating to the same scalar constant
    // If token is char, then convert it to int string for comparison
    if (int_string.compare(mutated_token) == 0)
    	continue;

    // Mitigate mutation from causing duplicate-case-label error.
    // If this constant is in range of a case label
    // then check if the replacing token is same with any other label.
    if (LocationIsInRange(start_loc, *(context->switchcase_range)) &&
    		IsDuplicateCaseLabel(mutated_token, context->switchstmt_info_list))
    	continue;

    GenerateMutantFile(context, start_loc, end_loc, mutated_token);
		WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																	token, mutated_token);
	}
}

void CGCR::Mutate(clang::Stmt *s, ComutContext *context)
{}

bool CGCR::IsDuplicateCaseLabel(
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