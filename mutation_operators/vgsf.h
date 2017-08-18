#ifndef COMUT_VGSF_H_
#define COMUT_VGSF_H_

#include "mutant_operator_template.h"

class VGSF : public MutantOperatorTemplate
{
public:
	VGSF(const std::string name = "VGSF")
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
};
	
#endif	// COMUT_VGSF_H_