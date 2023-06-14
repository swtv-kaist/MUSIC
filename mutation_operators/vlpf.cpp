#include "../music_utility.h"
#include "vlpf.h"

bool VLPF::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool VLPF::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

void VLPF::setRange(std::set<std::string> &range)
{
  for (auto it = range.begin(); it != range.end(); )
  {
    if (HandleRangePartition(*it))
      it = range.erase(it);
    else
      ++it;
  }

  range_ = range;

  // for (auto it: partitions)
  //   cout << "part: " << it << endl;

  // for (auto it: range_)
  //   cout << "range: " << it << endl;
}

// Return True if the mutant operator can mutate this expression
bool VLPF::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (CallExpr *ce = dyn_cast<CallExpr>(e))
	{
		SourceLocation start_loc = ce->getBeginLoc();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    string token{
        ConvertToString(ce->getCallee(), context->comp_inst_->getLangOpts())};
    bool is_in_domain = domain_.empty() ? true : 
                        IsStringElementOfSet(token, domain_);

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is pointer type.
		return (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
            !context->getStmtContext().IsInEnumDecl() && ExprIsPointer(e)) && 
            is_in_domain;
	}

	return false;
}

void VLPF::Mutate(clang::Expr *e, MusicContext *context)
{
	CallExpr *ce;
	if (!(ce = dyn_cast<CallExpr>(e)))
		return;

	SourceLocation start_loc = ce->getBeginLoc();

  // getRParenLoc returns the location before the right parenthesis
  SourceLocation end_loc = ce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

	// get all variable declaration that VLSR can mutate this expr to.
  VarDeclList range(
      (*(context->getSymbolTable()->getLocalPointerVarDeclList()))[context->getFunctionId()]);

  GetRange(e, context, &range);

  vector<string> string_range;

  // type of the entity pointed to by pointer variable
  string pointee_type{
  		getPointerType(e->getType().getCanonicalType())};

  for (auto vardecl: range)
  {
    string mutated_token{GetVarDeclName(vardecl)};

    // Mutated token is not inside user-specified range.
    if (!range_.empty() && range_.find(mutated_token) == range_.end())
      continue;

    if (pointee_type.compare(getPointerType(vardecl->getType())) == 0)
    {
      string_range.push_back(mutated_token);
    }
  }

  // for (auto it: string_range)
  //   cout << "before range: " << it << endl;

  if (partitions.size() > 0)
    ApplyRangePartition(&string_range);

  for (auto it: string_range)
  {
    // cout << "after range: " << it << endl;
    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, it, 
        context->getStmtContext().getProteumStyleLineNum());
  }
}



void VLPF::GetRange(Expr *e, MusicContext *context, VarDeclList *range)
{
  SourceLocation start_loc = e->getBeginLoc();
  Rewriter rewriter;
  rewriter.setSourceMgr(context->comp_inst_->getSourceManager(), 
                        context->comp_inst_->getLangOpts());

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  
  // cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = \
      context->getStmtContext().IsInSwitchStmtConditionRange(e);

  // remove all vardecl appear after expr
  for (auto it = range->begin(); it != range->end(); )
  {
    if (!((*it)->getBeginLoc() < start_loc))
    {
      range->erase(it, range->end());
      break;
    }

    if (skip_float_vardecl && IsVarDeclFloating(*it))
    {
      it = range->erase(it);
      continue;
    }

    ++it;
  }

  for (auto scope: *(context->scope_list_))
  {
    // all vardecl after expr are removed.
    // Hence no need to consider scopes after expr as well.
    if (LocationBeforeRangeStart(start_loc, scope))
      break;

    if (!LocationIsInRange(start_loc, scope))
      for (auto it = range->begin(); it != range->end();)
      {
        if (LocationAfterRangeEnd((*it)->getBeginLoc(), scope))
          break;

        if (LocationIsInRange((*it)->getBeginLoc(), scope))
        {
          it = range->erase(it);
          continue;
        }

        ++it;
      }
  }
}

bool VLPF::HandleRangePartition(string option) 
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

void VLPF::ApplyRangePartition(vector<string> *range)
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
