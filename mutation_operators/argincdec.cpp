#include "../music_utility.h"
#include "argincdec.h"

/* The domain for ArgIncDec must be names of functions whose
   function calls will be mutated. */
bool ArgIncDec::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool ArgIncDec::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void ArgIncDec::setRange(std::set<std::string> &range)
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
bool ArgIncDec::IsMutationTarget(clang::Expr *e, MusicContext *context)
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

    // Return True if ftn call is in mutation range, NOT inside enum decl
    // and has at least 1 argument.
    return (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
            !context->getStmtContext().IsInEnumDecl() && 
            ce->getNumArgs() > 0);
  }

  return false;
}

void ArgIncDec::Mutate(clang::Expr *e, MusicContext *context)
{
  CallExpr *ce;
  if (!(ce = dyn_cast<CallExpr>(e)))
    return;

  // SourceLocation start_loc = ce->getLocStart();
  // getRParenLoc returns the location before the right parenthesis
  // SourceLocation end_loc = ce->getRParenLoc();
  // end_loc = end_loc.getLocWithOffset(1);

  for (auto it = ce->arg_begin(); it != ce->arg_end(); it++)
  {
    Expr *arg = (*it)->IgnoreImpCasts();
    if (!ExprIsScalar(arg) && !ExprIsPointer(arg))
      continue;

    SourceLocation start_loc = arg->getLocStart();
    SourceLocation end_loc = GetEndLocOfExpr(arg, context->comp_inst_);

    string token{ConvertToString(arg, context->comp_inst_->getLangOpts())};
    string mutated_token1{token + "+1"};
    string mutated_token2{token + "-1"};

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token1, 
        context->getStmtContext().getProteumStyleLineNum(), "plusone");

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token2, 
        context->getStmtContext().getProteumStyleLineNum(), "minusone");
  }  
}
