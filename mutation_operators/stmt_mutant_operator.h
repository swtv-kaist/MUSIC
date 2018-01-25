#ifndef MUSIC_STMT_MUTANT_OPERATOR_H_
#define MUSIC_STMT_MUTANT_OPERATOR_H_	

#include "mutant_operator_template.h"

class StmtMutantOperator: public MutantOperatorTemplate
{
public:
	StmtMutantOperator(const std::string name) 
		: MutantOperatorTemplate(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain) = 0;
	virtual bool ValidateRange(const std::set<std::string> &range) = 0;

	// Return True if the mutant operator can mutate this statement
	virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context) = 0;

	virtual void Mutate(clang::Stmt *s, MusicContext *context) = 0;
};

#endif	// MUSIC_STMT_MUTANT_OPERATOR_H_