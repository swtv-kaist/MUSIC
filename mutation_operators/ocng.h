#ifndef MUSIC_OCNG_H_
#define MUSIC_OCNG_H_	

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
	virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);

	virtual void Mutate(clang::Stmt *s, MusicContext *context);

private:
	void GenerateMutantByNegation(Expr *e, MusicContext *context);
};

#endif	// MUSIC_OCNG_H_