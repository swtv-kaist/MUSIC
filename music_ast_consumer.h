#ifndef MUSIC_AST_CONSUMER_H_
#define MUSIC_AST_CONSUMER_H_  

#include <vector>
#include <string>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"

#include "mutation_operators/expr_mutant_operator.h"
#include "mutation_operators/stmt_mutant_operator.h"
#include "music_context.h"

class MusicASTVisitor : public clang::RecursiveASTVisitor<MusicASTVisitor>
{
private:
  clang::SourceManager &src_mgr_;
  clang::CompilerInstance *comp_inst_;
  clang::Rewriter rewriter_;

  int proteumstyle_stmt_end_line_num_;

  ScopeRangeList scope_list_;

  // Range of the latest parsed array declaration statement
  clang::SourceRange *array_decl_range_;

  // The following range are used to prevent certain uncompilable mutations
  clang::SourceRange *functionprototype_range_;  // Type FunctionName(params);

  SwitchStmtInfoList switchstmt_info_list_;

  ScalarReferenceNameList non_VTWD_mutatable_scalarref_list_;

  MusicContext &context_;
  StmtContext &stmt_context_;
  std::vector<StmtMutantOperator*> &stmt_mutant_operator_list_;
  std::vector<ExprMutantOperator*> &expr_mutant_operator_list_;

  void UpdateAddressOfRange(clang::UnaryOperator *uo, 
                            clang::SourceLocation *start_loc, 
                            clang::SourceLocation *end_loc);

  /**
    @param  scalarref_name: string of name of a declaration reference
    @return True if reference name is not in the prohibited list
        False otherwise
  */
  bool IsScalarRefMutatableByVtwd(std::string scalarref_name);

  /**
    if there are addition of multiple scalar reference
    then only mutate one, put all the other inside the nonMutatableList
  
    @param  e: pointer to an expression
        exclude_last_scalarref: boolean variable. 
            True if the function should exclude one reference 
            for application of VTWD.
            False if the function should collect all reference possible. 
    @return True if a scalar reference is excluded 
        (VTWD can apply to that ref)
        False otherwise
  */
  bool CollectNonVtwdMutatableScalarRef(clang::Expr *e, bool exclude_last_scalarref);

  void HandleUnaryOperatorExpr(clang::UnaryOperator *uo);

  void HandleBinaryOperatorExpr(clang::Expr *e);

public:
  MusicASTVisitor(clang::CompilerInstance *CI, 
                  LabelStmtToGotoStmtListMap *label_to_gotolist_map, 
                  std::vector<StmtMutantOperator*> &stmt_operator_list,
                  std::vector<ExprMutantOperator*> &expr_operator_list,
                  MusicContext &context);

  bool VisitStmt(clang::Stmt *s);
  bool VisitCompoundStmt(clang::CompoundStmt *c);
  bool VisitSwitchStmt(clang::SwitchStmt *ss);
  bool VisitSwitchCase(clang::SwitchCase *sc);
  bool VisitExpr(clang::Expr *e);
  bool VisitEnumDecl(clang::EnumDecl *ed);
  bool VisitTypedefDecl(clang::TypedefDecl *td);
  bool VisitFieldDecl(clang::FieldDecl *fd);
  bool VisitVarDecl(clang::VarDecl *vd);
  bool VisitFunctionDecl(clang::FunctionDecl *f);
};

class MusicASTConsumer : public clang::ASTConsumer
{
public:
  MusicASTConsumer(clang::CompilerInstance *CI, 
                   LabelStmtToGotoStmtListMap *label_to_gotolist_map, 
                   std::vector<StmtMutantOperator*> &stmt_operator_list,
                   std::vector<ExprMutantOperator*> &expr_operator_list,
                   MusicContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Context);

private:
  MusicASTVisitor Visitor;
};

#endif    // MUSIC_AST_CONSUMER_H_  