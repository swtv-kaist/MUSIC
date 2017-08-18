#include "../comut_utility.h"
#include "vlpr.h"

bool VLPR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VLPR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VLPR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsPointerReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	// VLPR can mutate this expression only if it is a pointer expression
	// inside mutation range and NOT inside array decl size or enum declaration
	return Range1IsPartOfRange2(
			SourceRange(start_loc, end_loc), 
			SourceRange(*(context->userinput->getStartOfMutationRange()),
									*(context->userinput->getEndOfMutationRange()))) &&
				 !context->is_inside_array_decl_size &&
				 !context->is_inside_enumdecl;
}

// Return True if the mutant operator can mutate this statement
bool VLPR::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void VLPR::Mutate(clang::Expr *e, ComutContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(e)};

	// cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = LocationIsInRange(
      start_loc, *(context->switchstmt_condition_range));

  // cannot mutate a variable in lhs of assignment to a const variable
  bool skip_const_vardecl = LocationIsInRange(
      start_loc, *(context->lhs_of_assignment_range));

  bool skip_register_vardecl = LocationIsInRange(
  		start_loc, *(context->addressop_range));

  string pointee_type = getPointerType(e->getType());

  for (auto scope: *(context->local_pointer_vardecl_list))
  	for (auto vardecl: scope)
  	{
  		if (skip_const_vardecl && IsVarDeclConst(vardecl)) 
        continue;   

      if (skip_float_vardecl && IsVarDeclFloating(vardecl))
        continue;

      if (skip_register_vardecl && 
          vardecl->getStorageClass() == SC_Register)
        continue;

      string mutated_token{GetVarDeclName(vardecl)};

      if (token.compare(mutated_token) != 0 &&
          pointee_type.compare(getPointerType(vardecl->getType())) == 0)
      {
      	GenerateMutantFile(context, start_loc, end_loc, mutated_token);
				WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																		token, mutated_token);
      }
  	}
}

void VLPR::Mutate(clang::Stmt *s, ComutContext *context)
{}
