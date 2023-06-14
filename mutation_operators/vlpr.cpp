#include "../music_utility.h"
#include "vlpr.h"

bool VLPR::ValidateDomain(const std::set<std::string> &domain)
{
	for (auto e: domain)
    if (!IsValidVariableName(e))
      return false;

  return true;
}

bool VLPR::ValidateRange(const std::set<std::string> &range)
{
	// for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void VLPR::setRange(std::set<std::string> &range)
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
bool VLPR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (!ExprIsPointerReference(e))
		return false;

	SourceLocation start_loc = e->getBeginLoc();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
  StmtContext &stmt_context = context->getStmtContext();

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

	// VLPR can mutate this expression only if it is a pointer expression
	// inside mutation range and NOT inside array decl size or enum declaration
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         !stmt_context.IsInArrayDeclSize() &&
         !stmt_context.IsInEnumDecl() &&
         stmt_context.IsInCurrentlyParsedFunctionRange(e) &&
         is_in_domain;
}

void VLPR::Mutate(clang::Expr *e, MusicContext *context)
{
	SourceLocation start_loc = e->getBeginLoc();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

	// get all variable declaration that VLPR can mutate this expr to.
  VarDeclList range(
      (*(context->getSymbolTable()->getLocalPointerVarDeclList()))[context->getFunctionId()]);

  GetRange(e, context, &range);

  string pointee_type = getPointerType(e->getType());
  vector<string> string_range;

  for (auto vardecl: range)
  {
    string mutated_token{GetVarDeclName(vardecl)};

    if (pointee_type.compare(getPointerType(vardecl->getType())) == 0)
    {
      // cout << mutated_token << endl;
      // PrintLocation(src_mgr, vardecl->getEndLoc());

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

void VLPR::GetRange(Expr *e, MusicContext *context, VarDeclList *range)
{
  SourceLocation start_loc = e->getBeginLoc();
  Rewriter rewriter;
  rewriter.setSourceMgr(context->comp_inst_->getSourceManager(), 
                        context->comp_inst_->getLangOpts());

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  StmtContext &stmt_context = context->getStmtContext();

  // cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = stmt_context.IsInSwitchStmtConditionRange(e);

  // cannot mutate a variable in lhs of assignment to a const variable
  bool skip_const_vardecl = stmt_context.IsInLhsOfAssignmentRange(e);

  bool skip_register_vardecl = stmt_context.IsInAddressOpRange(e);

  // remove all vardecl appear after expr
  for (auto it = range->begin(); it != range->end(); )
  {
    if (!((*it)->getBeginLoc() < start_loc &&
          ((*it)->getEndLoc() < start_loc || (*it)->getEndLoc() == start_loc)))
    {
      range->erase(it, range->end());
      break;
    }

    bool is_in_range = range_.empty() ? true :
                       IsStringElementOfSet(GetVarDeclName(*it), range_);

    if ((skip_const_vardecl && IsVarDeclConst((*it))) ||
        (skip_float_vardecl && IsVarDeclFloating((*it))) ||
        (skip_register_vardecl && 
          (*it)->getStorageClass() == SC_Register) ||
        (token.compare(GetVarDeclName(*it)) == 0) ||
        !is_in_range)
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

bool VLPR::HandleRangePartition(string option) 
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

void VLPR::ApplyRangePartition(vector<string> *range)
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
