#ifndef COMUT_STMT_CONTEXT_H_
#define COMUT_STMT_CONTEXT_H_ 

#include <vector>
#include <string>
#include <utility>
#include <map>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"

class StmtContext
{
private:
	// initialize to 0
  int proteumstyle_stmt_start_line_num_;

	// initialize to false
  bool is_inside_stmtexpr_;
  bool is_inside_array_decl_size_;
  bool is_inside_enumdecl_;

  clang::SourceRange *lhs_of_assignment_range_;
  clang::SourceRange *addressop_range_;
  clang::SourceRange *unary_inc_dec_range_;
  clang::SourceRange *fielddecl_range_;
  clang::SourceRange *currently_parsed_function_range_;
  clang::SourceRange *switchstmt_condition_range_;
  clang::SourceRange *arraysubscript_range_;
  clang::SourceRange *switchcase_range_;
  clang::SourceRange *non_floating_expr_range_;
  clang::SourceRange *typedef_range_;

public:
	StmtContext(clang::CompilerInstance *CI);

	// getter
	int getProteumStyleLineNum();

	// setters
	void setProteumStyleLineNum(int num);
	void setIsInStmtExpr(bool value);
	void setIsInArrayDeclSize(bool value);
	void setIsInEnumDecl(bool value);
	void setLhsOfAssignmentRange(clang::SourceRange *range);
  void setAddressOpRange(clang::SourceRange *range);
  void setUnaryIncrementDecrementRange(clang::SourceRange *range);
  void setFieldDeclRange(clang::SourceRange *range);
  void setCurrentlyParsedFunctionRange(clang::SourceRange *range);
  void setSwitchStmtConditionRange(clang::SourceRange *range);
  void setArraySubscriptRange(clang::SourceRange *range);
  void setSwitchCaseRange(clang::SourceRange *range);
  void setNonFloatingExprRange(clang::SourceRange *range);
  void setTypedefDeclRange(clang::SourceRange *range);

	bool IsInStmtExpr();
	bool IsInArrayDeclSize();
	bool IsInEnumDecl();

	bool IsInLhsOfAssignmentRange(clang::Stmt *s);
	bool IsInAddressOpRange(clang::Stmt *s);
	bool IsInUnaryIncrementDecrementRange(clang::Stmt *s);
	bool IsInFieldDeclRange(clang::Stmt *s);
	bool IsInCurrentlyParsedFunctionRange(clang::Stmt *s);
	bool IsInSwitchStmtConditionRange(clang::Stmt *s);
	bool IsInArraySubscriptRange(clang::Stmt *s);
	bool IsInSwitchCaseRange(clang::Stmt *s);
	bool IsInNonFloatingExprRange(clang::Stmt *s);
	bool IsInTypedefRange(clang::Stmt *s);
};

#endif	// COMUT_STMT_CONTEXT_H_