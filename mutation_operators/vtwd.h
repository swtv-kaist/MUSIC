#ifndef MUSIC_VTWD_H_
#define MUSIC_VTWD_H_ 

#include "expr_mutant_operator.h"

class VTWD : public ExprMutantOperator
{
public:
	VTWD(const std::string name = "VTWD")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	bool IsMutationTarget(std::string scalarref_name, MusicContext *context);
};

#endif	// MUSIC_VTWD_H_