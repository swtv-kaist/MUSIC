#ifndef MUSIC_OCOR_H_
#define MUSIC_OCOR_H_

#include "expr_mutant_operator.h"

class OCOR : public ExprMutantOperator
{
public:
	OCOR(const std::string name = "OCOR");

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	std::vector<std::string> integral_type_list_;
  std::vector<std::string> floating_type_list_;

  void MutateToIntegralType(
			const string &type_str, const string &token,
			const SourceLocation &start_loc, const SourceLocation &end_loc, 
			MusicContext *context);
	void MutateToFloatingType(
			const string &type_str, const string &token,
			const SourceLocation &start_loc, const SourceLocation &end_loc, 
			MusicContext *context);
	void MutateToSpecifiedRange(
			const string &type_str, const string &token,
			const SourceLocation &start_loc, const SourceLocation &end_loc, 
			MusicContext *context);
};

#endif	// MUSIC_OCOR_H_