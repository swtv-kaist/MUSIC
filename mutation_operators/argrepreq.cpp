#include "../music_utility.h"
#include "argrepreq.h"

/* The domain for ArgRepReq must be names of functions whose
   function calls will be mutated. */
bool ArgRepReq::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool ArgRepReq::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void ArgRepReq::setRange(std::set<std::string> &range)
{
  /*for (auto it = range.begin(); it != range.end(); )
  {
    if (HandleRangePartition(*it))
      it = range.erase(it);
    else
      ++it;
  }

  range_ = range;*/

  // for (auto it: partitions)
  //   cout << "part: " << it << endl;

  // for (auto it: range_)
  //   cout << "range: " << it << endl;
}

// Return True if the mutant operator can mutate this expression
bool ArgRepReq::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  if (CallExpr *ce = dyn_cast<CallExpr>(e))
  {
    SourceLocation start_loc = ce->getBeginLoc();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    FunctionDecl *fd = ce->getDirectCallee();
    if (!fd)
      return false;

    // if (!domain_.empty() && 
    //     !IsStringElementOfSet(fd->getNameAsString(), domain_))
    //   return false;

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is scalar type.
    return (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
            !context->getStmtContext().IsInEnumDecl() && 
            ce->getNumArgs() > 0);
  }

  return false;
}

void ArgRepReq::Mutate(clang::Expr *e, MusicContext *context)
{
  CallExpr *ce;
  if (!(ce = dyn_cast<CallExpr>(e)))
    return;

  // SourceLocation start_loc = ce->getBeginLoc();
  // getRParenLoc returns the location before the right parenthesis
  // SourceLocation end_loc = ce->getRParenLoc();
  // end_loc = end_loc.getLocWithOffset(1);

  for (auto it = ce->arg_begin(); it != ce->arg_end(); it++)
  {
    Expr *arg = (*it)->IgnoreImpCasts();
    if (ExprIsStruct(arg))
      continue;

    SourceLocation start_loc = arg->getBeginLoc();
    SourceLocation end_loc = GetEndLocOfExpr(arg, context->comp_inst_);

    string token{ConvertToString(arg, context->comp_inst_->getLangOpts())};
    
    vector<string> mutated_tokens;
    if (ExprIsFloat(arg))
      mutated_tokens = {"1.0", "-1.0", "0.0"};
    else
      mutated_tokens = {"1", "-1", "0"};

    mutated_tokens.push_back(GetMaxValue(arg->getType()));
    mutated_tokens.push_back(GetMinValue(arg->getType()));

    vector<string> additional_infos{"1", "-1", "0", "max", "min"};

    for (int idx = 0; idx < mutated_tokens.size(); idx++)
      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_tokens[idx], 
          context->getStmtContext().getProteumStyleLineNum(), additional_infos[idx]);
  }  
}
