#ifndef MUSIC_VLPF_H_
#define MUSIC_VLPF_H_

#include "expr_mutant_operator.h"

class VLPF : public ExprMutantOperator
{
public:
	VLPF(const std::string name = "VLPF")
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
	
#endif	// MUSIC_VLPF_H_