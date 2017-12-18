#ifndef COMUT_OPPO_H_
#define COMUT_OPPO_H_	

#include "expr_mutant_operator.h"

class OPPO : public ExprMutantOperator
{
public:
	OPPO(const std::string name = "OPPO")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	void GenerateMutantForPostInc(UnaryOperator *uo, ComutContext *context);
	void GenerateMutantForPreInc(UnaryOperator *uo, ComutContext *context);
};

#endif	// COMUT_OPPO_H_