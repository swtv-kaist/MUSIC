#include "../music_utility.h"
#include "argdel.h"

/* The domain for ArgDel must be names of functions whose
   function calls will be mutated. */
bool ArgDel::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool ArgDel::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void ArgDel::setRange(std::set<std::string> &range)
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
bool ArgDel::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  if (CallExpr *ce = dyn_cast<CallExpr>(e))
  {
    SourceLocation start_loc = ce->getLocStart();

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

void ArgDel::Mutate(clang::Expr *e, MusicContext *context)
{
  CallExpr *ce;
  if (!(ce = dyn_cast<CallExpr>(e)))
    return;

  SourceLocation start_loc = ce->getLocStart();
  // getRParenLoc returns the location before the right parenthesis
  SourceLocation end_loc = ce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  string ftn_name{ce->getDirectCallee()->getNameAsString()};

  for (auto it1 = ce->arg_begin(); it1 != ce->arg_end(); it1++)
  {
    string mutated_token{ftn_name + "("};
    Expr *arg1 = (*it1)->IgnoreImpCasts();
    string arg1_token{ConvertToString(arg1, context->comp_inst_->getLangOpts())};

    bool first = true;
    for (auto it2 = ce->arg_begin(); it2 != ce->arg_end(); it2++) 
    {
      Expr *arg2 = (*it2)->IgnoreImpCasts();
      string arg2_token{ConvertToString(arg2, context->comp_inst_->getLangOpts())};      

      // Delete the argument arg1_token by not including it in the mutated_token
      if (arg2_token.compare(arg1_token) == 0)
        continue;

      if (first)
        first = false;
      else
        mutated_token += ",";

      mutated_token += arg2_token;
    }

    mutated_token += ")";
    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());      
  }  
}
