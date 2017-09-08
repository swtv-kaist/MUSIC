#include "../comut_utility.h"
#include "ommo.h"

bool OMMO::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool OMMO::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool OMMO::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
		if (uo->getOpcode() == UO_PostDec || uo->getOpcode() == UO_PreDec)
		{
			SourceLocation start_loc = uo->getLocStart();
    	SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst);

    	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
		}

	return false;
}



void OMMO::Mutate(clang::Expr *e, ComutContext *context)
{
	UnaryOperator *uo;
	if (!(uo = dyn_cast<UnaryOperator>(e)))
		return;

	if (uo->getOpcode() == UO_PostInc)  // x++
		GenerateMutantForPostDec(uo, context);
	else  // ++x
		GenerateMutantForPreDec(uo, context);
}



void OMMO::GenerateMutantForPostDec(UnaryOperator *uo, ComutContext *context)
{
	SourceLocation start_loc = uo->getLocStart();
	SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst);

	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(uo)};

	// generate --x
  uo->setOpcode(UO_PreDec);
  string mutated_token = rewriter.ConvertToString(uo);

  context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());

  // generate x++
  uo->setOpcode(UO_PostInc);
  mutated_token = rewriter.ConvertToString(uo);
  
  context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());

  // reset the code structure
  uo->setOpcode(UO_PostDec);
}

void OMMO::GenerateMutantForPreDec(UnaryOperator *uo, ComutContext *context)
{
	SourceLocation start_loc = uo->getLocStart();
	SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst);

	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(uo)};

	// generate x--
  uo->setOpcode(UO_PostDec);
  string mutated_token = rewriter.ConvertToString(uo);
 
 	context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());

  // generate --x
  uo->setOpcode(UO_PreDec);
  mutated_token = rewriter.ConvertToString(uo);

  context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());

  // reset the code structure
  uo->setOpcode(UO_PreDec);
}