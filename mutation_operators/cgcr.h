#ifndef COMUT_CGCR_H_
#define COMUT_CGCR_H_	

#include "expr_mutant_operator.h"

class CGCR : public ExprMutantOperator
{
public:
	CGCR(const std::string name = "CGCR")
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

#endif	// COMUT_CGCR_H_