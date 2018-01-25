#ifndef MUSIC_ORSN_H_
#define MUSIC_ORSN_H_

#include "expr_mutant_operator.h"

class ORSN : public ExprMutantOperator
{
public:
	ORSN(const std::string name = "ORSN")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	virtual void setDomain(std::set<std::string> &domain);
  virtual void setRange(std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	bool IsMutationTarget(BinaryOperator *bo, string mutated_token,
								 MusicContext *context);
};

#endif	// MUSIC_ORSN_H_