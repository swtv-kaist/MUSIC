#include "../music_utility.h"
#include "clcr.h"

#include <algorithm>

void CLCR::setRange(std::set<std::string> &range)
{
  for (auto it = range.begin(); it != range.end(); )
  {
    if (it->compare("MAX") == 0 || it->compare("max") == 0)
    {
      choose_max_ = true;
      it = range.erase(it);
    }
    else if (it->compare("MIN") == 0 || it->compare("min") == 0)
    {
      choose_min_ = true;
      it = range.erase(it);
    }
    else if (it->compare("MEDIAN") == 0 || it->compare("median") == 0)
    {
      choose_median_ = true;
      it = range.erase(it);
    }
    else if (it->compare("CLOSE_LESS") == 0 || it->compare("close_less") == 0)
    {
      close_less_ = true;
      it = range.erase(it);
    }
    else if (it->compare("CLOSE_MORE") == 0 || it->compare("close_more") == 0)
    {
      close_more_ = true;
      it = range.erase(it);
    }
    else if (HandleRangePartition(*it))
      it = range.erase(it);
    else
    {
      // cout << *it << endl;
      ++it;
    }
  }

  // for (auto it: partitions)
  //   cout << it << endl;

  range_ = range;
  // exit(1);
}

bool CLCR::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool CLCR::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

// Return True if the mutant operator can mutate this expression
bool CLCR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (!isa<CharacterLiteral>(e) && !isa<FloatingLiteral>(e) &&
			!isa<IntegerLiteral>(e))
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

	// CLCR can mutate this constant literal if it is in mutation range,
	// outside array decl range, outside enum decl range, outside
	// field decl range and inside a function (local range)
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInEnumDecl() &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInFieldDeclRange(e) && is_in_domain &&
				 stmt_context.IsInCurrentlyParsedFunctionRange(e);
}

void CLCR::Mutate(clang::Expr *e, MusicContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  vector<string> range;

  GetRange(e, context, &range);

	for (auto it: range)
	{
    context->mutant_database_.AddMutantEntry(
        name_, start_loc, end_loc, token, it, 
        context->getStmtContext().getProteumStyleLineNum());
	}
}

bool CLCR::IsDuplicateCaseLabel(
		string new_label, SwitchStmtInfoList *switchstmt_list)
{
  for (auto case_value: (*switchstmt_list).back().second)
    if (new_label.compare(case_value) == 0)
	    return true;

	return false;
}

bool CLCRSortFunction (long long i,long long j) { return (i<j); }

// Do not mutate constants to floating type in this case
// (ptr_cast) <target_constant>
bool IsTargetOfConversionToPointer(Expr *e, MusicContext *context)
{
  const Stmt* parent = GetParentOfStmt(e, context->comp_inst_);

  if (parent)
  {
    // parent->dump();
    // cout << "Is C style cast\n" << endl;
    
    if (const CStyleCastExpr *csce = dyn_cast<CStyleCastExpr>(parent))
      return csce->getTypeAsWritten().getCanonicalType().getTypePtr()->isPointerType();
  }

  return false;
}

