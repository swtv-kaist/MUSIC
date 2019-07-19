#ifndef MUSIC_SGLR_H_
#define MUSIC_SGLR_H_

#include "stmt_mutant_operator.h"

class SGLR : public StmtMutantOperator
{
public:
  SGLR(const std::string name = "SGLR")
    : StmtMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);
  virtual void setRange(std::set<std::string> &range);
  
  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);

  virtual void Mutate(clang::Stmt *s, MusicContext *context);
};
  
#endif  // MUSIC_SGLR_H_