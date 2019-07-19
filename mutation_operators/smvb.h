#ifndef MUSIC_SMVB_H_
#define MUSIC_SMVB_H_

#include "stmt_mutant_operator.h"

class SMVB : public StmtMutantOperator
{
public:
  SMVB(const std::string name = "SMVB")
    : StmtMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);
  virtual void setRange(std::set<std::string> &range);
  
  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);

  virtual void Mutate(clang::Stmt *s, MusicContext *context);
};
  
#endif  // MUSIC_SMVB_H_