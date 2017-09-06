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
    StmtContext &stmt_context = context->getStmtContext();

    // Mutation is applicable if this expression is in mutation range,
    // not inside an enum declaration and not inside field decl range.
    // FieldDecl is a member of a struct or union.
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
    			 !stmt_context.IsInEnumDecl() &&
    			 !stmt_context.IsInFieldDeclRange(e);
	}

	return false;
}



void SCSR::Mutate(clang::Expr *e, ComutContext *context)
{
	// use to prevent duplicate mutants from local/global string literals
	set<string> stringCache;
	GenerateGlobalMutants(e, context, &stringCache);
	GenerateLocalMutants(e, context, &stringCache);
}



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
  for (auto it: *(context->getSymbolTable()->getGlobalStringLiteralList()))
  {
  	string mutated_token{rewriter.ConvertToString(it)};

    if (mutated_token.compare(token) != 0)
    {
      GenerateMutantFile(context, start_loc, end_loc, mutated_token);
			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																	token, mutated_token);

      stringCache->insert(mutated_token);
    }
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

	if (!context->getStmtContext().IsInCurrentlyParsedFunctionRange(e))
		return;

	for (auto it: (*(context->getSymbolTable()->getLocalStringLiteralList()))[context->getFunctionId()])
	{
		string mutated_token = rewriter.ConvertToString(it);

    // mutate if the literal is not the same as the token
    // and prevent duplicate if the literal is already in the cache
    if (mutated_token.compare(token) != 0 &&
        stringCache->find(mutated_token) == stringCache->end())
    {
      stringCache->insert(mutated_token);

      GenerateMutantFile(context, start_loc, end_loc, mutated_token);
			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																	token, mutated_token);
    }
	}
}