void CLCR::GetRange(
    Expr *e, MusicContext *context, vector<string> *range)
{
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  // if token is char, then convert to int string for later comparison
  // to avoid mutating to same value constant.
  string int_string{ConvertToString(e, context->comp_inst_->getLangOpts())};

  if (isa<FloatingLiteral>(e))
    ConvertConstFloatExprToFloatString(e, context->comp_inst_, int_string);
  else
    ConvertConstIntExprToIntString(e, context->comp_inst_, int_string);

  // cout << "target is: " << int_string << endl;

  // cannot mutate the variable in switch condition, case value, 
  // array subscript to a floating-type variable because
  // these location requires integral value.
  StmtContext &stmt_context = context->getStmtContext();
  bool skip_float_literal = stmt_context.IsInArraySubscriptRange(e) ||
                            stmt_context.IsInSwitchStmtConditionRange(e) ||
                            stmt_context.IsInSwitchCaseRange(e) ||
                            IsTargetOfConversionToPointer(e, context) ||
                            stmt_context.IsInNonFloatingExprRange(e);

  // cout << stmt_context.IsInArraySubscriptRange(e) << ":" << stmt_context.IsInSwitchStmtConditionRange(e) << ":" << stmt_context.IsInSwitchCaseRange(e) << ":" << IsTargetOfConversionToPointer(e, context) << ":" << stmt_context.IsInNonFloatingExprRange(e) << endl;

  ExprList local_consts(
      (*(context->getSymbolTable()->getLocalScalarConstantList()))[context->getFunctionId()]);

  vector<string> range_int;
  vector<string> range_float;

  // Basic filtering of range: type of constants, range of mutation, ...
  for (auto it = local_consts.begin(); it != local_consts.end(); ++it)
  {
    if (skip_float_literal && ExprIsFloat(*it))
      continue;

    string mutated_token{
        ConvertToString(*it, context->comp_inst_->getLangOpts())};
    string orig_mutated_token{
        ConvertToString(*it, context->comp_inst_->getLangOpts())};

    if (ExprIsFloat(*it))
      ConvertConstFloatExprToFloatString(*it, context->comp_inst_, mutated_token);
    else
      ConvertConstIntExprToIntString(*it, context->comp_inst_, mutated_token);

    if (int_string.compare(mutated_token) == 0)
      continue;

    // Mitigate mutation from causing duplicate-case-label error.
    // If this constant is in range of a case label
    // then check if the replacing token is same with any other label.
    if (stmt_context.IsInSwitchCaseRange(e) &&
        IsDuplicateCaseLabel(mutated_token, context->switchstmt_info_list_))
      continue;

    // If user did not specify range or this mutated token
    // is inside user-specified range, then add it to list 
    // of mutated tokens RANGE for this location
    if (range_.empty() || range_.find(mutated_token) != range_.end() ||
        range_.find(orig_mutated_token) != range_.end())
    {
      range->push_back(mutated_token);

      if (ExprIsFloat(*it))
        range_float.push_back(mutated_token);
      else
        range_int.push_back(mutated_token);
    }
  }

  // if (range->empty())
  //   return;

  if (range_int.empty() && range_float.empty())
    return;

  // for (auto num: range_int)
  //   cout << "int value: " << num << endl;

  // for (auto num: range_float)
  //   cout << "float value: " << num << endl;

  // ==========================================================================
  // =========          Handle special range specification           ==========
  // ==========================================================================
  if (!(choose_max_ || choose_min_ || choose_median_ ||
        close_less_ || close_more_ || partitions.size() != 0))
    return;

  range->clear();

  if (!skip_float_literal && !range_float.empty())
  {
    HandleRangeOptionsWithFloat(int_string, range_int, range_float, range);
    return;
  }

  // Check if user specify predefined values MAX, MIN, MEDIAN, 
  // CLOSE_LESS, CLOSE_MORE.

  // Convert the strings to long long.
  // Ignore those that cannot be converted.
  vector<long long> range_values;
  for (auto num: range_int)
  {
    long long val;
    try 
    {
      val = stoll(num);
    }
    catch(...) { continue; }

    range_values.push_back(val);
  }

  // Sort the list of constants in an increasing order
  sort(range_values.begin(), range_values.end(), CLCRSortFunction);
  range->clear();

  if (choose_max_)
    range->push_back(to_string(range_values.back()));

  if (choose_min_)
    range->push_back(to_string(range_values.front()));

  if (choose_median_)
    range->push_back(to_string(range_values[range_values.size()/2]));

  long long token_value;
  try
  {
    token_value = stoll(int_string);
  }
  catch(...) { return; }

  // If user wants the closest but less than original value,
  // original value must be higher than at least 1 of the mutated token.
  if (close_less_ && token_value > range_values[0])
  {
    int i = 1;
    for (; i < range_values.size(); i++)
      if (range_values[i] > token_value) 
        break;

    range->push_back(to_string(range_values[i-1]));
  }

  if (close_more_ && token_value < range_values.back())
  {
    int i = range_values.size() - 2;
    for (; i >= 0; i--)
      if (range_values[i] < token_value)
        break;

    range->push_back(to_string(range_values[i+1]));
  }

  // for (auto it: range_values)
  //   cout << it << endl;

  if (partitions.size() > 0)
    for (auto part_num: partitions) 
    {
      if (part_num > range_values.size())
      {
        cout << "There are only " << range_values.size() << " to mutate to.\n";
        cout << "No partition number " << part_num << endl;
        continue;
      }

      if (range_values.size() < num_partitions)
      {
        range->push_back(to_string(range_values[part_num-1]));
        continue;
      }

      int start_idx = (range_values.size() / 10) * (part_num - 1);
      int end_idx = (range_values.size() / 10) * part_num;

      if (part_num == 10)
        end_idx = range_values.size();

      for (int idx = start_idx; idx < end_idx; idx++)
        range->push_back(to_string(range_values[idx]));
    }

  // cout << "range is:\n";
  // for (auto e: *range)
  //   cout << e << endl;

  // exit(1);
}

