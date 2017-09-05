#ifndef COMUT_OLSN_H_
#define COMUT_OLSN_H_

#include "expr_mutant_operator.h"

class OLSN : public ExprMutantOperator
{
public:
	OLSN(const std::string name = "OLSN")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	virtual void setDomain(std::set<std::string> &domain);
  virtual void setRange(std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);

private:
	bool CanMutate(BinaryOperator *bo, string mutated_token,
								 ComutContext *context);
};

#endif	// COMUT_OLSN_H_