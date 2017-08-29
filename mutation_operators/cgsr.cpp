#include "../comut_utility.h"
#include "cgsr.h"

bool CGSR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool CGSR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool CGSR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsDeclRefExpr(e) || !ExprIsScalar(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	// CGSR can mutate scalar-type Declaration Reference Expression
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
				 !LocationIsInRange(start_loc, *(context->addressop_range));
}

// Return True if the mutant operator can mutate this statement
bool CGSR::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void CGSR::Mutate(clang::Expr *e, ComutContext *context)
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

  for (auto it: *(context->global_scalarconstant_list))
  {
  	if (skip_float_literal && it.second)
      continue;

    string mutated_token{it.first};

    GenerateMutantFile(context, start_loc, end_loc, mutated_token);
		WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																	token, mutated_token);
  }
}

void CGSR::Mutate(clang::Stmt *s, ComutContext *context)
{}