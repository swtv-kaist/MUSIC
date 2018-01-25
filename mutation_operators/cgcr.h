#ifndef MUSIC_CGCR_H_
#define MUSIC_CGCR_H_	

#include "expr_mutant_operator.h"

class CGCR : public ExprMutantOperator
{
public:
	CGCR(const std::string name = "CGCR")
		: ExprMutantOperator(name)
	{}

	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	// Return True if the mutant operator can mutate this expression
	virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

	virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
	bool IsDuplicateCaseLabel(string new_label, 
														SwitchStmtInfoList *switchstmt_list);
};

#endif	// MUSIC_CGCR_H_