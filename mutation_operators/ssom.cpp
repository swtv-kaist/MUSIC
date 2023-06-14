#include "../music_utility.h"
#include "ssom.h"

bool SSOM::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool SSOM::ValidateRange(const std::set<std::string> &range)
{
  return true;
}

bool SSOM::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
  {
    if (bo->getOpcode() != BO_Comma)
      return false;

    SourceLocation start_loc = bo->getOperatorLoc();
    SourceManager &src_mgr = context->comp_inst_->getSourceManager();
    SourceLocation end_loc = GetEndLocOfExpr(bo, context->comp_inst_);
    StmtContext &stmt_context = context->getStmtContext();

    // Return True if expr is in mutation range, NOT inside array decl size
    // and NOT inside enum declaration.
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
           !stmt_context.IsInArrayDeclSize() && !stmt_context.IsInEnumDecl() &&
           !stmt_context.IsInTypedefRange(e);
  }

  return false;
}

void SSOM::Mutate(clang::Expr *e, MusicContext *context)
{
  BinaryOperator *bo;
  if (!(bo = dyn_cast<BinaryOperator>(e)))
    return;

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  vector<string> expr_list;
  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(bo, context->comp_inst_);

  // Collect all the expressions of this comma expression
  BinaryOperator *temp_bo = bo;
  Expr *lhs = temp_bo->getLHS()->IgnoreImpCasts();
  while (true)
  {
    Expr *rhs = temp_bo->getRHS();
    string expr_string{ConvertToString(rhs, context->comp_inst_->getLangOpts())};
    expr_list.insert(expr_list.begin(), expr_string);

    lhs = temp_bo->getLHS()->IgnoreImpCasts();
    if (isa<BinaryOperator>(lhs))
    {
      temp_bo = dyn_cast<BinaryOperator>(lhs);
      if (temp_bo->getOpcode() != BO_Comma)
      {
        expr_string = ConvertToString(lhs, context->comp_inst_->getLangOpts());
        expr_list.insert(expr_list.begin(), expr_string);
        break;
      }
    }
    else
    {
      expr_string = ConvertToString(lhs, context->comp_inst_->getLangOpts());
      expr_list.insert(expr_list.begin(), expr_string);
      break;
    }
  }

  int length = expr_list.size();
  for (int i = 1; i < length; i++)
  {
    vector<string> mutated_expr_list;
    for (int j = i; j < i + length; j++)
      mutated_expr_list.push_back(expr_list[j%length]);

    bool redundant = true;
    for (int j = 0; j < length; j++)
      if (expr_list[j].compare(mutated_expr_list[j]) != 0)
      {
        redundant = false;
        break;
      }

    if (redundant)
      continue;

    string mutated_token{""};
    for (int j = 0; j < length; j++)
    {
      mutated_token += mutated_expr_list[j];
      if (j < length - 1)
        mutated_token += ",";
    }

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum());
  }
}

