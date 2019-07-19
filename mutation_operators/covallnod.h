#ifndef MUSIC_CovAllNod_H_
#define MUSIC_CovAllNod_H_

#include "stmt_mutant_operator.h"

class CovAllNod : public StmtMutantOperator
{
public:
  CovAllNod(const std::string name = "CovAllNod")
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
  
#endif  // MUSIC_CovAllNod_H_