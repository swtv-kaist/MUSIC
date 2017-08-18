#ifndef COMUT_VLTF_H_
#define COMUT_VLTF_H_

#include "mutant_operator_template.h"

class VLTF : public MutantOperatorTemplate
{
public:
	VLTF(const std::string name = "VLTF")
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
	
#endif	// COMUT_VLTF_H_