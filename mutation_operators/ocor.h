#ifndef COMUT_OCOR_H_
#define COMUT_OCOR_H_

#include "mutant_operator_template.h"

class OCOR : public MutantOperatorTemplate
{
public:
	OCOR(const std::string name = "OCOR");

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool CanMutate(clang::Expr *e, ComutContext *context);

	// Return True if the mutant operator can mutate this statement
	virtual bool CanMutate(clang::Stmt *s, ComutContext *context);

	virtual void Mutate(clang::Expr *e, ComutContext *context);
	virtual void Mutate(clang::Stmt *s, ComutContext *context);

private:
	std::vector<std::string> integral_type_list_;
  std::vector<std::string> floating_type_list_;

  void MutateToIntegralType(
			const string &type_str, const string &token,
			const SourceLocation &start_loc, const SourceLocation &end_loc, 
			ComutContext *context);
	void MutateToFloatingType(
			const string &type_str, const string &token,
			const SourceLocation &start_loc, const SourceLocation &end_loc, 
			ComutContext *context);
};

#endif	// COMUT_OCOR_H_