#ifndef COMUT_VLSR_H_
#define COMUT_VLSR_H_

#include "expr_mutant_operator.h"

class VLSR : public ExprMutantOperator
{
public:
	VLSR(const std::string name = "VLSR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	void GetRange(clang::Expr *e, ComutContext *context, VarDeclList *range);
};

#endif	// COMUT_VLSR_H_