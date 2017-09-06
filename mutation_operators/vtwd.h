#ifndef COMUT_VTWD_H_
#define COMUT_VTWD_H_ 

#include "expr_mutant_operator.h"

class VTWD : public ExprMutantOperator
{
public:
	VTWD(const std::string name = "VTWD")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	bool CanMutate(std::string scalarref_name, ComutContext *context);
};

#endif	// COMUT_VTWD_H_