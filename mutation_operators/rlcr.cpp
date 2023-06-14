#include "../music_utility.h"
#include "rlcr.h"
#include <algorithm>

extern set<string> arith_assignment_operators;
extern set<string> bitwise_assignment_operators;
extern set<string> shift_assignment_operators;
extern set<string> assignment_operator;

void RLCR::setRange(std::set<std::string> &range)
{
  for (auto it = range.begin(); it != range.end(); )
  {
    if (HandleRangePartition(*it))
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

bool RLCR::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool RLCR::ValidateRange(const std::set<std::string> &range)
{
  return true;
}

// Return True if the mutant operator can mutate this expression
bool RLCR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
  {
    // cout << "RLCR checking a binary operator" << endl;

    // First, check if the operator is an assignment operator
    string binary_operator{bo->getOpcodeStr()};

    if (arith_assignment_operators.find(binary_operator) == 
            arith_assignment_operators.end() &&
        bitwise_assignment_operators.find(binary_operator) ==
            bitwise_assignment_operators.end() &&
        shift_assignment_operators.find(binary_operator) ==
            shift_assignment_operators.end() &&
        assignment_operator.find(binary_operator) == assignment_operator.end())
      return false;

    // Second, RHS expression must be scalar, in mutation range, 
    // outside enum declaration, outside field declaration range and
    // in domain (if user specified) and inside a function (local range)
    Expr *rhs = bo->getRHS()->IgnoreImpCasts();
    SourceLocation start_loc = rhs->getBeginLoc();
    SourceLocation end_loc = GetEndLocOfExpr(rhs, context->comp_inst_);
    StmtContext& stmt_context = context->getStmtContext();
    string token{ConvertToString(rhs, context->comp_inst_->getLangOpts())};
    bool is_in_domain = domain_.empty() ? true : 
                        IsStringElementOfSet(token, domain_);

    // cout << "checking if MUSIC can mutate " << token << endl;

    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
           ExprIsScalar(rhs) && !stmt_context.IsInEnumDecl() &&
           !stmt_context.IsInFieldDeclRange(rhs) && is_in_domain &&
           stmt_context.IsInCurrentlyParsedFunctionRange(rhs);
  }

  return false;
}

bool RLCR::IsInitMutationTarget(clang::Expr *e, MusicContext *context)
{
  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
  StmtContext& stmt_context = context->getStmtContext();
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

  // cout << "checking if MUSIC can mutate " << token << endl;

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         ExprIsScalar(e) && !stmt_context.IsInEnumDecl() && 
         !stmt_context.IsInFieldDeclRange(e) && is_in_domain &&
         stmt_context.IsInCurrentlyParsedFunctionRange(e);
}

void RLCR::Mutate(clang::Expr *e, MusicContext *context)
{
  BinaryOperator *bo;
  Expr *rhs;

  if (!(bo = dyn_cast<BinaryOperator>(e)))
    rhs = e;
  else
  {
    string binary_operator{bo->getOpcodeStr()};

    if (arith_assignment_operators.find(binary_operator) == 
            arith_assignment_operators.end() &&
        bitwise_assignment_operators.find(binary_operator) ==
            bitwise_assignment_operators.end() &&
        shift_assignment_operators.find(binary_operator) ==
            shift_assignment_operators.end() &&
        assignment_operator.find(binary_operator) == assignment_operator.end())
      rhs = e;
    else
      rhs = bo->getRHS()->IgnoreImpCasts();
  }

  SourceLocation start_loc = rhs->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(rhs, context->comp_inst_);

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();

  string token{ConvertToString(rhs, context->comp_inst_->getLangOpts())};

  vector<string> range;

  GetRange(rhs, context, &range);

  for (auto it: range)
  {
    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, it, 
        context->getStmtContext().getProteumStyleLineNum());
  }
}

bool RLCR::IsDuplicateCaseLabel(
    string new_label, SwitchStmtInfoList *switchstmt_list)
{
  for (auto case_value: (*switchstmt_list).back().second)
    if (new_label.compare(case_value) == 0)
      return true;

  return false;
}

bool RLCRSortFunction (long long i,long long j) { return (i<j); }
bool RLCRSortFloatFunction (long double i,long double j) { return (i<j); }

void RLCR::GetRange(
    Expr *e, MusicContext *context, vector<string> *range)
{
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  // cannot mutate the variable in switch condition, case value, 
  // array subscript to a floating-type variable because
  // these location requires integral value.
  StmtContext &stmt_context = context->getStmtContext();
  bool skip_float_literal = stmt_context.IsInNonFloatingExprRange(e);

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

    if (token.compare(mutated_token) == 0 ||
        token.compare(orig_mutated_token) == 0)
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

  if (!(partitions.size() != 0))
    return;

  range->clear();

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
  sort(range_values_int.begin(), range_values_int.end(), RLCRSortFunction);

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
       RLCRSortFloatFunction);

  // for (auto num: range_values_int)
  //   cout << "int value: " << num << endl;

  // for (auto num: range_values_float)
  //   cout << "float value: " << num << endl;  

  vector<string> range_values;
  MergeListsToStringList(range_values_int, range_values_float, range_values);

  for (auto num: range_values)
    cout << "merged: " << num << endl; 

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

  // cout << "range is:\n";
  // for (auto e: *range)
  //   cout << e << endl;
}

void RLCR::MergeListsToStringList(vector<long long> &range_values_int,
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

bool RLCR::HandleRangePartition(string option) 
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