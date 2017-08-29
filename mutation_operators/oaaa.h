#ifndef COMUT_OAAA_H_
#define COMUT_OAAA_H_

#include "mutant_operator_template.h"

class OAAA : public MutantOperatorTemplate
{
public:
	OAAA(const std::string name = "OAAA")
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

	bool CanMutate(clang::BinaryOperator * const bo);
};

#endif	// COMUT_OAAA_H_