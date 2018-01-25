#ifndef MUSIC_VGPR_H_
#define MUSIC_VGPR_H_

#include "expr_mutant_operator.h"

class VGPR : public ExprMutantOperator
{
public:
	VGPR(const std::string name = "VGPR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);
};

#endif	// MUSIC_VGPR_H_