#ifndef MUSIC_SYMBOL_TABLE
#define MUSIC_SYMBOL_TABLE

#include <vector>
#include <string>
#include <utility>
#include <map>
#include <set>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"

// <line number, column number>
typedef std::pair<int, int> LabelStmtLocation; 

// list of locations of goto statements
typedef std::vector<clang::SourceLocation> GotoStmtLocationList;

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

typedef std::vector<clang::Expr*> ExprList;
typedef std::vector<clang::VarDecl *> VarDeclList;

typedef ExprList GlobalStringLiteralList;
typedef std::vector<ExprList> LocalStringLiteralList;

typedef ExprList GlobalScalarConstantList;
typedef std::vector<ExprList> LocalScalarConstantList;

// pair<range of switch statement, list of case values' string representation>
typedef std::vector<std::pair<clang::SourceRange, std::vector<std::string>>> SwitchStmtInfoList;

typedef std::vector<std::vector<clang::ParmVarDecl *>> FunctionParamList;
typedef std::vector<std::vector<clang::NamedDecl *>> FunctionUsedGlobalList;
typedef std::vector<VarDeclList> FunctionNotUsedGlobalList;
typedef std::vector<VarDeclList> FunctionLocalList;

// a map from a code line to a set of the variables used at the code line
typedef std::map<int, std::set<clang::VarDecl *>> LineToVarsMap;
typedef std::map<int, std::set<clang::Expr *>> LineToConstsMap;

typedef std::vector<clang::LabelStmt *> LabelList;

typedef std::vector<clang::ReturnStmt *> ReturnStmtList;

class SymbolTable
{
public:
	SymbolTable(
			GlobalScalarConstantList *g_scalarconstant_list,
      LocalScalarConstantList *l_scalarconstant_list,
      GlobalStringLiteralList *g_stringliteral_list,
      LocalStringLiteralList *l_stringliteral_list,
      VarDeclList *g_scalar_vardecl_list,
		  std::vector<VarDeclList> *l_scalar_vardecl_list,
			VarDeclList *g_array_vardecl_list,
		  std::vector<VarDeclList> *l_array_vardecl_list,
			VarDeclList *g_struct_vardecl_list,
		  std::vector<VarDeclList> *l_struct_vardecl_list,
			VarDeclList *g_pointer_vardecl_list,
		  std::vector<VarDeclList> *l_pointer_vardecl_list,
      FunctionParamList *func_param_list,
      FunctionUsedGlobalList *func_used_global_list,
      FunctionNotUsedGlobalList *func_not_used_global_list,
      FunctionLocalList *func_local_list,
      LineToVarsMap *line_to_vars_map,
      LineToConstsMap *line_to_consts_map,
      std::vector<LabelList> *label_list,
      std::vector<ReturnStmtList> *return_stmt_list_);


  // getters
	GlobalScalarConstantList* getGlobalScalarConstantList();
	LocalScalarConstantList* getLocalScalarConstantList();
	GlobalStringLiteralList* getGlobalStringLiteralList();
	LocalStringLiteralList* getLocalStringLiteralList();
	VarDeclList* getGlobalScalarVarDeclList();
	std::vector<VarDeclList>* getLocalScalarVarDeclList();
	VarDeclList* getGlobalArrayVarDeclList();
	std::vector<VarDeclList>* getLocalArrayVarDeclList();
	VarDeclList* getGlobalStructVarDeclList();
	std::vector<VarDeclList>* getLocalStructVarDeclList();
	VarDeclList* getGlobalPointerVarDeclList();
	std::vector<VarDeclList>* getLocalPointerVarDeclList();
  FunctionParamList* getFuncParamList();
  FunctionUsedGlobalList* getFuncUsedGlobalList();
  FunctionNotUsedGlobalList* getFuncNotUsedGlobalList();
  FunctionLocalList* getFuncLocalList();
  LineToVarsMap* getLineToVarMap();
  LineToConstsMap* getLineToConstsMap();
  std::vector<LabelList>* getLabelList();
  std::vector<ReturnStmtList>* getReturnStmtList();
	
private:
	// global/local number, char literals
  GlobalScalarConstantList *global_scalarconstant_list_;
  LocalScalarConstantList *local_scalarconstant_list_;

  // global/local string literals
  GlobalStringLiteralList *global_stringliteral_list_;
  LocalStringLiteralList *local_stringliteral_list_;

	// global/local scalar variables (char, int, double, float)
  // local_scalar_vardecl_list_ follows the same nesting rule as ScopeRangeList
  VarDeclList *global_scalar_vardecl_list_;
  std::vector<VarDeclList> *local_scalar_vardecl_list_;

  // global/local array variables
  // local_array_vardecl_list_ follows the same nesting rule as ScopeRangeList
  VarDeclList *global_array_vardecl_list_;
  std::vector<VarDeclList> *local_array_vardecl_list_;

  // global/local struct variables
  // local_struct_vardecl_list_ follows the same nesting rule as ScopeRangeList
  VarDeclList *global_struct_vardecl_list_;
  std::vector<VarDeclList> *local_struct_vardecl_list_;

  // global/local pointers
  // local_pointer_vardecl_list_ follows the same nesting rule as ScopeRangeList
  VarDeclList *global_pointer_vardecl_list_;
  std::vector<VarDeclList> *local_pointer_vardecl_list_;

  FunctionParamList *func_param_list_;
  FunctionUsedGlobalList *func_used_global_list_;
  FunctionNotUsedGlobalList *func_not_used_global_list_;
  FunctionLocalList *func_local_list_;

  LineToVarsMap *line_to_vars_map_;
  LineToConstsMap *line_to_consts_map_;

  // Each function has an ID corresponding to the vector index
  // Basically, this is like a function id to label list map.
  std::vector<LabelList> *label_list_;
  std::vector<ReturnStmtList> *return_stmt_list_;

};

#endif	// MUSIC_SYMBOL_TABLE