#ifndef MUSIC_OSSN_H_
#define MUSIC_OSSN_H_

#include "expr_mutant_operator.h"

class OSSN : public ExprMutantOperator
{
public:
	OSSN(const std::string name = "OSSN")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	virtual void setDomain(std::set<std::string> &domain);
  virtual void setRange(std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);
};

#endif	// MUSIC_OSSN_H_