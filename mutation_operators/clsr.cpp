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

	// CLSR can mutate scalar-type Declaration Reference Expression
	// inside mutation range, outside enum declaration, array decl size
	// (vulnerable to different uncompilable cases) and outside 
	// lhs of assignment, unary increment/decrement/addressop (these
	// cannot take constant literal as their target)
	return Range1IsPartOfRange2(
			SourceRange(start_loc, end_loc), 
			SourceRange(*(context->userinput->getStartOfMutationRange()),
									*(context->userinput->getEndOfMutationRange()))) &&
				 !context->is_inside_enumdecl &&
				 !context->is_inside_array_decl_size &&
				 !LocationIsInRange(start_loc, *(context->lhs_of_assignment_range)) &&
				 !LocationIsInRange(start_loc, *(context->unary_inc_dec_range)) &&
				 !LocationIsInRange(start_loc, *(context->addressop_range)) &&
				 LocationIsInRange(start_loc, *(context->currently_parsed_function_range));
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
  bool skip_float_literal = LocationIsInRange(
  		start_loc, *(context->arraysubscript_range)) ||
                            LocationIsInRange(
      start_loc, *(context->switchstmt_condition_range)) ||
                            LocationIsInRange(
      start_loc, *(context->switchcase_range));

  for (auto it: (*(context->getSymbolTable()->getLocalScalarConstantList()))[context->function_id_])
  {
  	if (skip_float_literal && ExprIsFloat(it))
              continue;

    string mutated_token{rewriter.ConvertToString(it)};

    if (mutated_token.front() == '\'' && mutated_token.back() == '\'')
    	mutated_token = ConvertCharStringToIntString(mutated_token);


    GenerateMutantFile(context, start_loc, end_loc, mutated_token);
		WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																	token, mutated_token);
  }
}