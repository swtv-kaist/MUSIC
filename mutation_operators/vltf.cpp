#include "../comut_utility.h"
#include "vltf.h"

bool VLTF::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VLTF::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VLTF::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (CallExpr *ce = dyn_cast<CallExpr>(e))
	{
		SourceLocation start_loc = ce->getLocStart();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is structure type.
		return (Range1IsPartOfRange2(
				SourceRange(start_loc, end_loc), 
				SourceRange(*(context->userinput->getStartOfMutationRange()),
										*(context->userinput->getEndOfMutationRange()))) &&
						!context->is_inside_enumdecl &&
						ExprIsStruct(e));
	}

	return false;
}

// Return True if the mutant operator can mutate this statement
bool VLTF::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void VLTF::Mutate(clang::Expr *e, ComutContext *context)
{
	CallExpr *ce;
	if (!(ce = dyn_cast<CallExpr>(e)))
		return;

	SourceLocation start_loc = ce->getLocStart();

  // getRParenLoc returns the location before the right parenthesis
  SourceLocation end_loc = ce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(e)};

	// cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = LocationIsInRange(
  		start_loc, *(context->switchstmt_condition_range));

  string struct_type{
  		getStructureType(e->getType().getCanonicalType())};

  for (auto scope: *(context->local_struct_vardecl_list))
  	for (auto vardecl: scope)
  	{
  		if (skip_float_vardecl && IsVarDeclFloating(vardecl))
        continue;

      string mutated_token{GetVarDeclName(vardecl)};

      // Mutateif 2 variable have exactly same structure type
      if (token.compare(mutated_token) != 0 &&
          struct_type.compare(getStructureType(vardecl->getType())) == 0)
      {
      	GenerateMutantFile(context, start_loc, end_loc, mutated_token);
				WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, token, 
																			mutated_token);
      }
  	}
}

void VLTF::Mutate(clang::Stmt *s, ComutContext *context)
{}
