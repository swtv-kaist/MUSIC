#ifndef MUSIC_STRI_H_
#define MUSIC_STRI_H_ 

#include "stmt_mutant_operator.h"

class STRI : public StmtMutantOperator
{
public:
  STRI(const std::string name = "STRI")
    : StmtMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  // Return True if the mutant operator can mutate this statement
  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);

  virtual void Mutate(clang::Stmt *s, MusicContext *context);
};

#endif  // MUSIC_STRI_H_