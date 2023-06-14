#include "../music_utility.h"
#include "obng.h"

bool OBNG::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool OBNG::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

// Return True if the mutant operator can mutate this expression
bool OBNG::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		SourceLocation start_loc = e->getBeginLoc();
    SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
    StmtContext &stmt_context = context->getStmtContext();

    // OPPO can mutate binary bitwise expression in mutation range,
    // outside array decl size and enum declaration.
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
	    		 bo->isBitwiseOp() &&
	    		 !stmt_context.IsInArrayDeclSize() &&
					 !stmt_context.IsInEnumDecl();
	}

	return false;
}

void OBNG::Mutate(clang::Expr *e, MusicContext *context)
{
	BinaryOperator *bo;
	if (!(bo = dyn_cast<BinaryOperator>(e)))
		return;

	// Generate mutant by negating the whole expression
	if (range_.empty() ||
      (!range_.empty() && range_.find("whole") != range_.end()))
		GenerateMutantByNegation(e, context, "both");

	// Generate mutant by negating right hand side of expr
	if (range_.empty() ||
      (!range_.empty() && range_.find("right") != range_.end()))
		GenerateMutantByNegation(bo->getRHS()->IgnoreImpCasts(), context, "right");

	// Generate mutant by negating left hand side of expr
	if (range_.empty() ||
      (!range_.empty() && range_.find("left") != range_.end()))
		GenerateMutantByNegation(bo->getLHS()->IgnoreImpCasts(), context, "left");
}

void OBNG::GenerateMutantByNegation(Expr *e, MusicContext *context, string side)
{
  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_); 

  Rewriter rewriter;
  rewriter.setSourceMgr(
  		context->comp_inst_->getSourceManager(),
  		context->comp_inst_->getLangOpts());
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};    

  string mutated_token = "~(" + token + ")";

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum(), side);
}