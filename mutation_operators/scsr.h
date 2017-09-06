#ifndef COMUT_SCSR_H_
#define COMUT_SCSR_H_

#include "expr_mutant_operator.h"

class SCSR : public ExprMutantOperator
{
public:
	SCSR(const std::string name = "SCSR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	void GenerateGlobalMutants(Expr *e, ComutContext *context,
														 set<string> *stringCache);
	void GenerateLocalMutants(Expr *e, ComutContext *context,
													  set<string> *stringCache);
};

#endif	// COMUT_SCSR_H_