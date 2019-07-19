#ifndef MUSIC_ArgIncDec_H_
#define MUSIC_ArgIncDec_H_

#include "expr_mutant_operator.h"

class ArgIncDec : public ExprMutantOperator
{
public:
  ArgIncDec(const std::string name = "ArgIncDec")
    : ExprMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);
  virtual void setRange(std::set<std::string> &range);
  
  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

  virtual void Mutate(clang::Expr *e, MusicContext *context);
};
  
#endif  // MUSIC_ArgIncDec_H_