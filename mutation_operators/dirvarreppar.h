#ifndef MUSIC_DirVarRepPar_H_
#define MUSIC_DirVarRepPar_H_ 

#include "expr_mutant_operator.h"

class DirVarRepPar : public ExprMutantOperator
{
public:
  DirVarRepPar(const std::string name = "DirVarRepPar")
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

#endif  // MUSIC_DirVarRepPar_H_