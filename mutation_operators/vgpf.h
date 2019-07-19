#ifndef MUSIC_VGPF_H_
#define MUSIC_VGPF_H_

#include "expr_mutant_operator.h"

class VGPF : public ExprMutantOperator
{
public:
	VGPF(const std::string name = "VGPF")
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
	
#endif	// MUSIC_VGPF_H_