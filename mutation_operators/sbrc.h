#ifndef MUSIC_SBRC_H_
#define MUSIC_SBRC_H_ 

#include "stmt_mutant_operator.h"
#include <iostream>

class SBRC : public StmtMutantOperator
{
public:
  SBRC(const std::string name = "SBRC")
    : StmtMutantOperator(name)
  {}

  // option -A and -B are not available for SBRC
  // using them for SBRC terminates MUSIC
  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);
  virtual void Mutate(clang::Stmt *s, MusicContext *context);
};

#endif  // MUSIC_SBRC_H_