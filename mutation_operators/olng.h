#ifndef COMUT_OLNG_H_
#define COMUT_OLNG_H_	

#include "mutant_operator_template.h"

class OLNG : public MutantOperatorTemplate
{
public:
	OLNG(const std::string name = "OLNG")
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
	void GenerateMutantByNegation(Expr *e, ComutContext *context);
};

#endif	// COMUT_OLNG_H_