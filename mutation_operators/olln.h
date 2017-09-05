#ifndef COMUT_OLLN_H_
#define COMUT_OLLN_H_

#include "expr_mutant_operator.h"

class OLLN : public ExprMutantOperator
{
public:
	OLLN(const std::string name = "OLLN")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	virtual void setDomain(std::set<std::string> &domain);
  virtual void setRange(std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);
};

#endif	// COMUT_OLLN_H_