bool CLCRSortFloatFunction (long double i,long double j) { return (i<j); }

void CLCR::MergeListsToStringList(vector<long long> &range_values_int,
                                  vector<long double> &range_values_float,
                                  vector<string> &range_values)
{
  int int_idx = 0;
  int float_idx = 0;

  while (int_idx < range_values_int.size() && 
         float_idx < range_values_float.size())
  {
    if (range_values_int[int_idx] > range_values_float[float_idx])
    {
      stringstream ss;
      ss << range_values_float[float_idx];
      range_values.push_back(ss.str());
      float_idx++;
    }
    else
    {
      range_values.push_back(to_string(range_values_int[int_idx]));
      int_idx++;
    }
  }

  while (int_idx < range_values_int.size())
  {
    range_values.push_back(to_string(range_values_int[int_idx]));
    int_idx++;
  }

  while (float_idx < range_values_float.size())
  {
    range_values.push_back(to_string(range_values_float[float_idx]));
    float_idx++;
  }  
}

void CLCR::HandleRangeOptionsWithFloat(string token_value_str, 
                                       vector<string> &range_int,
                                       vector<string> &range_float,  
                                       vector<string> *range)
{
  vector<long long> range_values_int;
  for (auto num: range_int)
  {
    long long val;
    try 
    {
      val = stoll(num);
    }
    catch(...) { continue; }

    range_values_int.push_back(val);
  }
  sort(range_values_int.begin(), range_values_int.end(), CLCRSortFunction);

  vector<long double> range_values_float;
  for (auto num: range_float)
  {
    long double val;
    try 
    {
      val = stold(num);
    }
    catch(...) { continue; }

    range_values_float.push_back(val);
  }
  sort(range_values_float.begin(), range_values_float.end(), 
       CLCRSortFloatFunction);

  // for (auto num: range_values_int)
  //   cout << "int value: " << num << endl;

  // for (auto num: range_values_float)
  //   cout << "float value: " << num << endl;  

  vector<string> range_values;
  MergeListsToStringList(range_values_int, range_values_float, range_values);

  for (auto num: range_values)
    cout << "merged: " << num << endl; 

  if (choose_max_)
    range->push_back(range_values.back());

  if (choose_min_)
    range->push_back(range_values.front());

  if (choose_median_)
    range->push_back(range_values[range_values.size()/2]);

  if (partitions.size() > 0)
    for (auto part_num: partitions) 
    {
      // Number of possible tokens to mutate to might be smaller than 10.
      // So we do not have 10 partitions.
      if (part_num > range_values.size())
      {
        cout << "There are only " << range_values.size() << " to mutate to.\n";
        cout << "No partition number " << part_num << endl;
        continue;
      }

      if (range_values.size() < num_partitions)
      {
        range->push_back(range_values[part_num-1]);
        continue;
      }

      int start_idx = (range_values.size() / 10) * (part_num - 1);
      int end_idx = (range_values.size() / 10) * part_num;

      if (part_num == 10)
        end_idx = range_values.size();

      for (int idx = start_idx; idx < end_idx; idx++)
        range->push_back(range_values[idx]);
    }

  // Ignoring the fact that target can be integer or float.
  // Convert token value to long double for comparison.
  long double token_value;
  try
  {
    token_value = stold(token_value_str);
  }
  catch(...) { return; }

  if (close_less_ && token_value > stold(range_values[0]))
    for (int i = 1; i <= range_values.size(); i++)
    {
      if (i == range_values.size())
      {
        range->push_back(range_values[i-1]);
        break;
      }

      if (stold(range_values[i]) > token_value)
      {
        range->push_back(range_values[i-1]);
        break;
      }
    }

  if (close_more_ && token_value < stold(range_values.back()))
    for (int i = range_values.size() - 2; i >= 0; i--)
    {
      if (stold(range_values[i]) < token_value)
      {
        range->push_back(range_values[i+1]);
        break;
      }

      if (i == 0)
      {
        range->push_back(range_values[0]);
        break; 
      }
    }

  cout << "range is:\n";
  for (auto e: *range)
    cout << e << endl;
}

bool CLCR::HandleRangePartition(string option) 
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