#ifndef COMUT_CONTEXT_H_
#define COMUT_CONTEXT_H_ 

#include <vector>
#include <string>
#include <utility>
#include <map>

#include "clang/Frontend/CompilerInstance.h"

#include "configuration.h"

// <line number, column number>
typedef std::pair<int, int> LabelStmtLocation; 

// list of locations of goto statements
typedef std::vector<clang::SourceLocation> GotoStmtLocationList;

typedef std::vector<std::string> GlobalStringLiteralList;
typedef std::vector<std::pair<std::string, clang::SourceLocation>> LocalStringLiteralList;

struct comp
{
  bool operator() (const LabelStmtLocation &lhs, const LabelStmtLocation &rhs) const
  {
    // if lhs appear before rhs, return true
    // else return false.

    if (lhs.first < rhs.first)
      return true;

    if (lhs.first == rhs.first && lhs.second < rhs.second)
      return true;

    return false;
  }
};

// Map label declaration location with its usage locations (goto)
// Key is location instead of name to resolve same named labels in different ftn.
typedef std::map<LabelStmtLocation, GotoStmtLocationList, comp> LabelStmtToGotoStmtListMap;

typedef std::vector<std::string> GlobalStringLiteralList;
typedef std::vector<std::pair<std::string, clang::SourceLocation>> LocalStringLiteralList;

typedef std::vector<clang::VarDecl *> VarDeclList;

typedef std::vector<std::string> ScalarReferenceNameList;

// pair<string representation, isFloat?>
typedef std::vector<std::pair<std::string, bool>> GlobalScalarConstantList;

// pair<string representation, pair<location, isFloat?>>
typedef std::vector<std::pair<std::string, std::pair<clang::SourceLocation, bool>>> LocalScalarConstantList;

// pair<range of switch statement, list of case values' string representation>
typedef std::vector<std::pair<clang::SourceRange, std::vector<std::string>>> SwitchStmtInfoList;

struct ComutContext
{
	// initialize to false
	bool is_inside_stmtexpr;
  bool is_inside_array_decl_size;
  bool is_inside_enumdecl;

	clang::CompilerInstance *comp_inst;
	LabelStmtToGotoStmtListMap *label_to_gotolist_map;
  SwitchStmtInfoList *switchstmt_info_list;

	// initialize to 1
	int next_mutantfile_id;

	Configuration *userinput;

	// initialize to 0
	int proteumstyle_stmt_start_line_num;

  std::string mutant_filename;

  clang::SourceRange *lhs_of_assignment_range;
  clang::SourceRange *addressop_range;
  clang::SourceRange *unary_increment_range;
  clang::SourceRange *unary_decrement_range;
  clang::SourceRange *fielddecl_range;
  clang::SourceRange *currently_parsed_function_range;
  clang::SourceRange *switchstmt_condition_range;
  clang::SourceRange *arraysubscript_range;
  clang::SourceRange *switchcase_range;

  // global/local number, char literals
  GlobalScalarConstantList *global_scalarconstant_list;
  LocalScalarConstantList *local_scalarconstant_list;

  // global/local string literals
  GlobalStringLiteralList *global_stringliteral_list;
  LocalStringLiteralList *local_stringliteral_list;

  // global/local scalar variables (char, int, double, float)
  // local_scalar_vardecl_list_ follows the same nesting rule as ScopeRangeList
  VarDeclList *global_scalar_vardecl_list;
  std::vector<VarDeclList> *local_scalar_vardecl_list;

  // global/local array variables
  // local_array_vardecl_list_ follows the same nesting rule as ScopeRangeList
  VarDeclList *global_array_vardecl_list;
  std::vector<VarDeclList> *local_array_vardecl_list;

  // global/local struct variables
  // local_struct_vardecl_list_ follows the same nesting rule as ScopeRangeList
  VarDeclList *global_struct_vardecl_list;
  std::vector<VarDeclList> *local_struct_vardecl_list;

  // global/local pointers
  // local_pointer_vardecl_list_ follows the same nesting rule as ScopeRangeList
  VarDeclList *global_pointer_vardecl_list;
  std::vector<VarDeclList> *local_pointer_vardecl_list;

  ScalarReferenceNameList *non_VTWD_mutatable_scalarref_list;
};

#endif	// COMUT_CONTEXT_H_