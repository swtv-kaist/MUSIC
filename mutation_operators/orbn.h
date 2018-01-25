#ifndef MUSIC_ORBN_H_
#define MUSIC_ORBN_H_

#include "expr_mutant_operator.h"

class ORBN : public ExprMutantOperator
{
public:
	ORBN(const std::string name = "ORBN")
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

#endif	// MUSIC_ORBN_H_