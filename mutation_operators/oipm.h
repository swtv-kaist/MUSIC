#ifndef COMUT_OIPM_H_
#define COMUT_OIPM_H_

#include "expr_mutant_operator.h"

class OIPM : public ExprMutantOperator
{
public:
	OIPM(const std::string name = "OIPM")
		: ExprMutantOperator(name), is_subexpr_arraysubscript(false),
		  is_subexpr_pointer(false)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	bool is_subexpr_arraysubscript;
	bool is_subexpr_pointer;

	SourceLocation GetLeftBracketOfArraySubscript(
		const ArraySubscriptExpr *ase, SourceManager &src_mgr);

	void MutateArraySubscriptSubExpr(
			Expr *subexpr, const SourceLocation start_loc,
			const SourceLocation end_loc, const string token, 
			ComutContext *context);

	void MutatePointerSubExpr(
			Expr *subexpr, const SourceLocation start_loc,
			const SourceLocation end_loc, const string token, 
			ComutContext *context);
};

#endif	// 	COMUT_OIPM_H_