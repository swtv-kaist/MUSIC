#include "../comut_utility.h"
#include "vtwd.h"

bool VTWD::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VTWD::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VTWD::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsScalarReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	Rewriter rewriter;
	rewriter.setSourceMgr(
			context->comp_inst->getSourceManager(),
			context->comp_inst->getLangOpts());

	// VTWD can mutate expr that are
	// 		- inside mutation range
	// 		- not inside enum decl
	// 		- not on lhs of assignment (a+1=a -> uncompilable)
	// 		- not inside unary increment/decrement/addressop
	return Range1IsPartOfRange2(
			SourceRange(start_loc, end_loc), 
			SourceRange(*(context->userinput->getStartOfMutationRange()),
									*(context->userinput->getEndOfMutationRange()))) &&
				 !context->is_inside_enumdecl &&
				 !LocationIsInRange(start_loc, *(context->lhs_of_assignment_range)) &&
				 !LocationIsInRange(start_loc, *(context->addressop_range)) &&
				 !LocationIsInRange(start_loc, *(context->unary_increment_range)) &&
				 !LocationIsInRange(start_loc, *(context->unary_decrement_range)) &&
				 CanMutate(rewriter.ConvertToString(e), context);
}

// Return True if the mutant operator can mutate this statement
bool VTWD::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void VTWD::Mutate(clang::Expr *e, ComutContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	Rewriter rewriter;
	rewriter.setSourceMgr(
			context->comp_inst->getSourceManager(),
			context->comp_inst->getLangOpts());
	string token{rewriter.ConvertToString(e)};

	string mutated_token = "(" + token + "+1)";

	GenerateMutantFile(context, start_loc, end_loc, mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																token, mutated_token);

	mutated_token = "(" + token + "-1)";
	GenerateMutantFile(context, start_loc, end_loc, mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																token, mutated_token);
}

void VTWD::Mutate(clang::Stmt *s, ComutContext *context)
{}

bool VTWD::CanMutate(std::string scalarref_name, ComutContext *context)
{
	// if reference name is in the nonMutatableList then it is not mutatable
	ScalarReferenceNameList *scalarref_list = \
			context->non_VTWD_mutatable_scalarref_list;

  for (auto it = (*scalarref_list).begin(); it != (*scalarref_list).end(); ++it)
    if (scalarref_name.compare(*it) == 0)
    {
    	scalarref_list->erase(it);
      return false;
    }

  return true;
}