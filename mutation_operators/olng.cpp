#include "../music_utility.h"
#include "olng.h"

bool OLNG::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool OLNG::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

// Return True if the mutant operator can mutate this expression
bool OLNG::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
	{
		SourceLocation start_loc = e->getLocStart();
    SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
    StmtContext &stmt_context = context->getStmtContext();

    // OPPO can mutate binary logical expression in mutation range,
    // outside array decl size and enum declaration.
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
    		bo->isLogicalOp() &&
    		!stmt_context.IsInArrayDeclSize() &&
				!stmt_context.IsInEnumDecl();
	}

	return false;
}

void OLNG::Mutate(clang::Expr *e, MusicContext *context)
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



void OLNG::GenerateMutantByNegation(Expr *e, MusicContext *context, string side)
{
  SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_); 

  // cout << "OLNG end loc is: ";
  // PrintLocation(context->comp_inst_->getSourceManager(), end_loc);

  Rewriter rewriter;
  rewriter.setSourceMgr(
  		context->comp_inst_->getSourceManager(),
  		context->comp_inst_->getLangOpts());
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};    

  string mutated_token = "!(" + token + ")";

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum(), side);
}