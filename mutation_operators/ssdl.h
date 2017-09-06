#ifndef COMUT_SSDL_H_
#define COMUT_SSDL_H_ 

#include "stmt_mutant_operator.h"
#include <iostream>

class SSDL : public StmtMutantOperator
{
public:
	SSDL(const std::string name = "SSDL")
		: StmtMutantOperator(name)
	{}

	// option -A and -B are not available for SSDL
	// using them for SSDL terminates COMUT
	virtual bool ValidateDomain(const std::set<std::string> &domain);
	virtual bool ValidateRange(const std::set<std::string> &range);

	virtual bool CanMutate(clang::Stmt *s, ComutContext *context);
	virtual void Mutate(clang::Stmt *s, ComutContext *context);

	void DeleteStatement(Stmt *s, ComutContext *context);

	// an unremovable label is a label defined inside range stmtRange,
	// but goto-ed outside of range stmtRange.
	// Deleting such label can cause goto-undefined-label error.
	// Return True if there is no such label inside given range
	bool NoUnremovableLabelInsideRange(SourceManager &src_mgr, SourceRange range, 
																		 LabelStmtToGotoStmtListMap *label_map);

	bool HandleStmtWithSubStmt(Stmt *s, ComutContext *context);

	void HandleStmtWithBody(Stmt *s, ComutContext *context);

	void DeleteCompoundStmtContent(CompoundStmt *c, ComutContext *context);
};

#endif	// COMUT_SSDL_H_