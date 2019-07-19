#ifndef MUSIC_OLNG_H_
#define MUSIC_OLNG_H_	

#include "expr_mutant_operator.h"

class OLNG : public ExprMutantOperator
{
public:
	OLNG(const std::string name = "OLNG")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	void GenerateMutantByNegation(Expr *e, MusicContext *context, std::string side);
};

#endif	// MUSIC_OLNG_H_