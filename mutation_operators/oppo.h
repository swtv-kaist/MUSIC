#ifndef COMUT_OPPO_H_
#define COMUT_OPPO_H_	

#include "mutant_operator_template.h"

class OPPO : public MutantOperatorTemplate
{
public:
	OPPO(const std::string name = "OPPO")
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
	void GenerateMutantForPostInc(UnaryOperator *uo, ComutContext *context);
	void GenerateMutantForPreInc(UnaryOperator *uo, ComutContext *context);
};

#endif	// COMUT_OPPO_H_