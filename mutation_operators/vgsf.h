#ifndef COMUT_VGSF_H_
#define COMUT_VGSF_H_

#include "expr_mutant_operator.h"

class VGSF : public ExprMutantOperator
{
public:
	VGSF(const std::string name = "VGSF")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);
};
	
#endif	// COMUT_VGSF_H_