#ifndef MUSIC_SSOM_H_
#define MUSIC_SSOM_H_

#include "expr_mutant_operator.h"

class SSOM : public ExprMutantOperator
{
public:
  SSOM(const std::string name = "SSOM")
    : ExprMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  // virtual void setDomain(std::set<std::string> &domain);
  // virtual void setRange(std::set<std::string> &range);

  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

  virtual void Mutate(clang::Expr *e, MusicContext *context);
};

#endif  // MUSIC_SSOM_H_