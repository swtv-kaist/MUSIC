#include "../music_utility.h"
#include "cgcr.h"

void CGCR::setRange(std::set<std::string> &range)
{
  for (auto it = range.begin(); it != range.end(); )
  {
    if (it->compare("MAX") == 0)
    {
      choose_max_ = true;
      it = range.erase(it);
    }
    else if (it->compare("MIN") == 0)
    {
      choose_min_ = true;
      it = range.erase(it);
    }
    else if (it->compare("MEDIAN") == 0)
    {
      choose_median_ = true;
      it = range.erase(it);
    }
    else if (it->compare("CLOSE_LESS") == 0)
    {
      close_less_ = true;
      it = range.erase(it);
    }
    else if (it->compare("CLOSE_MORE") == 0)
    {
      close_more_ = true;
      it = range.erase(it);
    }
    else
      ++it;
  }

  range_ = range;
}

bool CGCR::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool CGCR::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

// Return True if the mutant operator can mutate this expression
bool CGCR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (!isa<CharacterLiteral>(e) && !isa<FloatingLiteral>(e) &&
			!isa<IntegerLiteral>(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
	StmtContext& stmt_context = context->getStmtContext();

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

	// CGCR can mutate this constant literal if it is in mutation range,
	// outside array decl range, outside enum decl range and outside
	// field decl range.
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInEnumDecl() &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInFieldDeclRange(e) && is_in_domain;
}

void CGCR::Mutate(clang::Expr *e, MusicContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  vector<string> range;
  GetRange(e, context, &range);

  for (auto it: range)
  	context->mutant_database_.AddMutantEntry(
        name_, start_loc, end_loc, token, it, 
        context->getStmtContext().getProteumStyleLineNum());
}

bool CGCR::IsDuplicateCaseLabel(
		string new_label, SwitchStmtInfoList *switchstmt_list)
{
  for (auto case_value: (*switchstmt_list).back().second)
    if (new_label.compare(case_value) == 0)
	    return true;

	return false;
}

bool CGCRSortFunction (long long i,long long j) { return (i<j); }

void CGCR::GetRange(
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

  // cannot mutate the variable in switch condition, case value, 
  // array subscript to a floating-type variable because
  // these location requires integral value.
  StmtContext &stmt_context = context->getStmtContext();
  bool skip_float_literal = stmt_context.IsInArraySubscriptRange(e) ||
                            stmt_context.IsInSwitchStmtConditionRange(e) ||
                            stmt_context.IsInSwitchCaseRange(e) ||
                            stmt_context.IsInNonFloatingExprRange(e);

  ExprList global_consts(
  		*(context->getSymbolTable()->getGlobalScalarConstantList()));

  for (auto it: global_consts)
  {
  	if (skip_float_literal && ExprIsFloat(it))
      continue;

    string mutated_token{
        ConvertToString(it, context->comp_inst_->getLangOpts())};
    string orig_mutated_token{
        ConvertToString(it, context->comp_inst_->getLangOpts())};

    if (ExprIsFloat(it))
      ConvertConstFloatExprToFloatString(it, context->comp_inst_, mutated_token);
    else
      ConvertConstIntExprToIntString(it, context->comp_inst_, mutated_token);

	  // Avoid mutating to the same scalar constant
    // If token is char, then convert it to int string for comparison
    if (int_string.compare(mutated_token) == 0)
    	continue;

    // Mitigate mutation from causing duplicate-case-label error.
    // If this constant is in range of a case label
    // then check if the replacing token is same with any other label.
    if (stmt_context.IsInSwitchCaseRange(e) &&
    		IsDuplicateCaseLabel(mutated_token, context->switchstmt_info_list_))
    	continue;

    if (range_.empty() || range_.find(mutated_token) != range_.end() ||
        range_.find(orig_mutated_token) != range_.end())
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
  for (auto num: *range)
  {
    long long val;
    try 
    {
      val = stoll(num);
    }
    catch(...) { continue; }

    range_values.push_back(val);
  }

  sort(range_values.begin(), range_values.end(), CGCRSortFunction);
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