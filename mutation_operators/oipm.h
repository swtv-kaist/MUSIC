#ifndef COMUT_OIPM_H_
#define COMUT_OIPM_H_

#include "mutant_operator_template.h"

class OIPM : public MutantOperatorTemplate
{
public:
	OIPM(const std::string name = "OIPM")
		: MutantOperatorTemplate(name), is_subexpr_arraysubscript(false),
		  is_subexpr_pointer(false)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	// Return True if the mutant operator can mutate this statement
	virtual bool CanMutate(clang::Stmt *s, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);
	virtual void Mutate(clang::Stmt *s, ComutContext *context);

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