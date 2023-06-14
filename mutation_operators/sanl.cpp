#include "../music_utility.h"
#include "sanl.h"

bool SANL::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool SANL::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool SANL::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  // if (GetLineNumber(context->comp_inst_->getSourceManager(), e->getBeginLoc()) == 49)
  //   cout << "SANL can mutate?\n";

	if (StringLiteral *sl = dyn_cast<StringLiteral>(e))
	{
		SourceLocation start_loc = sl->getBeginLoc();
    SourceLocation end_loc = GetEndLocOfStringLiteral(
    		context->comp_inst_->getSourceManager(), start_loc);
    StmtContext &stmt_context = context->getStmtContext();

    // Mutation is applicable if this expression is in mutation range,
    // not inside an enum declaration and not inside field decl range.
    // FieldDecl is a member of a struct or union.
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
    			 !stmt_context.IsInEnumDecl() &&
    			 !stmt_context.IsInFieldDeclRange(e);
	}

  // if (GetLineNumber(context->comp_inst_->getSourceManager(), e->getBeginLoc()) == 49)
  //   cout << "not a string literal: " << e->getStmtClassName() << endl;

	return false;
}



void SANL::Mutate(clang::Expr *e, MusicContext *context)
{
  // if (GetLineNumber(context->comp_inst_->getSourceManager(), e->getBeginLoc()) == 49)
  //   cout << "mutating\n";

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfStringLiteral(src_mgr, start_loc);

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
	
	string mutated_token{token};
	mutated_token.pop_back();
	mutated_token += "\\n\"";

	context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum());
}

