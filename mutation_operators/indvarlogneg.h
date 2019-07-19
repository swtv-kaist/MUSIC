#ifndef MUSIC_IndVarLogNeg_H_
#define MUSIC_IndVarLogNeg_H_ 

#include "expr_mutant_operator.h"

class IndVarLogNeg : public ExprMutantOperator
{
public:
  IndVarLogNeg(const std::string name = "IndVarLogNeg")
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

#endif  // MUSIC_IndVarLogNeg_H_