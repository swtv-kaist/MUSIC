#ifndef MUSIC_VGSF_H_
#define MUSIC_VGSF_H_

#include "expr_mutant_operator.h"

class VGSF : public ExprMutantOperator
{
public:
	VGSF(const std::string name = "VGSF")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);
};
	
#endif	// MUSIC_VGSF_H_