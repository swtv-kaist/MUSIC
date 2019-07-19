#ifndef MUSIC_SMTT_H_
#define MUSIC_SMTT_H_ 

#include "stmt_mutant_operator.h"
#include <iostream>

class SMTT : public StmtMutantOperator
{
public:
  SMTT(const std::string name = "SMTT")
    : StmtMutantOperator(name)
  {}

  // option -A and -B are not available for SMTT
  // using them for SMTT terminates MUSIC
  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  virtual void setRange(std::set<std::string> &range);

  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);
  virtual void Mutate(clang::Stmt *s, MusicContext *context);
};

#endif  // MUSIC_SMTT_H_