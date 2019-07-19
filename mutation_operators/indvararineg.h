#ifndef MUSIC_IndVarAriNeg_H_
#define MUSIC_IndVarAriNeg_H_ 

#include "expr_mutant_operator.h"

class IndVarAriNeg : public ExprMutantOperator
{
public:
  IndVarAriNeg(const std::string name = "IndVarAriNeg")
    : ExprMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

  virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
  bool IsIndVar(clang::Expr *e,MusicContext *context);
};

#endif  // MUSIC_IndVarAriNeg_H_