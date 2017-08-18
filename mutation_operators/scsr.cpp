#include "../comut_utility.h"
#include "scsr.h"

bool SCSR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool SCSR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool SCSR::CanMutate(clang::Expr *e, ComutContext *context)
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
bool SCSR::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void SCSR::Mutate(clang::Expr *e, ComutContext *context)
{
	// use to prevent duplicate mutants from local/global string literals
	set<string> stringCache;
	GenerateGlobalMutants(e, context, &stringCache);
	GenerateLocalMutants(e, context, &stringCache);
}

void SCSR::Mutate(clang::Stmt *s, ComutContext *context)
{}

void SCSR::GenerateGlobalMutants(Expr *e, ComutContext *context,
																 set<string> *stringCache)
{
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfStringLiteral(src_mgr, start_loc);

	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());
	string token{rewriter.ConvertToString(e)};

	// All string literals from global list are distinct 
  // (filtered from InformationGatherer).
  for (auto mutated_token: *(context->global_stringliteral_list))
    if (mutated_token.compare(token) != 0)
    {
      GenerateMutantFile(context, start_loc, end_loc, mutated_token);
			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																	token, mutated_token);

      stringCache->insert(mutated_token);
    }
}

void SCSR::GenerateLocalMutants(Expr *e, ComutContext *context,
															  set<string> *stringCache)
{
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfStringLiteral(src_mgr, start_loc);

	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());
	string token{rewriter.ConvertToString(e)};

	if (!LocationIsInRange(
			start_loc, *(context->currently_parsed_function_range)))
		return;

	for (auto mutated_token: *(context->local_stringliteral_list))
	{
		// Do not mutate to any string literals outside current function
    if (LocationBeforeRangeStart(
    		mutated_token.second, 
    		*(context->currently_parsed_function_range)))
      continue;

    // A string literal outside current function range signals
    // all following string literals are also outside.
    if (!LocationIsInRange(
    		mutated_token.second, 
    		*(context->currently_parsed_function_range)))
      break;

    // mutate if the literal is not the same as the token
    // and prevent duplicate if the literal is already in the cache
    if (mutated_token.first.compare(token) != 0 &&
        stringCache->find(mutated_token.first) == stringCache->end())
    {
      stringCache->insert(mutated_token.first);

      GenerateMutantFile(context, start_loc, end_loc, mutated_token.first);
			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																	token, mutated_token.first);
    }
	}
}
