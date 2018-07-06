#ifndef MUSIC_CGSR_H_
#define MUSIC_CGSR_H_	

#include "expr_mutant_operator.h"

class CGSR : public ExprMutantOperator
{
public:
	CGSR(const std::string name = "CGSR")
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

  void GetRange(clang::Expr *e, MusicContext *context, 
                std::vector<std::string> *range);

  bool HandleRangePartition(std::string option);

  void MergeListsToStringList(std::vector<long long> &range_values_int,
                              std::vector<long double> &range_values_float,
                              std::vector<std::string> &range_values);
};

#endif	// MUSIC_CGSR_H_