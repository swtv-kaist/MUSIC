#ifndef COMUT_SCSR_H_
#define COMUT_SCSR_H_

#include "mutant_operator_template.h"

class SCSR : public MutantOperatorTemplate
{
public:
	SCSR(const std::string name = "SCSR")
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
	void GenerateGlobalMutants(Expr *e, ComutContext *context,
														 set<string> *stringCache);
	void GenerateLocalMutants(Expr *e, ComutContext *context,
													  set<string> *stringCache);
};

#endif	// COMUT_SCSR_H_