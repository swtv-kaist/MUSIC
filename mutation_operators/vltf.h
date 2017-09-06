#ifndef COMUT_VLTF_H_
#define COMUT_VLTF_H_

#include "expr_mutant_operator.h"

class VLTF : public ExprMutantOperator
{
public:
	VLTF(const std::string name = "VLTF")
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
	
#endif	// COMUT_VLTF_H_