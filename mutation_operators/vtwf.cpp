#include "../music_utility.h"
#include "vtwf.h"

bool VTWF::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool VTWF::ValidateRange(const std::set<std::string> &range)
{
	return true;
}


// Return True if the mutant operator can mutate this expression
bool VTWF::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (CallExpr *ce = dyn_cast<CallExpr>(e))
	{
		SourceLocation start_loc = ce->getBeginLoc();

    // Only delete COMPLETE statements whose parent is a CompoundStmt.
    const Stmt* parent = GetParentOfStmt(e, context->comp_inst_);

    // Single function call statement
    // Mutating +1 or -1 has no impact --> equivalent mutant.
    // And it can sometimes cause uncompilable mutants.
    if (parent)
      if (isa<CompoundStmt>(parent))
        return false;

    string token{
    		ConvertToString(ce->getCallee(), context->comp_inst_->getLangOpts())};
    bool is_in_domain = domain_.empty() ? true : 
                      	IsStringElementOfSet(token, domain_);

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is scalar type.
		return (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
            !context->getStmtContext().IsInEnumDecl() && ExprIsScalar(e)) &&
						is_in_domain;
	}

	return false;
}

void VTWF::Mutate(clang::Expr *e, MusicContext *context)
{
	CallExpr *ce;
	if (!(ce = dyn_cast<CallExpr>(e)))
		return;

	SourceLocation start_loc = ce->getBeginLoc();

  // getRParenLoc returns the location before the right parenthesis
  SourceLocation end_loc = ce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

	if (range_.empty() ||
			(!range_.empty() && range_.find("plusone") != range_.end()))
	{
		string mutated_token = "(" + token + "+1)";
		context->mutant_database_.AddMutantEntry(context->getStmtContext(),
				name_, start_loc, end_loc, token, mutated_token,
				context->getStmtContext().getProteumStyleLineNum(), "plus");
	}
		
	if (range_.empty() ||
			(!range_.empty() && range_.find("minusone") != range_.end()))
	{
		string mutated_token = "(" + token + "-1)";
		context->mutant_database_.AddMutantEntry(context->getStmtContext(),
				name_, start_loc, end_loc, token, mutated_token, 
				context->getStmtContext().getProteumStyleLineNum(), "minus");
	}
}

