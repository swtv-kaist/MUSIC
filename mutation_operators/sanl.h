#ifndef COMUT_SANL_H_
#define COMUT_SANL_H_ 

#include "mutant_operator_template.h"

class SANL : public MutantOperatorTemplate
{
public:
	SANL(const std::string name = "SANL")
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

#endif	// COMUT_SANL_H_