#ifndef MUSIC_SCRB_H_
#define MUSIC_SCRB_H_ 

#include "stmt_mutant_operator.h"
#include <iostream>

class SCRB : public StmtMutantOperator
{
public:
  SCRB(const std::string name = "SCRB")
    : StmtMutantOperator(name)
  {}

  // option -A and -B are not available for SCRB
  // using them for SCRB terminates MUSIC
  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);
  virtual void Mutate(clang::Stmt *s, MusicContext *context);
};

#endif  // MUSIC_SCRB_H_