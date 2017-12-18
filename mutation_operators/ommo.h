#ifndef COMUT_OMMO_H_
#define COMUT_OMMO_H_	

#include "expr_mutant_operator.h"

class OMMO : public ExprMutantOperator
{
public:
	OMMO(const std::string name = "OMMO")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	void GenerateMutantForPostDec(UnaryOperator *uo, ComutContext *context);
	void GenerateMutantForPreDec(UnaryOperator *uo, ComutContext *context);
};

#endif	// COMUT_OMMO_H_