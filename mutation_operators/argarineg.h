#ifndef MUSIC_ArgAriNeg_H_
#define MUSIC_ArgAriNeg_H_

#include "expr_mutant_operator.h"

class ArgAriNeg : public ExprMutantOperator
{
public:
  ArgAriNeg(const std::string name = "ArgAriNeg")
    : ExprMutantOperator(name), num_partitions(10)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);
  virtual void setRange(std::set<std::string> &range);
  
  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

  virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
  std::set<int> partitions;
  int num_partitions;

  bool HandleRangePartition(std::string option);
  void ApplyRangePartition(std::vector<std::string> *range);
};
  
#endif  // MUSIC_ArgAriNeg_H_