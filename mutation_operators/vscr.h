#ifndef COMUT_VSCR_H_
#define COMUT_VSCR_H_

#include "expr_mutant_operator.h"

class VSCR : public ExprMutantOperator
{
public:
	VSCR(const std::string name = "VSCR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	bool IsSameType(const QualType type1, const QualType type2);
};

#endif	// COMUT_VSCR_H_