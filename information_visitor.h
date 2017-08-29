#ifndef COMUT_INFORMATION_VISITOR_H_
#define COMUT_INFORMATION_VISITOR_H_

#include <set>

#include "clang/Rewrite/Core/Rewriter.h"

#include "comut_context.h"

class InformationVisitor : public clang::RecursiveASTVisitor<InformationVisitor>
{
public:
  InformationVisitor(clang::CompilerInstance *CI);

  // Add a new Goto statement location to LabelStmtToGotoStmtListMap.
  // Add the label to the map if map does not contain label.
  // Else add the Goto location to label's list of Goto locations.
  void addGotoLocToMap(LabelStmtLocation label_loc, 
                       clang::SourceLocation goto_stmt_loc);
  
  bool VisitLabelStmt(clang::LabelStmt *ls);
  bool VisitGotoStmt(clang::GotoStmt * gs);
  bool VisitExpr(clang::Expr *e);
  bool VisitFunctionDecl(clang::FunctionDecl *fd);

public:
	clang::CompilerInstance *comp_inst_;

  clang::SourceManager &src_mgr_;
  clang::LangOptions &lang_option_;

  clang::Rewriter rewriter_;

  // List of locations of label declarations
  std::vector<clang::SourceLocation> *label_srclocation_list_; 

  // Map from label declaration location to locations of Goto statements
  // pointing to that label.
  LabelStmtToGotoStmtListMap label_to_gotolist_map_;

  // Global/Local numbers, chars
  GlobalScalarConstantList global_scalarconstant_list_;
  LocalScalarConstantList local_scalarconstant_list_;

  // Last (or current) range of the function COMUT is traversing
  clang::SourceRange *currently_parsed_function_range_;

  // A set holding all distinguished consts in currently/last parsed fuction.
  std::set<std::string> local_scalar_constant_cache_;

  // A set holding all distinguished consts with global scope
  std::set<std::string> global_scalar_constant_cache_;

  // A vector holding string literals used outside a function (global scope)
  GlobalStringLiteralList global_stringliteral_list_;

  // A vector holding string literals used inside a function (local scope)
  // and their location.
  LocalStringLiteralList local_stringliteral_list_;
};

#endif	// COMUT_INFORMATION_VISITOR_H_