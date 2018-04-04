#ifndef MUSIC_SSDL_H_
#define MUSIC_SSDL_H_ 

#include "stmt_mutant_operator.h"
#include <iostream>

class SSDL : public StmtMutantOperator
{
public:
	SSDL(const std::string name = "SSDL")
		: StmtMutantOperator(name)
	{}

	// option -A and -B are not available for SSDL
	// using them for SSDL terminates MUSIC
	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	virtual bool IsMutationTarget(clang::Stmt *s, MusicContext *context);
	virtual void Mutate(clang::Stmt *s, MusicContext *context);

	void DeleteStatement(Stmt *s, MusicContext *context);

	// an unremovable label is a label defined inside range stmtRange,
	// but goto-ed outside of range stmtRange.
	// Deleting such label can cause goto-undefined-label error.
	// Return True if there is no such label inside given range
	bool NoUnremovableLabelInsideRange(SourceManager &src_mgr, SourceRange range, 
																		 LabelStmtToGotoStmtListMap *label_map);

	bool HandleStmtWithSubStmt(Stmt *s, MusicContext *context);

	void HandleStmtWithBody(Stmt *s, MusicContext *context);

	void DeleteCompoundStmtContent(CompoundStmt *c, MusicContext *context);

	bool IsInSpecifiedDomain(SourceManager &src_mgr,
													clang::SourceLocation start_loc,
													clang::SourceLocation end_loc);
};

#endif	// MUSIC_SSDL_H_