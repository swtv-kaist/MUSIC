#ifndef COMUT_VGTR_H_
#define COMUT_VGTR_H_

#include "expr_mutant_operator.h"

class VGTR : public ExprMutantOperator
{
public:
	VGTR(const std::string name = "VGTR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);
};

#endif	// COMUT_VGTR_H_