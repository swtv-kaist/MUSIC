#include "../music_utility.h"
#include "rlar.h"

extern set<string> arith_assignment_operators;
extern set<string> bitwise_assignment_operators;
extern set<string> shift_assignment_operators;
extern set<string> assignment_operator;

bool RLAR::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool RLAR::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void RLAR::setRange(std::set<std::string> &range)
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
bool RLAR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
  {
    // cout << "RGCR checking a binary operator" << endl;
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

    // Second, RHS expression must be scalar
    Expr *rhs = bo->getRHS()->IgnoreImpCasts();

    // RHS needs to be inside mutation range, outside enum declaration,
    // and inside user-specified domain (if available)
    SourceLocation start_loc = rhs->getBeginLoc();
    SourceLocation end_loc = GetEndLocOfExpr(rhs, context->comp_inst_);
    StmtContext& stmt_context = context->getStmtContext();
    string token{ConvertToString(rhs, context->comp_inst_->getLangOpts())};
    bool is_in_domain = domain_.empty() ? true : 
                        IsStringElementOfSet(token, domain_);

    // cout << "checking if MUSIC can mutate " << token << endl;

    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
           !stmt_context.IsInEnumDecl() && is_in_domain && ExprIsArray(rhs) &&
           stmt_context.IsInCurrentlyParsedFunctionRange(rhs);
  }

  return false;
}

bool RLAR::IsInitMutationTarget(clang::Expr *e, MusicContext *context)
{
  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
  StmtContext& stmt_context = context->getStmtContext();
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

  // cout << "checking if MUSIC can mutate " << token << endl;

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         !stmt_context.IsInEnumDecl() && is_in_domain && ExprIsArray(e) &&
         stmt_context.IsInCurrentlyParsedFunctionRange(e);
}

void RLAR::Mutate(clang::Expr *e, MusicContext *context)
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

  string token{ConvertToString(rhs, context->comp_inst_->getLangOpts())};

  // get all variable declaration that RLAR can mutate this expr to.
  VarDeclList range(
      (*(context->getSymbolTable()->getLocalArrayVarDeclList()))[context->getFunctionId()]);

  GetRange(rhs, context, &range);

  vector<string> string_range;

  for (auto vardecl: range)
  {
    string mutated_token{GetVarDeclName(vardecl)};
    
    if (sameArrayElementType(rhs->getType(), vardecl->getType()))
    {
      string_range.push_back(mutated_token);
    }
  }

  // for (auto it: string_range)
  //   cout << "before range: " << it << endl;

  if (partitions.size() > 0)
    ApplyRangePartition(&string_range);

  // for (auto it: string_range)
  //   cout << "after range: " << it << endl;

  for (auto it: string_range)
    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, it, 
        context->getStmtContext().getProteumStyleLineNum());
}

void RLAR::GetRange(Expr *e, MusicContext *context, VarDeclList *range)
{
  SourceLocation start_loc = e->getBeginLoc();

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  StmtContext &stmt_context = context->getStmtContext();

  // remove all vardecl appear after expr
  for (auto it = range->begin(); it != range->end(); )
  {
    if (!((*it)->getBeginLoc() < start_loc))
    {
      range->erase(it, range->end());
      break;
    }

    bool is_in_range = range_.empty() ? true :
                       IsStringElementOfSet(GetVarDeclName(*it), range_);

    if ((token.compare(GetVarDeclName(*it)) == 0) ||
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

bool RLAR::HandleRangePartition(string option) 
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

void RLAR::ApplyRangePartition(vector<string> *range)
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
