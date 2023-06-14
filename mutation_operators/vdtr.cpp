#include "../music_utility.h"
#include "vdtr.h"

bool VDTR::ValidateDomain(const std::set<std::string> &domain)
{
  for (auto e: domain)
    if (!IsValidVariableName(e))
      return false;

  return true;

  // return domain.empty();
}

bool VDTR::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
  //   if (!IsValidVariableName(e))
  //     return false;

  return true;

  // return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VDTR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  if (!ExprIsScalarReference(e))
    return false;

  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
  StmtContext &stmt_context = context->getStmtContext();

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         !stmt_context.IsInEnumDecl() &&
         !stmt_context.IsInArrayDeclSize() &&
         !stmt_context.IsInLhsOfAssignmentRange(e) &&
         !stmt_context.IsInUnaryIncrementDecrementRange(e) &&
         !stmt_context.IsInAddressOpRange(e) &&
         stmt_context.IsInCurrentlyParsedFunctionRange(e) &&
         is_in_domain;
}

void VDTR::Mutate(clang::Expr *e, MusicContext *context)
{
  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  SourceLocation start_of_file = src_mgr.getLocForStartOfFile(src_mgr.getMainFileID());
  vector<string> extra_tokens{""};
  vector<string> extra_mutated_tokens{"#include <sys/types.h>\n#include <signal.h>\n#include <unistd.h>\n"};
  vector<SourceLocation> extra_start_locs{start_of_file};
  vector<SourceLocation> extra_end_locs{start_of_file};

  string mutated_token_neg{"((" + token + ") < 0 ? kill(getpid(), 9) : (" + token + "))"};
  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token_neg, 
      context->getStmtContext().getProteumStyleLineNum(), "NEG",
      extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);

  string mutated_token_zero{"((" + token + ") == 0 ? kill(getpid(), 9) : (" + token + "))"};
  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token_zero, 
      context->getStmtContext().getProteumStyleLineNum(), "ZERO",
      extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);

  string mutated_token_pos{"((" + token + ") > 0 ? kill(getpid(), 9) : (" + token + "))"};
  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token_pos, 
      context->getStmtContext().getProteumStyleLineNum(), "POS",
      extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);
}
