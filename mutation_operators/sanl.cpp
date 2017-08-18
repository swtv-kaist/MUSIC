#include "../comut_utility.h"
#include "sanl.h"

bool SANL::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool SANL::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool SANL::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (StringLiteral *sl = dyn_cast<StringLiteral>(e))
	{
		SourceLocation start_loc = sl->getLocStart();
    SourceLocation end_loc = GetEndLocOfStringLiteral(
    		context->comp_inst->getSourceManager(), start_loc);

    // Mutation is applicable if this expression is in mutation range,
    // not inside an enum declaration and not inside field decl range.
    // FieldDecl is a member of a struct or union.
    return Range1IsPartOfRange2(
				SourceRange(start_loc, end_loc), 
				SourceRange(*(context->userinput->getStartOfMutationRange()),
										*(context->userinput->getEndOfMutationRange()))) &&
    			 !context->is_inside_enumdecl &&
    			 !LocationIsInRange(start_loc, *(context->fielddecl_range));
	}

	return false;
}

// Return True if the mutant operator can mutate this statement
bool SANL::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void SANL::Mutate(clang::Expr *e, ComutContext *context)
{
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfStringLiteral(src_mgr, start_loc);

	string token{rewriter.ConvertToString(e)};
	
	string mutated_token{token};
	mutated_token.pop_back();
	mutated_token += "\\n\"";

	GenerateMutantFile(context, start_loc, end_loc, mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																	token, mutated_token);
}

void SANL::Mutate(clang::Stmt *s, ComutContext *context)
{}