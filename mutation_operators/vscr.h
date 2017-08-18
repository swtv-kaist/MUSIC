#ifndef COMUT_VSCR_H_
#define COMUT_VSCR_H_

#include "mutant_operator_template.h"

class VSCR : public MutantOperatorTemplate
{
public:
	VSCR(const std::string name = "VSCR")
		: MutantOperatorTemplate(name)
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
	bool IsSameType(const QualType type1, const QualType type2);
};

#endif	// COMUT_VSCR_H_