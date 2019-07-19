#ifndef MUSIC_VGSR_H_
#define MUSIC_VGSR_H_

#include "expr_mutant_operator.h"

class VGSR : public ExprMutantOperator
{
public:
	VGSR(const std::string name = "VGSR")
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

#endif	// MUSIC_VGSR_H_