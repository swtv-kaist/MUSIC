#include "../music_utility.h"
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
bool OCNG::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
	SourceLocation start_loc, end_loc;
	Expr *condition{nullptr};

	if (IfStmt *is = dyn_cast<IfStmt>(s))
	{
		// empty condition
		if (is->getCond() == nullptr)
			return false;

		condition = is->getCond()->IgnoreImpCasts();
		start_loc = condition->getBeginLoc();

		if (start_loc.isInvalid())
			goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
	}
	else if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
	{
		// empty condition
		if (ws->getCond() == nullptr)
      return false;

		condition = ws->getCond()->IgnoreImpCasts();
		start_loc = condition->getBeginLoc();

		if (start_loc.isInvalid())
			goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
	}
	else if (DoStmt *ds = dyn_cast<DoStmt>(s))
	{
		// empty condition
		if (ds->getCond() == nullptr)
			return false;

		condition = ds->getCond()->IgnoreImpCasts();
		start_loc = condition->getBeginLoc();

		if (start_loc.isInvalid())
			goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
	}
	else if (ForStmt *fs = dyn_cast<ForStmt>(s))
	{
		// empty condition
		if (fs->getCond() == nullptr)
			return false;

		condition = fs->getCond()->IgnoreImpCasts();
		start_loc = condition->getBeginLoc();

		if (start_loc.isInvalid())
			goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
	}
	else if (
			AbstractConditionalOperator *aco = dyn_cast<AbstractConditionalOperator>(s))
	{
		// empty condition
		if (aco->getCond() == nullptr)
			return false;

		condition = aco->getCond()->IgnoreImpCasts();
		start_loc = condition->getBeginLoc();

		if (start_loc.isInvalid())
			goto invalid_start_loc;

    end_loc = GetEndLocOfExpr(condition, context->comp_inst_);
	}
	else
		return false;

	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));

	invalid_start_loc:
	cout << "OCNG: cannot retrieve start loc for condition of ";
	ConvertToString(s, context->comp_inst_->getLangOpts());
	cout << endl;
	return false;
}

void OCNG::Mutate(clang::Stmt *s, MusicContext *context)
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

void OCNG::GenerateMutantByNegation(Expr *e, MusicContext *context)
{
  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_); 

  Rewriter rewriter;
  rewriter.setSourceMgr(
  		context->comp_inst_->getSourceManager(),
  		context->comp_inst_->getLangOpts());
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};    

  string mutated_token = "!(" + token + ")";

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum());
}