#ifndef MUSIC_RRCR_H_
#define MUSIC_RRCR_H_

#include "expr_mutant_operator.h"

class RRCR : public ExprMutantOperator
{
private:
  std::set<std::string> range_integral_;
  std::set<std::string> range_float_;

  // std::string GetMaxValue(clang::QualType qualtype);
  // std::string GetMinValue(clang::QualType qualtype);

public:
  RRCR(const std::string name = "RRCR")
    : ExprMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  virtual void setRange(std::set<std::string> &range);

  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

  virtual void Mutate(clang::Expr *e, MusicContext *context);

  bool IsInitMutationTarget(clang::Expr *e, MusicContext *context);
};

#endif  // MUSIC_RRCR_H_