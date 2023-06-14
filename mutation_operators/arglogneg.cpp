#include "../music_utility.h"
#include "arglogneg.h"

/* The domain for ArgLogNeg must be names of functions whose
   function calls will be mutated. */
bool ArgLogNeg::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool ArgLogNeg::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void ArgLogNeg::setRange(std::set<std::string> &range)
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
bool ArgLogNeg::IsMutationTarget(clang::Expr *e, MusicContext *context)
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

void ArgLogNeg::Mutate(clang::Expr *e, MusicContext *context)
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
    string mutated_token{"!(" + token + ")"};

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());
  }  
}
