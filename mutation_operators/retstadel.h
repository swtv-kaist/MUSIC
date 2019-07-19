#ifndef MUSIC_RetStaDel_H_
#define MUSIC_RetStaDel_H_ 

#include "stmt_mutant_operator.h"
#include <iostream>

class RetStaDel : public StmtMutantOperator
{
public:
  RetStaDel(const std::string name = "RetStaDel")
    : StmtMutantOperator(name)
  {}

  // option -A and -B are not available for RetStaDel
  // using them for RetStaDel terminates MUSIC
  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);
  virtual void Mutate(clang::Stmt *s, MusicContext *context);
};

#endif  // MUSIC_RetStaDel_H_