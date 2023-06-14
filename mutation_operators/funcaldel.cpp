#include "../music_utility.h"
#include "funcaldel.h"

/* The domain for FunCalDel must be names of functions whose
   function calls will be mutated. */
bool FunCalDel::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool FunCalDel::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void FunCalDel::setRange(std::set<std::string> &range)
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
bool FunCalDel::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  if (CallExpr *ce = dyn_cast<CallExpr>(e))
  {
    SourceLocation start_loc = ce->getBeginLoc();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    // FunCalDel does not delete void function
    FunctionDecl *fd = ce->getDirectCallee();
    if (!fd || fd->getReturnType().getTypePtr()->isVoidType())
      return false;

    // if (!domain_.empty() && 
    //     !IsStringElementOfSet(fd->getNameAsString(), domain_))
    //   return false;

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is scalar type.
    return (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
            !context->getStmtContext().IsInEnumDecl() &&
            !ExprIsStruct(e));
  }

  return false;
}

void FunCalDel::Mutate(clang::Expr *e, MusicContext *context)
{
  CallExpr *ce;
  if (!(ce = dyn_cast<CallExpr>(e)))
    return;

  SourceLocation start_loc = ce->getBeginLoc();
  // getRParenLoc returns the location before the right parenthesis
  SourceLocation end_loc = ce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  // If the function call is not used in an expression (i.e. single function
  // call stmt), then simply delete it.
  // ISSUE: duplicate with SSDL.
  const Stmt* parent = GetParentOfStmt(e, context->comp_inst_);
  if (parent)
    if (isa<CompoundStmt>(parent))
    {
      string mutated_token{""};
      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum());
      return;
    }

  vector<string> mutated_tokens;
  if (ExprIsFloat(e))
    mutated_tokens = {"1.0", "-1.0", "0.0"};
  else
    mutated_tokens = {"1", "-1", "0"};

  mutated_tokens.push_back(GetMaxValue(e->getType()));
  mutated_tokens.push_back(GetMinValue(e->getType()));

  for (auto mutated_token: mutated_tokens)
    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());
}
