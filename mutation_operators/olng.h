#ifndef COMUT_OLNG_H_
#define COMUT_OLNG_H_	

#include "expr_mutant_operator.h"

class OLNG : public ExprMutantOperator
{
public:
	OLNG(const std::string name = "OLNG")
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

#endif	// COMUT_OLNG_H_