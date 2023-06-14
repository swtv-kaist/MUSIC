#include "../music_utility.h"
#include "ommo.h"

bool OMMO::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool OMMO::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

// Return True if the mutant operator can mutate this expression
bool OMMO::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
		if (uo->getOpcode() == UO_PostDec || uo->getOpcode() == UO_PreDec)
		{
			SourceLocation start_loc = uo->getBeginLoc();
    	SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst_);

    	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
		  Rewriter rewriter;
		  rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

		  string token{ConvertToString(uo->getSubExpr(), 
		  														 context->comp_inst_->getLangOpts())};
		  bool is_in_domain = domain_.empty() ? true : 
		                      IsStringElementOfSet(token, domain_);

    	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
    				 is_in_domain;
		}

	return false;
}



void OMMO::Mutate(clang::Expr *e, MusicContext *context)
{
	UnaryOperator *uo;
	if (!(uo = dyn_cast<UnaryOperator>(e)))
		return;

	if (uo->getOpcode() == UO_PostDec)  // x--
		GenerateMutantForPostDec(uo, context);
	else  // --x
		GenerateMutantForPreDec(uo, context);
}



void OMMO::GenerateMutantForPostDec(UnaryOperator *uo, MusicContext *context)
{
	SourceLocation start_loc = uo->getBeginLoc();
	SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst_);

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(uo, context->comp_inst_->getLangOpts())};

	if (range_.empty() ||
			(!range_.empty() && range_.find("predec") != range_.end()))
	{
		// generate --x
	  uo->setOpcode(UO_PreDec);
	  string mutated_token = ConvertToString(uo, context->comp_inst_->getLangOpts());

	  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
	  		name_, start_loc, end_loc, token, mutated_token, 
	  		context->getStmtContext().getProteumStyleLineNum(), "predec");
	}

  if (range_.empty() ||
			(!range_.empty() && range_.find("postinc") != range_.end()))
  {
  	// generate x++
	  uo->setOpcode(UO_PostInc);
	  string mutated_token = ConvertToString(uo, context->comp_inst_->getLangOpts());
	  
	  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
	  		name_, start_loc, end_loc, token, mutated_token, 
	  		context->getStmtContext().getProteumStyleLineNum(), "postinc");
  }

  // reset the code structure
  uo->setOpcode(UO_PostDec);
}

void OMMO::GenerateMutantForPreDec(UnaryOperator *uo, MusicContext *context)
{
	SourceLocation start_loc = uo->getBeginLoc();
	SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst_);

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(uo, context->comp_inst_->getLangOpts())};

	if (range_.empty() ||
			(!range_.empty() && range_.find("postdec") != range_.end()))
	{
		// generate x--
	  uo->setOpcode(UO_PostDec);
	  string mutated_token = ConvertToString(uo, context->comp_inst_->getLangOpts());
	 
	 	context->mutant_database_.AddMutantEntry(context->getStmtContext(),
	 			name_, start_loc, end_loc, token, mutated_token, 
	 			context->getStmtContext().getProteumStyleLineNum(), "postdec");
	}


 	if (range_.empty() ||
			(!range_.empty() && range_.find("preinc") != range_.end()))
 	{
 		// generate --x
	  uo->setOpcode(UO_PreInc);
	  string mutated_token = ConvertToString(uo, context->comp_inst_->getLangOpts());

	  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
	  		name_, start_loc, end_loc, token, mutated_token, 
	  		context->getStmtContext().getProteumStyleLineNum(), "preinc");
 	}

  // reset the code structure
  uo->setOpcode(UO_PreDec);
}