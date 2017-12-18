#include "../comut_utility.h"
#include "oppo.h"

bool OPPO::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool OPPO::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool OPPO::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
		if (uo->getOpcode() == UO_PostInc || uo->getOpcode() == UO_PreInc)
		{
			SourceLocation start_loc = uo->getLocStart();
    	SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst_);

    	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
		}

	return false;
}



void OPPO::Mutate(clang::Expr *e, ComutContext *context)
{
	UnaryOperator *uo;
	if (!(uo = dyn_cast<UnaryOperator>(e)))
		return;

	if (uo->getOpcode() == UO_PostInc)  // x++
		GenerateMutantForPostInc(uo, context);
	else  // ++x
		GenerateMutantForPreInc(uo, context);
}



void OPPO::GenerateMutantForPostInc(UnaryOperator *uo, ComutContext *context)
{
	SourceLocation start_loc = uo->getLocStart();
	SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst_);

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(uo, context->comp_inst_->getLangOpts())};

	// generate ++x
  uo->setOpcode(UO_PreInc);
  string mutated_token = ConvertToString(uo, context->comp_inst_->getLangOpts());

  context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());

  // generate x--
  uo->setOpcode(UO_PostDec);
  mutated_token = ConvertToString(uo, context->comp_inst_->getLangOpts());

  context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());

  // reset the code structure
  uo->setOpcode(UO_PostInc);
}

void OPPO::GenerateMutantForPreInc(UnaryOperator *uo, ComutContext *context)
{
	SourceLocation start_loc = uo->getLocStart();
	SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst_);

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(uo, context->comp_inst_->getLangOpts())};

	// generate x++
  uo->setOpcode(UO_PostInc);
  string mutated_token = ConvertToString(uo, context->comp_inst_->getLangOpts());
  
  context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());

  // generate --x
  uo->setOpcode(UO_PreDec);
  mutated_token = ConvertToString(uo, context->comp_inst_->getLangOpts());

  context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());

  // reset the code structure
  uo->setOpcode(UO_PreInc);
}