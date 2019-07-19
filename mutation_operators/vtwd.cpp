#include "../music_utility.h"
#include "vtwd.h"

bool VTWD::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool VTWD::ValidateRange(const std::set<std::string> &range)
{
	return range.empty() || 
				 (range.size() == 1 &&
					(range.find("plusone") != range.end() || 
					range.find("minusone") != range.end()));
}

// Return True if the mutant operator can mutate this expression
bool VTWD::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (!ExprIsScalarReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	Rewriter rewriter;
	rewriter.setSourceMgr(
			context->comp_inst_->getSourceManager(),
			context->comp_inst_->getLangOpts());
	StmtContext &stmt_context = context->getStmtContext();

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
	bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

	// VTWD can mutate expr that are
	// 		- inside mutation range
	// 		- not inside enum decl
	// 		- not on lhs of assignment (a+1=a -> uncompilable)
	// 		- not inside unary increment/decrement/addressop
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         !stmt_context.IsInEnumDecl() &&
				 !stmt_context.IsInLhsOfAssignmentRange(e) &&
				 !stmt_context.IsInAddressOpRange(e) && is_in_domain &&
				 !stmt_context.IsInUnaryIncrementDecrementRange(e) &&
				 IsMutationTarget(
				 		ConvertToString(e, context->comp_inst_->getLangOpts()), context);
}



void VTWD::Mutate(clang::Expr *e, MusicContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	Rewriter rewriter;
	rewriter.setSourceMgr(
			context->comp_inst_->getSourceManager(),
			context->comp_inst_->getLangOpts());
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



bool VTWD::IsMutationTarget(std::string scalarref_name, MusicContext *context)
{
	// if reference name is in the nonMutatableList then it is not mutatable
	ScalarReferenceNameList *scalarref_list = \
			context->non_VTWD_mutatable_scalarref_list_;

  for (auto it = (*scalarref_list).begin(); it != (*scalarref_list).end(); ++it)
    if (scalarref_name.compare(*it) == 0)
    {
    	scalarref_list->erase(it);
      return false;
    }

  return true;
}