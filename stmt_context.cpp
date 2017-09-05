#include "comut_utility.h"
#include "stmt_context.h"

StmtContext::StmtContext(CompilerInstance *CI)
	:proteumstyle_stmt_start_line_num_(0),
	is_inside_stmtexpr_(false), is_inside_array_decl_size_(0),
	is_inside_enumdecl_(false)
{
	clang::SourceManager &src_mgr = CI->getSourceManager();
	clang::SourceLocation start_of_file = \
			src_mgr.getLocForStartOfFile(src_mgr.getMainFileID());

	lhs_of_assignment_range_ = new clang::SourceRange(
				start_of_file, start_of_file);
  addressop_range_ = new clang::SourceRange(
  			start_of_file, start_of_file);
  unary_inc_dec_range_ = new clang::SourceRange(
  			start_of_file, start_of_file);
  fielddecl_range_ = new clang::SourceRange(
  			start_of_file, start_of_file);
  currently_parsed_function_range_ = new clang::SourceRange(
  			start_of_file, start_of_file);
  switchstmt_condition_range_ = new clang::SourceRange(
  			start_of_file, start_of_file);
  arraysubscript_range_ = new clang::SourceRange(
  			start_of_file, start_of_file);
  switchcase_range_ = new clang::SourceRange(
  			start_of_file, start_of_file);
  non_floating_expr_range_ = new clang::SourceRange(
  			start_of_file, start_of_file);
  typedef_range_ = new clang::SourceRange(
  			start_of_file, start_of_file);
}

int StmtContext::getProteumStyleLineNum()
{
	return proteumstyle_stmt_start_line_num_;
}

void StmtContext::setProteumStyleLineNum(int num)
{
	proteumstyle_stmt_start_line_num_ = num;
}

void StmtContext::setIsInStmtExpr(bool value)
{
	is_inside_stmtexpr_ = value;
}

void StmtContext::setIsInArrayDeclSize(bool value)
{
	is_inside_array_decl_size_ = value;
}

void StmtContext::setIsInEnumDecl(bool value)
{
	is_inside_enumdecl_ = value;
}

void StmtContext::setLhsOfAssignmentRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (lhs_of_assignment_range_ != nullptr)
		delete lhs_of_assignment_range_;

	lhs_of_assignment_range_ = range;
}

void StmtContext::setAddressOpRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (addressop_range_ != nullptr)
		delete addressop_range_;

	addressop_range_ = range;
}

void StmtContext::setUnaryIncrementDecrementRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (unary_inc_dec_range_ != nullptr)
		delete unary_inc_dec_range_;

	unary_inc_dec_range_ = range;
}

void StmtContext::setFieldDeclRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (fielddecl_range_ != nullptr)
		delete fielddecl_range_;

	fielddecl_range_ = range;
}

void StmtContext::setCurrentlyParsedFunctionRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (currently_parsed_function_range_ != nullptr)
		delete currently_parsed_function_range_;

	currently_parsed_function_range_ = range;
}

void StmtContext::setSwitchStmtConditionRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (switchstmt_condition_range_ != nullptr)
		delete switchstmt_condition_range_;

	switchstmt_condition_range_ = range;
}

void StmtContext::setArraySubscriptRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (arraysubscript_range_ != nullptr)
		delete arraysubscript_range_;

	arraysubscript_range_ = range;
}

void StmtContext::setSwitchCaseRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (switchcase_range_ != nullptr)
		delete switchcase_range_;

	switchcase_range_ = range;
}

void StmtContext::setNonFloatingExprRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (non_floating_expr_range_ != nullptr)
		delete non_floating_expr_range_;

	non_floating_expr_range_ = range;
}

void StmtContext::setTypedefDeclRange(clang::SourceRange *range)
{
	if (range == nullptr) return;

	if (typedef_range_ != nullptr)
		delete typedef_range_;

	typedef_range_ = range;
}

bool StmtContext::IsInStmtExpr()
{
	return is_inside_stmtexpr_;
}

bool StmtContext::IsInArrayDeclSize()
{
	return is_inside_array_decl_size_;
} 

bool StmtContext::IsInEnumDecl()
{
	return is_inside_enumdecl_;
}

bool StmtContext::IsInLhsOfAssignmentRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), *lhs_of_assignment_range_);
}

bool StmtContext::IsInAddressOpRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), *addressop_range_);
}

bool StmtContext::IsInUnaryIncrementDecrementRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), *unary_inc_dec_range_);
}

bool StmtContext::IsInFieldDeclRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), *fielddecl_range_);
}

bool StmtContext::IsInCurrentlyParsedFunctionRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), 
													 *currently_parsed_function_range_);
}

bool StmtContext::IsInSwitchStmtConditionRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), *switchstmt_condition_range_);
}

bool StmtContext::IsInArraySubscriptRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), *arraysubscript_range_);
}

bool StmtContext::IsInSwitchCaseRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), *switchcase_range_);
}

bool StmtContext::IsInNonFloatingExprRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), *non_floating_expr_range_);
}

bool StmtContext::IsInTypedefRange(Stmt *s)
{
	return LocationIsInRange(s->getLocStart(), *typedef_range_);
}