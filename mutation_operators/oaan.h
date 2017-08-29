#ifndef COMUT_OAAN_H_
#define COMUT_OAAN_H_

#include "mutant_operator_template.h"

class OAAN : public MutantOperatorTemplate
{
public:
	OAAN(const std::string name = "OAAN")
		: MutantOperatorTemplate(name), only_plus_minus_(false)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	virtual void setDomain(std::set<std::string> &domain);
  virtual void setRange(std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	// Return True if the mutant operator can mutate this statement
	virtual bool CanMutate(clang::Stmt *s, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);
	virtual void Mutate(clang::Stmt *s, ComutContext *context);

private:
	bool only_plus_minus_;

	bool CanMutate(BinaryOperator *bo, string mutated_token,
								 ComutContext *context);
};

#endif	// COMUT_OAAN_H_