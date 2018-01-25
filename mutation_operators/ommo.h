#ifndef MUSIC_OMMO_H_
#define MUSIC_OMMO_H_	

#include "expr_mutant_operator.h"

class OMMO : public ExprMutantOperator
{
public:
	OMMO(const std::string name = "OMMO")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	void GenerateMutantForPostDec(UnaryOperator *uo, MusicContext *context);
	void GenerateMutantForPreDec(UnaryOperator *uo, MusicContext *context);
};

#endif	// MUSIC_OMMO_H_