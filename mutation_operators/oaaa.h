#ifndef MUSIC_OAAA_H_
#define MUSIC_OAAA_H_

#include "expr_mutant_operator.h"

class OAAA : public ExprMutantOperator
{
public:
	OAAA(const std::string name = "OAAA")
		: ExprMutantOperator(name), only_plus_minus_(false)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	virtual void setDomain(std::set<std::string> &domain);
  virtual void setRange(std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	bool only_plus_minus_;

	bool IsMutationTarget(clang::BinaryOperator * const bo);
};

#endif	// MUSIC_OAAA_H_