#include "../comut_utility.h"
#include "clcr.h"

#include <algorithm>

bool CLCR::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool CLCR::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

// Return True if the mutant operator can mutate this expression
bool CLCR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!isa<CharacterLiteral>(e) && !isa<FloatingLiteral>(e) &&
			!isa<IntegerLiteral>(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
	StmtContext &stmt_context = context->getStmtContext();

	// CLCR can mutate this constant literal if it is in mutation range,
	// outside array decl range, outside enum decl range, outside
	// field decl range and inside a function (local range)
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInEnumDecl() &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInFieldDeclRange(e) &&
				 stmt_context.IsInCurrentlyParsedFunctionRange(e);
}

void CLCR::Mutate(clang::Expr *e, ComutContext *context)
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
	// Convert char value to int for precise comparison
  if (new_label.front() == '\'' && new_label.back() == '\'')
    new_label = ConvertCharStringToIntString(new_label);

  for (auto case_value: (*switchstmt_list).back().second)
    if (new_label.compare(case_value) == 0)
	    return true;

	return false;
}

bool SortFunction (long long i,long long j) { return (i<j); }

// Do not mutate constants to floating type in this case
// (ptr_cast) <target_constant>
bool IsTargetOfConversionToPointer(Expr *e, ComutContext *context)
{
  const Stmt* parent = GetParentOfStmt(e, context->comp_inst_);

  if (parent)
  {
    // parent->dump();
    // cout << "Is C style cast\n" << endl;
    
    if (const CStyleCastExpr *csce = dyn_cast<CStyleCastExpr>(parent))
      return csce->getTypeAsWritten().getCanonicalType().getTypePtr()->isPointerType();
    
  }

  return true;
}

void CLCR::GetRange(
    Expr *e, ComutContext *context, vector<string> *range)
{
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  // if token is char, then convert to int string for later comparison
  // to avoid mutating to same value constant.
  string int_string{ConvertToString(e, context->comp_inst_->getLangOpts())};

  if (int_string.front() == '\'' && int_string.back() == '\'')
    int_string = ConvertCharStringToIntString(int_string);

  // cannot mutate the variable in switch condition, case value, 
  // array subscript to a floating-type variable because
  // these location requires integral value.
  StmtContext &stmt_context = context->getStmtContext();
  bool skip_float_literal = stmt_context.IsInArraySubscriptRange(e) ||
                            stmt_context.IsInSwitchStmtConditionRange(e) ||
                            stmt_context.IsInSwitchCaseRange(e) ||
                            IsTargetOfConversionToPointer(e, context);

  ExprList local_consts(
      (*(context->getSymbolTable()->getLocalScalarConstantList()))[context->getFunctionId()]);

  for (auto it = local_consts.begin(); it != local_consts.end(); ++it)
  {
    if (skip_float_literal && ExprIsFloat(*it))
      continue;

    string mutated_token{
        ConvertToString(*it, context->comp_inst_->getLangOpts())};

    if (mutated_token.front() == '\'' && mutated_token.back() == '\'')
      mutated_token = ConvertCharStringToIntString(mutated_token);

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
    if (range_.empty() || range_.find(mutated_token) != range_.end())
      range->push_back(mutated_token);
  }

  if (range->empty())
    return;

  if (!(choose_max_ || choose_min_ || choose_median_ ||
        close_less_ || close_more_))
    return;

  // Check if user specify predefined values MAX, MIN, MEDIAN, 
  // CLOSE_LESS, CLOSE_MORE.

  // Convert the strings to long long.
  // Ignore those that cannot be converted.
  vector<long long> range_values;
  for (auto e: *range)
  {
    long long val;
    try 
    {
      val = stoll(e);
    }
    catch(...) { continue; }

    range_values.push_back(val);
  }

  sort(range_values.begin(), range_values.end(), SortFunction);
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
}