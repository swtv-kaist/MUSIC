#ifndef MUSIC_OEAA_H_
#define MUSIC_OEAA_H_

#include "expr_mutant_operator.h"

class OEAA : public ExprMutantOperator
{
public:
	OEAA(const std::string name = "OEAA")
		: ExprMutantOperator(name), only_plus_minus_(false),
			only_minus_(false), only_plus_(false)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	virtual void setDomain(std::set<std::string> &domain);
  virtual void setRange(std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	bool only_plus_minus_;
	bool only_minus_;
	bool only_plus_;

	bool IsMutationTarget(clang::BinaryOperator * const bo, 
								 clang::CompilerInstance *comp_inst);
};

#endif	// MUSIC_OEAA_H_