#ifndef MUSIC_OPPO_H_
#define MUSIC_OPPO_H_	

#include "expr_mutant_operator.h"

class OPPO : public ExprMutantOperator
{
public:
	OPPO(const std::string name = "OPPO")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	void GenerateMutantForPostInc(UnaryOperator *uo, MusicContext *context);
	void GenerateMutantForPreInc(UnaryOperator *uo, MusicContext *context);
};

#endif	// MUSIC_OPPO_H_