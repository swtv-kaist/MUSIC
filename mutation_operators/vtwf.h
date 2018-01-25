#ifndef MUSIC_VTWF_H_
#define MUSIC_VTWF_H_

#include "expr_mutant_operator.h"

class VTWF : public ExprMutantOperator
{
public:
	VTWF(const std::string name = "VTWF")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);
};

#endif	// MUSIC_VTWF_H_