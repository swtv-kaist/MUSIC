#include "../music_utility.h"
#include "rrcr.h"

extern set<string> arith_assignment_operators;
extern set<string> bitwise_assignment_operators;
extern set<string> shift_assignment_operators;
extern set<string> assignment_operator;

bool RRCR::ValidateDomain(const std::set<std::string> &domain)
{
  return domain.empty();
}

bool RRCR::ValidateRange(const std::set<std::string> &range)
{
  for (auto num: range)
    if (!StringIsANumber(num))
      return false;

  return true;
}

void RRCR::setRange(std::set<std::string> &range)
{
  range_integral_ = {"0", "1", "-1", "min", "max"};
  range_float_ = {"0.0", "1.0", "-1.0", "min", "max"};

  for (auto num: range)
    if (NumIsFloat(num))
      range_float_.insert(num);
    else
      range_integral_.insert(num);
}


// Return True if the mutant operator can mutate this expression
bool RRCR::IsMutationTarget(clang::Expr *e, MusicContext *context)
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

    // cout << "checking if MUSIC can mutate " << token << endl;

    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
           !stmt_context.IsInEnumDecl() && ExprIsScalar(rhs);
  }

  return false;
}

bool RRCR::IsInitMutationTarget(clang::Expr *e, MusicContext *context)
{
  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
  StmtContext& stmt_context = context->getStmtContext();
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  
  // cout << "checking if MUSIC can mutate " << token << endl;

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         !stmt_context.IsInEnumDecl() && ExprIsScalar(e);
}

void RRCR::Mutate(clang::Expr *e, MusicContext *context)
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
  
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  string token{ConvertToString(rhs, context->comp_inst_->getLangOpts())};
  SourceLocation start_loc = rhs->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(rhs, context->comp_inst_);

  if (ExprIsIntegral(context->comp_inst_, rhs))
  {
    for (auto num: range_integral_)
    {
      string mutated_token{"(" + num + ")"};

      if (num.compare("min") == 0)
      {
        mutated_token = GetMinValue(rhs->getType());
        context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum(), 
          "min");
        continue;
      }

      if (num.compare("max") == 0)
      {
        mutated_token = GetMaxValue(rhs->getType());
        context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum(), 
          "max");
        continue;
      }

      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum(), num);
    }

    return;
  }

  if (ExprIsFloat(rhs))
  {
    for (auto num: range_float_)
    {
      string mutated_token{"(" + num + ")"};
      string info{""};

      if (num.compare("min") == 0)
      {
        mutated_token = GetMinValue(rhs->getType());
        info = "min";
      }

      if (num.compare("max") == 0)
      {
        mutated_token = GetMaxValue(rhs->getType());
        info = "max";       
      }

      if (num.compare("0.0") == 0)
        info = "0";
      if (num.compare("1.0") == 0)
        info = "1";
      if (num.compare("-1.0") == 0)
        info = "-1";

      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum(), info);
    }

    return;
  }
}
