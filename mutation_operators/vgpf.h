#ifndef COMUT_VGPF_H_
#define COMUT_VGPF_H_

#include "mutant_operator_template.h"

class VGPF : public MutantOperatorTemplate
{
public:
	VGPF(const std::string name = "VGPF")
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
	
#endif	// COMUT_VGPF_H_