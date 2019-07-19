#ifndef MUSIC_IndVarBitNeg_H_
#define MUSIC_IndVarBitNeg_H_ 

#include "expr_mutant_operator.h"

class IndVarBitNeg : public ExprMutantOperator
{
public:
  IndVarBitNeg(const std::string name = "IndVarBitNeg")
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

#endif  // MUSIC_IndVarBitNeg_H_