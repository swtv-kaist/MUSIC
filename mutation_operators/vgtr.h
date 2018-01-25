#ifndef MUSIC_VGTR_H_
#define MUSIC_VGTR_H_

#include "expr_mutant_operator.h"

class VGTR : public ExprMutantOperator
{
public:
	VGTR(const std::string name = "VGTR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);
};

#endif	// MUSIC_VGTR_H_