#ifndef COMUT_CLCR_H_
#define COMUT_CLCR_H_	

#include "expr_mutant_operator.h"

class CLCR : public ExprMutantOperator
{
public:
	CLCR(const std::string name = "CLCR")
		: ExprMutantOperator(name), choose_max_(false), 
		choose_min_(false), choose_median_(false), 
		close_less_(false), close_more_(false)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	bool choose_max_;
	bool choose_min_;
	bool choose_median_;
	bool close_less_;
	bool close_more_;

	bool IsDuplicateCaseLabel(string new_label, 
														SwitchStmtInfoList *switchstmt_list);
	void GetRange(clang::Expr *e, ComutContext *context, std::vector<std::string> *range);
};

#endif	// COMUT_CLCR_H_