#ifndef COMUT_VGPF_H_
#define COMUT_VGPF_H_

#include "expr_mutant_operator.h"

class VGPF : public ExprMutantOperator
{
public:
	VGPF(const std::string name = "VGPF")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);
};
	
#endif	// COMUT_VGPF_H_