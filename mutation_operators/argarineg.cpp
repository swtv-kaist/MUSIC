#include "../music_utility.h"
#include "argarineg.h"

/* The domain for ArgAriNeg must be names of functions whose
   function calls will be mutated. */
bool ArgAriNeg::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool ArgAriNeg::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void ArgAriNeg::setRange(std::set<std::string> &range)
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
bool ArgAriNeg::IsMutationTarget(clang::Expr *e, MusicContext *context)
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

void ArgAriNeg::Mutate(clang::Expr *e, MusicContext *context)
{
  // cout << "Mutating " << ConvertToString(e, context->comp_inst_->getLangOpts()) << endl;
  // PrintLocation(context->comp_inst_->getSourceManager(), e->getLocStart());

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
    if (!ExprIsScalar(arg))
      continue;

    string token{ConvertToString(arg, context->comp_inst_->getLangOpts())};
    string mutated_token{"-(" + token + ")"};

    // cout << "cp1\n";

    SourceLocation start_loc = arg->getLocStart();
    SourceLocation end_loc = GetEndLocOfExpr(arg, context->comp_inst_);

    // cout << token << " ~~~ " << mutated_token << endl;
    // PrintLocation(context->comp_inst_->getSourceManager(), start_loc);
    // PrintLocation(context->comp_inst_->getSourceManager(), end_loc);

    // cout << "cp2\n";

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum());

    // cout << "cp3\n";
  }  
}

bool ArgAriNeg::HandleRangePartition(string option) 
{
  vector<string> words;
  SplitStringIntoVector(option, words, string(" "));

  // Return false if this option does not contain enough words to specify 
  // partition or first word is not 'part'
  if (words.size() < 2 || words[0].compare("part") != 0)
    return false;

  for (int i = 1; i < words.size(); i++)
  {
    int num;
    if (ConvertStringToInt(words[i], num))
    {
      if (num > 0 && num <= 10)
        partitions.insert(num);
      else
      {
        cout << "No partition number " << num << ". Skip.\n";
        cout << "There are only 10 partitions for now.\n";
        continue;
      }
    }
    else
    {
      cout << "Cannot convert " << words[i] << " to an integer. Skip.\n";
      continue;
    }
  }

  return true;
}

void ArgAriNeg::ApplyRangePartition(vector<string> *range)
{
  vector<string> range2;
  range2 = *range;

  range->clear();
  sort(range2.begin(), range2.end(), SortStringAscending);

  for (auto part_num: partitions) 
  {
    // Number of possible tokens to mutate to might be smaller than 10.
    // So we do not have 10 partitions.
    if (part_num > range2.size())
    {
      cout << "There are only " << range2.size() << " to mutate to.\n";
      cout << "No partition number " << part_num << endl;
      continue;
    }

    if (range2.size() < num_partitions)
    {
      range->push_back(range2[part_num-1]);
      continue;
    }

    int start_idx = (range2.size() / 10) * (part_num - 1);
    int end_idx = (range2.size() / 10) * part_num;

    if (part_num == 10)
      end_idx = range2.size();

    for (int idx = start_idx; idx < end_idx; idx++)
      range->push_back(range2[idx]);
  }
}
