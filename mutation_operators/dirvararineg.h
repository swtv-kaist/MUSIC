#ifndef MUSIC_DirVarAriNeg_H_
#define MUSIC_DirVarAriNeg_H_ 

#include "expr_mutant_operator.h"

class DirVarAriNeg : public ExprMutantOperator
{
public:
  DirVarAriNeg(const std::string name = "DirVarAriNeg")
    : ExprMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

  virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
  bool IsDirVar(clang::Expr *e,MusicContext *context);
};

#endif  // MUSIC_DirVarAriNeg_H_