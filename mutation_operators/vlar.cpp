#include "../comut_utility.h"
#include "vlar.h"

bool VLAR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VLAR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VLAR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsArrayReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	// VLAR can mutate this expression only if it is array type
	// inside mutation range and NOT inside array decl size or enum declaration
	return Range1IsPartOfRange2(
			SourceRange(start_loc, end_loc), 
			SourceRange(*(context->userinput->getStartOfMutationRange()),
									*(context->userinput->getEndOfMutationRange()))) &&
				 !context->is_inside_array_decl_size &&
				 !context->is_inside_enumdecl;
}

// Return True if the mutant operator can mutate this statement
bool VLAR::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void VLAR::Mutate(clang::Expr *e, ComutContext *context)
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

  for (auto scope: *(context->local_array_vardecl_list))
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
          sameArrayElementType(e->getType(), vardecl->getType()))
      {
      	GenerateMutantFile(context, start_loc, end_loc, mutated_token);
				WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																		token, mutated_token);
      }
  	}
}

void VLAR::Mutate(clang::Stmt *s, ComutContext *context)
{}
