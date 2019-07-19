#ifndef MUSIC_STRP_H_
#define MUSIC_STRP_H_

#include "stmt_mutant_operator.h"

class STRP : public StmtMutantOperator
{
public:
  STRP(const std::string name = "STRP")
    : StmtMutantOperator(name)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);
  virtual void setRange(std::set<std::string> &range);
  
  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);

  virtual void Mutate(clang::Stmt *s, MusicContext *context);

private:
  bool NoUnremovableLabelInsideRange(
      clang::SourceManager &src_mgr, clang::SourceRange range, 
      LabelStmtToGotoStmtListMap *label_map);
};
  
#endif  // MUSIC_STRP_H_