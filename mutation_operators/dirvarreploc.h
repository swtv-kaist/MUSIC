#ifndef MUSIC_DirVarRepLoc_H_
#define MUSIC_DirVarRepLoc_H_ 

#include "expr_mutant_operator.h"

class DirVarRepLoc : public ExprMutantOperator
{
public:
  DirVarRepLoc(const std::string name = "DirVarRepLoc")
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

#endif  // MUSIC_DirVarRepLoc_H_