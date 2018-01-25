#ifndef MUSIC_EXPR_MUTANT_OPERATOR_H_
#define MUSIC_EXPR_MUTANT_OPERATOR_H_	

#include "mutant_operator_template.h"

class ExprMutantOperator: public MutantOperatorTemplate
{
public:
	ExprMutantOperator(const std::string name) 
		: MutantOperatorTemplate(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain) = 0;
	virtual bool ValidateRange(const std::set<std::string> &range) = 0;

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context) = 0;

	virtual void Mutate(clang::Expr *e, MusicContext *context) = 0;
};

#endif	// MUSIC_EXPR_MUTANT_OPERATOR_H_