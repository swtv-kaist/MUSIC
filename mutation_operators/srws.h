#ifndef COMUT_SRWS_H_
#define COMUT_SRWS_H_

#include "expr_mutant_operator.h"

class SRWS : public ExprMutantOperator
{
public:
	SRWS(const std::string name = "SRWS")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);
};

#endif	// COMUT_SRWS_H_