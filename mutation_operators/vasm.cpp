#include "../music_utility.h"
#include "vasm.h"

bool VASM::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool VASM::ValidateRange(const std::set<std::string> &range)
{
  return true;
}

bool VASM::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  if (ArraySubscriptExpr *ase = dyn_cast<ArraySubscriptExpr>(e))
  {
    // Only mutate full ArraySubscriptExpr
    // const Stmt* parent = GetParentOfStmt(s, context->comp_inst_);
    // if (!parent)
    //   return false;
    // if (isa<ArraySubscriptExpr>(parent))
    //   return false;

    // If the expression is only arr[idx] then VASM cannot be applied.
    Expr *base = ase->getBase()->IgnoreImpCasts();
    if (!isa<ArraySubscriptExpr>(base))
      return false;

    SourceLocation start_loc = e->getBeginLoc();
    SourceLocation end_loc = GetEndLocOfExpr(ase, context->comp_inst_);

    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
  }

  return false;
}

void VASM::Mutate(clang::Expr *e, MusicContext *context)
{
  ArraySubscriptExpr *ase;
  if (!(ase = dyn_cast<ArraySubscriptExpr>(e)))
    return;

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  vector<string> idx_list;
  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfExpr(ase, context->comp_inst_);

  // Collect all the index of this ArraySubscriptExpr
  ArraySubscriptExpr *temp_ase = ase;
  Expr *base = temp_ase->getBase()->IgnoreImpCasts();
  while (true)
  {
    Expr *idx = temp_ase->getIdx();
    string idx_string{ConvertToString(idx, context->comp_inst_->getLangOpts())};
    idx_list.insert(idx_list.begin(), idx_string);

    base = temp_ase->getBase()->IgnoreImpCasts();
    if (isa<ArraySubscriptExpr>(base))
      temp_ase = dyn_cast<ArraySubscriptExpr>(base);
    else
      break;
  }

  int length = idx_list.size();
  for (int i = 1; i < length; i++)
  {
    vector<string> mutated_idx_list;
    for (int j = i; j < i + length; j++)
      mutated_idx_list.push_back(idx_list[j%length]);

    bool redundant = true;
    for (int j = 0; j < length; j++)
      if (idx_list[j].compare(mutated_idx_list[j]) != 0)
      {
        redundant = false;
        break;
      }

    if (redundant)
      continue;

    string mutated_token{ConvertToString(base, 
                                         context->comp_inst_->getLangOpts())};
    for (auto e: mutated_idx_list)
      mutated_token += ("[" + e + "]");

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum());
  }
}

