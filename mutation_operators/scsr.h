#ifndef MUSIC_SCSR_H_
#define MUSIC_SCSR_H_

#include "expr_mutant_operator.h"

class SCSR : public ExprMutantOperator
{
public:
	SCSR(const std::string name = "SCSR")
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

	void GenerateGlobalMutants(Expr *e, MusicContext *context,
														 set<string> *stringCache);
	void GenerateLocalMutants(Expr *e, MusicContext *context,
													  set<string> *stringCache);
	void GetRange(clang::Expr *e, MusicContext *context, 
                std::vector<std::string> *range);
	bool HandleRangePartition(std::string option);
	void ApplyRangePartition(std::vector<std::string> *range);
};

#endif	// MUSIC_SCSR_H_