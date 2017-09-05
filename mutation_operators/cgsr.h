#ifndef COMUT_CGSR_H_
#define COMUT_CGSR_H_	

#include "expr_mutant_operator.h"

class CGSR : public ExprMutantOperator
{
public:
	CGSR(const std::string name = "CGSR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);
};

#endif	// COMUT_CGSR_H_