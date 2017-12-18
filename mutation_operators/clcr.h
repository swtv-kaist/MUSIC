#ifndef COMUT_CLCR_H_
#define COMUT_CLCR_H_	

#include "expr_mutant_operator.h"

class CLCR : public ExprMutantOperator
{
public:
	CLCR(const std::string name = "CLCR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	bool IsDuplicateCaseLabel(string new_label, 
														SwitchStmtInfoList *switchstmt_list);
};

#endif	// COMUT_CLCR_H_