#ifndef MUSIC_SANL_H_
#define MUSIC_SANL_H_ 

#include "expr_mutant_operator.h"

class SANL : public ExprMutantOperator
{
public:
	SANL(const std::string name = "SANL")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);
};

#endif	// MUSIC_SANL_H_