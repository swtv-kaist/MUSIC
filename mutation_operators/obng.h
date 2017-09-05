#ifndef COMUT_OBNG_H_
#define COMUT_OBNG_H_	

#include "expr_mutant_operator.h"

class OBNG : public ExprMutantOperator
{
public:
	OBNG(const std::string name = "OBNG")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	void GenerateMutantByNegation(Expr *e, ComutContext *context);
};

#endif	// COMUT_OBNG_H_