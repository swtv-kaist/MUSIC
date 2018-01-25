#ifndef MUSIC_CLSR_H_
#define MUSIC_CLSR_H_	

#include "expr_mutant_operator.h"

class CLSR : public ExprMutantOperator
{
public:
	CLSR(const std::string name = "CLSR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);
};

#endif	// MUSIC_CLSR_H_