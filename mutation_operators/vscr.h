#ifndef MUSIC_VSCR_H_
#define MUSIC_VSCR_H_

#include "expr_mutant_operator.h"

class VSCR : public ExprMutantOperator
{
public:
	VSCR(const std::string name = "VSCR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	bool IsSameType(const QualType type1, const QualType type2);
};

#endif	// MUSIC_VSCR_H_