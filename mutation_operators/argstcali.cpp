#include "../music_utility.h"
#include "argstcali.h"

/* The domain for ArgStcAli must be names of functions whose
   function calls will be mutated. */
bool ArgStcAli::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool ArgStcAli::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void ArgStcAli::setRange(std::set<std::string> &range)
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
bool ArgStcAli::IsMutationTarget(clang::Expr *e, MusicContext *context)
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
            ce->getNumArgs() > 1);
  }

  return false;
}

void ArgStcAli::Mutate(clang::Expr *e, MusicContext *context)
{
  CallExpr *ce;
  if (!(ce = dyn_cast<CallExpr>(e)))
    return;

  SourceLocation start_loc = ce->getBeginLoc();
  // getRParenLoc returns the location before the right parenthesis
  SourceLocation end_loc = ce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  string ftn_name{ce->getDirectCallee()->getNameAsString()};

  // collect the string of each function argument
  vector<string> arg_strings;
  for (auto it = ce->arg_begin(); it != ce->arg_end(); it++)
    arg_strings.push_back(
        ConvertToString((*it)->IgnoreImpCasts(), context->comp_inst_->getLangOpts()));

  int idx1 = 0;
  for (auto it = ce->arg_begin(); it != ce->arg_end(); it++, idx1++)
  {
    Expr *arg = (*it)->IgnoreImpCasts();
    
    auto it2 = it;
    it2++;

    int idx2 = 0;
    for (; it2 != ce->arg_end(); it2++, idx2++)
    {
      Expr *arg2 = (*it2)->IgnoreImpCasts();

      if (isCompatibleType(arg->getType(), arg2->getType()))
      {
        string mutated_token{ftn_name + "("};
        for (int i = 0; i < arg_strings.size(); i++)
        {
          if (i == idx1)
            mutated_token += arg_strings[idx2];
          else 
            if (i == idx2)
              mutated_token += arg_strings[idx1];
            else
              mutated_token += arg_strings[i];

          if (i == arg_strings.size()-1)
            mutated_token += ")";
          else
            mutated_token += ",";
        }

        context->mutant_database_.AddMutantEntry(context->getStmtContext(),
            name_, start_loc, end_loc, token, mutated_token, 
            context->getStmtContext().getProteumStyleLineNum());
      }
    }
  }  
}
