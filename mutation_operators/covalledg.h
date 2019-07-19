#ifndef MUSIC_CovAllEdg_H_
#define MUSIC_CovAllEdg_H_ 

#include "stmt_mutant_operator.h"

class CovAllEdg : public StmtMutantOperator
{
public:
  CovAllEdg(const std::string name = "CovAllEdg")
    : StmtMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  // Return True if the mutant operator can mutate this statement
  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);

  virtual void Mutate(clang::Stmt *s, MusicContext *context);

private:
  void HandleConditionalStmt(clang::Stmt *s, MusicContext *context);
  void HandleSwitchStmt(clang::Stmt *s, MusicContext *context);
};

#endif  // MUSIC_CovAllEdg_H_