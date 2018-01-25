#ifndef MUSIC_VLAR_H_
#define MUSIC_VLAR_H_

#include "expr_mutant_operator.h"

class VLAR : public ExprMutantOperator
{
public:
	VLAR(const std::string name = "VLAR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);
private:
	void GetRange(clang::Expr *e, MusicContext *context, VarDeclList *range);
};

#endif	// MUSIC_VLAR_H_