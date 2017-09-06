#include "../comut_utility.h"
#include "ocng.h"

bool OCNG::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool OCNG::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this statement
bool OCNG::CanMutate(clang::Stmt *s, ComutContext *context)
{
	SourceLocation start_loc, end_loc;
	Expr *condition{nullptr};

	if (IfStmt *is = dyn_cast<IfStmt>(s))
	{
		// empty condition
		if (is->getCond() == nullptr)
			return false;

		condition = is->getCond()->IgnoreImpCasts();
		start_loc = condition->getLocStart();
    end_loc = GetEndLocOfExpr(condition, context->comp_inst);
	}
	else if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
	{
		// empty condition
		if (ws->getCond() == nullptr)
			return false;

		condition = ws->getCond()->IgnoreImpCasts();
		start_loc = condition->getLocStart();
    end_loc = GetEndLocOfExpr(condition, context->comp_inst);
	}
	else if (DoStmt *ds = dyn_cast<DoStmt>(s))
	{
		// empty condition
		if (ds->getCond() == nullptr)
			return false;

		condition = ds->getCond()->IgnoreImpCasts();
		start_loc = condition->getLocStart();
    end_loc = GetEndLocOfExpr(condition, context->comp_inst);
	}
	else if (ForStmt *fs = dyn_cast<ForStmt>(s))
	{
		// empty condition
		if (fs->getCond() == nullptr)
			return false;

		condition = fs->getCond()->IgnoreImpCasts();
		start_loc = condition->getLocStart();
    end_loc = GetEndLocOfExpr(condition, context->comp_inst);
	}
	else if (
			AbstractConditionalOperator *aco = dyn_cast<AbstractConditionalOperator>(s))
	{
		// empty condition
		if (aco->getCond() == nullptr)
			return false;

		condition = aco->getCond()->IgnoreImpCasts();
		start_loc = condition->getLocStart();
    end_loc = GetEndLocOfExpr(condition, context->comp_inst);
	}
	else
		return false;

	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
}

void OCNG::Mutate(clang::Stmt *s, ComutContext *context)
{
	Expr *condition{nullptr};

	if (IfStmt *is = dyn_cast<IfStmt>(s))
		condition = is->getCond()->IgnoreImpCasts();
	else if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
		condition = ws->getCond()->IgnoreImpCasts();
	else if (DoStmt *ds = dyn_cast<DoStmt>(s))
		condition = ds->getCond()->IgnoreImpCasts();
	else if (ForStmt *fs = dyn_cast<ForStmt>(s))
		condition = fs->getCond()->IgnoreImpCasts();
	else if (
			AbstractConditionalOperator *aco = dyn_cast<AbstractConditionalOperator>(s))
		condition = aco->getCond()->IgnoreImpCasts();
	else
		return;

	if (condition != nullptr)
		GenerateMutantByNegation(condition, context);
}

void OCNG::GenerateMutantByNegation(Expr *e, ComutContext *context)
{
  SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst); 

  Rewriter rewriter;
  rewriter.setSourceMgr(
  		context->comp_inst->getSourceManager(),
  		context->comp_inst->getLangOpts());
  string token{rewriter.ConvertToString(e)};    

  string mutated_token = "!(" + token + ")";

  GenerateMutantFile(context, start_loc, end_loc, mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																token, mutated_token);
}