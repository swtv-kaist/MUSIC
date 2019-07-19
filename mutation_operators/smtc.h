#ifndef MUSIC_SMTC_H_
#define MUSIC_SMTC_H_ 

#include "stmt_mutant_operator.h"
#include <iostream>

class SMTC : public StmtMutantOperator
{
public:
  SMTC(const std::string name = "SMTC")
    : StmtMutantOperator(name)
  {}

  // option -A and -B are not available for SMTC
  // using them for SMTC terminates MUSIC
  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  virtual void setRange(std::set<std::string> &range);

  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);
  virtual void Mutate(clang::Stmt *s, MusicContext *context);
};

#endif  // MUSIC_SMTC_H_