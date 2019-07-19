#include "../music_utility.h"
#include "vlsr.h"

bool VLSR::ValidateDomain(const std::set<std::string> &domain)
{
  for (auto e: domain)
    if (!IsValidVariableName(e))
      return false;

  return true;

	// return domain.empty();
}

bool VLSR::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
  //   if (!IsValidVariableName(e))
  //     return false;

  return true;

	// return range.empty();
}

void VLSR::setRange(std::set<std::string> &range)
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
bool VLSR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (!ExprIsScalarReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
  StmtContext &stmt_context = context->getStmtContext();

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

	// VLSR can mutate this expression only if it is a scalar expression
	// inside mutation range and NOT inside array decl size or enum declaration
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         !stmt_context.IsInArrayDeclSize() &&
         !stmt_context.IsInEnumDecl() &&
         stmt_context.IsInCurrentlyParsedFunctionRange(e) &&
         is_in_domain;
}

void VLSR::Mutate(clang::Expr *e, MusicContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

  // cout << start_loc.printToString(context->comp_inst_->getSourceManager()) << endl;
  // cout << e->getLocEnd().printToString(context->comp_inst_->getSourceManager()) << endl;

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  // get all variable declaration that VLSR can mutate this expr to.
  VarDeclList range(
  		(*(context->getSymbolTable()->getLocalScalarVarDeclList()))[context->getFunctionId()]);

  GetRange(e, context, &range);

  vector<string> string_range;

  for (auto vardecl: range)
  {
    string_range.push_back(GetVarDeclName(vardecl));
    // cout << "before range: " << GetVarDeclName(vardecl) << endl;
  }

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

void VLSR::GetRange(Expr *e, MusicContext *context, VarDeclList *range)
{
  SourceLocation start_loc = e->getLocStart();

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  StmtContext &stmt_context = context->getStmtContext();

	// cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = stmt_context.IsInSwitchStmtConditionRange(e) ||
                            stmt_context.IsInArraySubscriptRange(e) ||
                            stmt_context.IsInNonFloatingExprRange(e);

  // cannot mutate a variable in lhs of assignment to a const variable
  bool skip_const_vardecl = stmt_context.IsInLhsOfAssignmentRange(e) ||
  													stmt_context.IsInUnaryIncrementDecrementRange(e);

  bool skip_register_vardecl = stmt_context.IsInAddressOpRange(e);

	// remove all VarDecl appearing after expr, 
  // const/float/register VarDecl (if necessary) and
  // VarDecl not inside user-specified range (if range is not empty)
	for (auto it = range->begin(); it != range->end(); )
	{
		if (!((*it)->getLocStart() < start_loc))
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

  // Remove all VarDecl not inside the same scope as expr E.
	for (auto scope: *(context->scope_list_))
	{
		// all vardecl after expr are removed.
		// No need to consider scopes after expr as well.
		if (LocationBeforeRangeStart(start_loc, scope))
			break;

		if (!LocationIsInRange(start_loc, scope))
			for (auto it = range->begin(); it != range->end();)
			{
        // We are only considering variable inside this scope.
        // The rest of the loop are VarDecl after scope.
				if (LocationAfterRangeEnd((*it)->getLocStart(), scope))
					break;

				// Expr E is not inside this scope so we cannot mutate to any variables
				// declared inside this scope.
				if (LocationIsInRange((*it)->getLocStart(), scope))
				{
					it = range->erase(it);
					continue;
				}

				++it;
			}
	}
}

bool VLSR::HandleRangePartition(string option) 
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

void VLSR::ApplyRangePartition(vector<string> *range)
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
