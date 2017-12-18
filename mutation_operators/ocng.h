#ifndef COMUT_OCNG_H_
#define COMUT_OCNG_H_	

#include "stmt_mutant_operator.h"

class OCNG : public StmtMutantOperator
{
public:
	OCNG(const std::string name = "OCNG")
		: StmtMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this statement
	virtual bool CanMutate(clang::Stmt *s, ComutContext *context);

	virtual void Mutate(clang::Stmt *s, ComutContext *context);

private:
	void GenerateMutantByNegation(Expr *e, ComutContext *context);
};

#endif	// COMUT_OCNG_H_