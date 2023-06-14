#include "../music_utility.h"
#include "srws.h"

bool SRWS::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool SRWS::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

// Return True if the mutant operator can mutate this expression
bool SRWS::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
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

	return false;
}

void SRWS::Mutate(clang::Expr *e, MusicContext *context)
{
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfStringLiteral(src_mgr, start_loc);

  Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());
	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

	int first_non_whitespace = GetFirstNonWhitespaceIndex(token);
  int last_non_whitespace = GetLastNonWhitespaceIndex(token);

  // Generate mutant by removing whitespaces
  // only when there is some whitespace in front
  if (first_non_whitespace != 1 &&
      (range_.empty() || range_.find("trimfront") != range_.end()))
  {
    string mutated_token = "\"" + token.substr(first_non_whitespace);
    
    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum(), "front");
  }

  // Generate mutant by removing trailing whitespaces
  // only when there is whitespace in the back
  if (last_non_whitespace < token.length()-2)
  {
    if (range_.empty() || range_.find("trimback") != range_.end())
    {
      string mutated_token = token.substr(0, last_non_whitespace+1) + "\"";
      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum(), "back");
    }

    // Generate mutant by removing whitespaces in the front and back
    if (first_non_whitespace != 1 &&
        (range_.empty() || range_.find("trimboth") != range_.end()))
    {
      string mutated_token = "\"";
      int str_length = last_non_whitespace - first_non_whitespace + 1;

      mutated_token += token.substr(first_non_whitespace, str_length);
      mutated_token += "\"";

      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum(), "both");
    }
  }
}


