#ifndef MUSIC_ORRN_H_
#define MUSIC_ORRN_H_

#include "expr_mutant_operator.h"

class ORRN : public ExprMutantOperator
{
public:
	ORRN(const std::string name = "ORRN")
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

#endif	// MUSIC_ORRN_H_