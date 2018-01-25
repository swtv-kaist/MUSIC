#ifndef MUSIC_VGTF_H_
#define MUSIC_VGTF_H_

#include "expr_mutant_operator.h"

class VGTF : public ExprMutantOperator
{
public:
	VGTF(const std::string name = "VGTF")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);
};
	
#endif	// MUSIC_VGTF_H_