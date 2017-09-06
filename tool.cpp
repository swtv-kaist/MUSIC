#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <utility>
#include <iomanip>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <set>
#include <cctype>
#include <limits.h>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Frontend/Utils.h"

#include "clang/Sema/Scope.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/ScopeInfo.h"

#include "comut_utility.h"
#include "configuration.h"

#include "comut_context.h"
#include "information_visitor.h"
#include "information_gatherer.h"

#include "mutation_operators/mutant_operator_template.h"
#include "mutation_operators/expr_mutant_operator.h"
#include "mutation_operators/stmt_mutant_operator.h"
#include "mutation_operators/ssdl.h"
#include "mutation_operators/orrn.h"
#include "mutation_operators/vtwf.h"
#include "mutation_operators/crcr.h"
#include "mutation_operators/sanl.h"
#include "mutation_operators/srws.h"
#include "mutation_operators/scsr.h"
#include "mutation_operators/vlsf.h"
#include "mutation_operators/vgsf.h"
#include "mutation_operators/vltf.h"
#include "mutation_operators/vgtf.h"
#include "mutation_operators/vlpf.h"
#include "mutation_operators/vgpf.h"
#include "mutation_operators/vgsr.h"
#include "mutation_operators/vlsr.h"
#include "mutation_operators/vgar.h"
#include "mutation_operators/vlar.h"
#include "mutation_operators/vgtr.h"
#include "mutation_operators/vltr.h"
#include "mutation_operators/vgpr.h"
#include "mutation_operators/vlpr.h"
#include "mutation_operators/vtwd.h"
#include "mutation_operators/vscr.h"
#include "mutation_operators/cgcr.h"
#include "mutation_operators/clcr.h"
#include "mutation_operators/cgsr.h"
#include "mutation_operators/clsr.h"
#include "mutation_operators/oppo.h"
#include "mutation_operators/ommo.h"
#include "mutation_operators/olng.h"
#include "mutation_operators/obng.h"
#include "mutation_operators/ocng.h"
#include "mutation_operators/oipm.h"
#include "mutation_operators/ocor.h"
#include "mutation_operators/olln.h"
#include "mutation_operators/ossn.h"
#include "mutation_operators/obbn.h"
#include "mutation_operators/olrn.h"
#include "mutation_operators/orln.h"
#include "mutation_operators/obln.h"
#include "mutation_operators/obrn.h"
#include "mutation_operators/osln.h"
#include "mutation_operators/osrn.h"
#include "mutation_operators/oban.h"
#include "mutation_operators/obsn.h"
#include "mutation_operators/osan.h"
#include "mutation_operators/osbn.h"
#include "mutation_operators/oaea.h"
#include "mutation_operators/obaa.h"
#include "mutation_operators/obba.h"
#include "mutation_operators/obea.h"
#include "mutation_operators/obsa.h"
#include "mutation_operators/osaa.h"
#include "mutation_operators/osba.h"
#include "mutation_operators/osea.h"
#include "mutation_operators/ossa.h"
#include "mutation_operators/oeaa.h"
#include "mutation_operators/oeba.h"
#include "mutation_operators/oesa.h"
#include "mutation_operators/oaaa.h"
#include "mutation_operators/oaba.h"
#include "mutation_operators/oasa.h"
#include "mutation_operators/oaln.h"
#include "mutation_operators/oaan.h"
#include "mutation_operators/oarn.h"
#include "mutation_operators/oabn.h"
#include "mutation_operators/oasn.h"
#include "mutation_operators/olan.h"
#include "mutation_operators/oran.h"
#include "mutation_operators/olbn.h"
#include "mutation_operators/olsn.h"
#include "mutation_operators/orsn.h"
#include "mutation_operators/orbn.h"

enum class UserInputAnalyzingState
{
  // expecting any not-A-B option
  kNonAOrBOption,

  // expecting a mutant operator name
  kMutantName,

  // expecting a mutant operator name, or another not-A-B option
  kNonAOrBOptionAndMutantName,

  // expecting a directory
  kOutputDir,

  // expecting a number
  kRsLine, kRsColumn,
  kReLine, kReColumn,
  kLimitNumOfMutant,

  // expecting a mutant name or any option
  kAnyOptionAndMutantName,

  // expecting either mutant name or any not-A option
  kNonAOptionAndMutantName,

  // expecting domain for previous mutant operator
  kDomainOfMutantOperator,

  // expecting range for previous mutant operator
  kRangeOfMutantOperator,

  // A_SDL_LINE, A_SDL_COL,  // expecting a number
  // B_SDL_LINE, B_SDL_COL,  // expecting a number
};

// return the first non-ParenExpr inside this Expr e
Expr* IgnoreParenExpr(Expr *e)
{
  Expr *ret = e;

  if (isa<ParenExpr>(ret))
  {
    ParenExpr *pe;

    while (pe = dyn_cast<ParenExpr>(ret))
      ret = pe->getSubExpr()->IgnoreImpCasts();
  }
  
  return ret;
}

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
private:
  SourceManager &src_mgr_;
  CompilerInstance *comp_inst_;
  Rewriter rewriter_;

  int proteumstyle_stmt_end_line_num_;

  ScopeRangeList scope_list_;

  // Range of the latest parsed array declaration statement
  SourceRange *array_decl_range_;

  // The following range are used to prevent certain uncompilable mutations
  SourceRange *functionprototype_range_;  // Type FunctionName(params);

  SwitchStmtInfoList switchstmt_info_list_;

  ScalarReferenceNameList non_VTWD_mutatable_scalarref_list_;

  ComutContext &context_;
  StmtContext &stmt_context_;
  vector<StmtMutantOperator*> &stmt_mutant_operator_list_;
  vector<ExprMutantOperator*> &expr_mutant_operator_list_;

  void UpdateAddressOfRange(UnaryOperator *uo, SourceLocation *start_loc, SourceLocation *end_loc)
  {
    Expr *sub_expr_of_unaryop = uo->getSubExpr()->IgnoreImpCasts();

    if (isa<ParenExpr>(sub_expr_of_unaryop))
    {
      ParenExpr *pe;

      while (pe = dyn_cast<ParenExpr>(sub_expr_of_unaryop))
        sub_expr_of_unaryop = pe->getSubExpr()->IgnoreImpCasts();
    }

    if (ExprIsDeclRefExpr(sub_expr_of_unaryop) ||
      ExprIsPointerDereferenceExpr(sub_expr_of_unaryop) ||
      isa<MemberExpr>(sub_expr_of_unaryop) ||
      isa<ArraySubscriptExpr>(sub_expr_of_unaryop))
    {
      stmt_context_.setAddressOpRange(new SourceRange(*start_loc, *end_loc));
    }
  }

  /**
    @param  scalarref_name: string of name of a declaration reference
    @return True if reference name is not in the prohibited list
        False otherwise
  */
  bool IsScalarRefMutatableByVtwd(string scalarref_name)
  {
    // if reference name is in the nonMutatableList then it is not mutatable
    for (auto it = non_VTWD_mutatable_scalarref_list_.begin(); 
          it != non_VTWD_mutatable_scalarref_list_.end(); ++it)
    {
      if (scalarref_name.compare(*it) == 0)
        return false;
    }

    return true;
  }

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
  bool CollectNonVtwdMutatableScalarRef(Expr *e, bool exclude_last_scalarref)
  {
    bool scalarref_excluded{false};

    if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
    {
      if (bo->isAdditiveOp())
      {
        Expr *rhs = bo->getRHS()->IgnoreImpCasts();
        Expr *lhs = bo->getLHS()->IgnoreImpCasts();

        if (!exclude_last_scalarref)  // collect all references possible
        {
          if (ExprIsScalarReference(rhs))
          {
            string reference_name{rewriter_.ConvertToString(rhs)};

            // if this scalar reference is mutatable then block it
            if (IsScalarRefMutatableByVtwd(reference_name))
              non_VTWD_mutatable_scalarref_list_.push_back(reference_name);
          }
          else if (ParenExpr *pe = dyn_cast<ParenExpr>(rhs))
            CollectNonVtwdMutatableScalarRef(pe->getSubExpr(), false);
          else
            ;   // do nothing

          if (ExprIsScalarReference(lhs))
          {
            string reference_name{rewriter_.ConvertToString(lhs)};

            // if this scalar reference is mutatable then block it
            if (IsScalarRefMutatableByVtwd(reference_name))
              non_VTWD_mutatable_scalarref_list_.push_back(reference_name);
          }
          else if (ParenExpr *pe = dyn_cast<ParenExpr>(lhs))
            CollectNonVtwdMutatableScalarRef(pe->getSubExpr(), false);
          else
            ;   // do nothing
        }
        else  // have to exclude 1 scalar reference for VTWD
        {
          if (ExprIsScalarReference(rhs))
          {
            scalarref_excluded = true;
          }
          else if (ParenExpr *pe = dyn_cast<ParenExpr>(rhs))
          {
            if (CollectNonVtwdMutatableScalarRef(pe->getSubExpr(), true))
              scalarref_excluded = true;
          }
          else
            ;

          if (ExprIsScalarReference(lhs))
          {
            if (scalarref_excluded)
            {
              string reference_name{rewriter_.ConvertToString(lhs)};

              // if this scalar reference is mutatable then block it
              if (IsScalarRefMutatableByVtwd(reference_name))
                non_VTWD_mutatable_scalarref_list_.push_back(reference_name);
            }
            else
              scalarref_excluded = true;
          }
          else if (ParenExpr *pe = dyn_cast<ParenExpr>(lhs))
          {
            if (scalarref_excluded)
              CollectNonVtwdMutatableScalarRef(pe->getSubExpr(), false);
            else
            {
              if (CollectNonVtwdMutatableScalarRef(pe->getSubExpr(), true))
                scalarref_excluded = true;
            }
          }
          else
            ;
        }
      }
    }

    return scalarref_excluded;
  }

  void HandleUnaryOperatorExpr(UnaryOperator *uo)
  {
    SourceLocation start_loc = uo->getLocStart();
    SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, comp_inst_);

    // Retrieve the range of UnaryOperator address-of
    // to prevent getting-address-of-a-register error.
    if (uo->getOpcode() == UO_AddrOf)
    {
      UpdateAddressOfRange(uo, &start_loc, &end_loc);
    }
    else if (uo->getOpcode() == UO_PostInc || uo->getOpcode() == UO_PreInc ||
             uo->getOpcode() == UO_PostDec || uo->getOpcode() == UO_PreDec)
    {
      stmt_context_.setUnaryIncrementDecrementRange(
          new SourceRange(start_loc, end_loc));
    }
  }

  void HandleBinaryOperatorExpr(Expr *e)
  {
    BinaryOperator *bo = cast<BinaryOperator>(e);

    // Retrieve the location and the operator of the expression
    string binary_operator = static_cast<string>(bo->getOpcodeStr());
    SourceLocation start_loc = bo->getOperatorLoc();
    SourceLocation end_loc = src_mgr_.translateLineCol(
        src_mgr_.getMainFileID(),
        GetLineNumber(src_mgr_, start_loc),
        GetColumnNumber(src_mgr_, start_loc) + binary_operator.length());

    // Certain mutations are NOT syntactically correct for left side of 
    // assignment. Store location for prevention of generating 
    // uncompilable mutants.
    if (bo->isAssignmentOp())
    {
      stmt_context_.setLhsOfAssignmentRange(
          new SourceRange(bo->getLHS()->getLocStart(), start_loc));
    }

    // Setting up for prevent redundant VTWD mutants
    // if A and B are both scalar reference then
    // (A+B) should only be mutated to (A+B+1),
    // not both (A+B+1) and (A+1+B).
    // Store the name of A so that COMUT do not mutate it.
    if (bo->isAdditiveOp())
      CollectNonVtwdMutatableScalarRef(e, true);

    // Modulo, shift and bitwise expressions' values are integral,
    // and they also only take integral operands.
    // So OCOR should not mutate any cast inside these expr to float type
    if (binary_operator.compare("%") == 0 || bo->isBitwiseOp() || 
        bo->isShiftOp()) 
    {
      // Setting up for blocking uncompilable mutants for OCOR
      if (!LocationIsInRange(
          bo->getLocStart(), *(stmt_context_.getNonFloatingExprRange())))
      {
        stmt_context_.setNonFloatingExprRange(new SourceRange(
            bo->getLocStart(), GetEndLocOfExpr(e, comp_inst_)));
      }        
    }
  }

public:
  MyASTVisitor(CompilerInstance *CI, 
               LabelStmtToGotoStmtListMap *label_to_gotolist_map, 
               vector<StmtMutantOperator*> &stmt_operator_list,
               vector<ExprMutantOperator*> &expr_operator_list,
               ComutContext &context) 
    : src_mgr_(CI->getSourceManager()),
      comp_inst_(CI), context_(context), stmt_context_(context.getStmtContext()),
      stmt_mutant_operator_list_(stmt_operator_list),
      expr_mutant_operator_list_(expr_operator_list)
  {
    proteumstyle_stmt_end_line_num_ = 0;

    // setup for rewriter
    rewriter_.setSourceMgr(src_mgr_, CI->getLangOpts());

    // set range variables to a 0 range, start_loc and end_loc at the same point
    SourceLocation start_of_file;
    start_of_file = src_mgr_.getLocForStartOfFile(src_mgr_.getMainFileID());

    functionprototype_range_ = new SourceRange(start_of_file, start_of_file);

    array_decl_range_ = new SourceRange(start_of_file, start_of_file);

    context_.switchstmt_info_list = &switchstmt_info_list_;
    context_.non_VTWD_mutatable_scalarref_list = &non_VTWD_mutatable_scalarref_list_;

    context_.scope_list_ = &scope_list_;
  }

  bool VisitEnumDecl(EnumDecl *ed)
  {
    stmt_context_.setIsInEnumDecl(true);
    return true;
  }

  bool VisitTypedefDecl(TypedefDecl *td)
  {
    stmt_context_.setTypedefDeclRange(
        new SourceRange(td->getLocStart(), td->getLocEnd()));

    return true;
  }

  bool VisitExpr(Expr *e)
  {
    // Do not mutate or consider anything inside a typedef definition
    if (LocationIsInRange(
        e->getLocStart(), *(stmt_context_.getTypedefRange())))
      return true;

    for (auto mutant_operator: expr_mutant_operator_list_)
      if (mutant_operator->CanMutate(e, &context_))
        mutant_operator->Mutate(e, &context_);

    if (StmtExpr *se = dyn_cast<StmtExpr>(e))  
    {
      // set boolean variable signals following stmt are inside stmt expr
      stmt_context_.setIsInStmtExpr(true);
    }
    else if (ArraySubscriptExpr *ase = dyn_cast<ArraySubscriptExpr>(e))
    {
      SourceLocation start_loc = ase->getLocStart();
      SourceLocation end_loc = GetEndLocOfStmt(ase->getLocEnd(), comp_inst_);

      stmt_context_.setArraySubscriptRange(new SourceRange(start_loc, end_loc));
    }
    else if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
      HandleUnaryOperatorExpr(uo);
    else if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e)) 
      HandleBinaryOperatorExpr(e);

    return true;
  }

  bool VisitCompoundStmt(CompoundStmt *c)
  {
    // entering a new scope
    scope_list_.push_back(SourceRange(c->getLocStart(), c->getLocEnd()));

    return true;
  }

  bool VisitFieldDecl(FieldDecl *fd)
  {
    SourceLocation start_loc = fd->getLocStart();
    SourceLocation end_loc = fd->getLocEnd();

    stmt_context_.setFieldDeclRange(new SourceRange(start_loc, end_loc));
    
    if (fd->getType().getTypePtr()->isArrayType())
    {
      stmt_context_.setIsInArrayDeclSize(true);

      array_decl_range_ = new SourceRange(fd->getLocStart(), fd->getLocEnd());
    }
    return true;
  }

  bool VisitVarDecl(VarDecl *vd)
  {
    SourceLocation start_loc = vd->getLocStart();
    SourceLocation end_loc = vd->getLocEnd();

    stmt_context_.setIsInEnumDecl(false);

    if (LocationIsInRange(start_loc, *(stmt_context_.getTypedefRange())))
      return true;

    if (LocationIsInRange(start_loc, *functionprototype_range_))
      return true;

    if (IsVarDeclArray(vd))
    {
      auto type = vd->getType().getCanonicalType().getTypePtr();

      if (auto array_type =  dyn_cast_or_null<ConstantArrayType>(type)) 
      {  
        stmt_context_.setIsInArrayDeclSize(true);

        array_decl_range_ = new SourceRange(start_loc, end_loc);
      }
    }

    return true;
  }

  bool VisitSwitchStmt(SwitchStmt *ss)
  {
    stmt_context_.setSwitchStmtConditionRange(
        new SourceRange(ss->getSwitchLoc(), ss->getBody()->getLocStart()));

    // remove switch statements that are already passed
    while (!switchstmt_info_list_.empty() && 
           !LocationIsInRange(
               ss->getLocStart(), switchstmt_info_list_.back().first))
      switchstmt_info_list_.pop_back();

    vector<string> case_value_list;
    SwitchCase *sc = ss->getSwitchCaseList();

    // collect all case values' strings
    while (sc != nullptr)
    {
      if (isa<DefaultStmt>(sc))
      {
        sc = sc->getNextSwitchCase();
        continue;
      }

      // retrieve location before 'case'
      SourceLocation keyword_loc = sc->getKeywordLoc();  
      SourceLocation colon_loc = sc->getColonLoc();
      string case_value{""};

      // retrieve case label starting from after 'case'
      SourceLocation location_iterator = keyword_loc.getLocWithOffset(4);

      // retrieve string from after 'case' to before ':'
      while (location_iterator != colon_loc)
      {
        case_value += *(src_mgr_.getCharacterData(location_iterator));
        location_iterator = location_iterator.getLocWithOffset(1);
      }

      // remove whitespaces at the beginning and end_loc of retrieved string
      case_value = TrimBeginningAndEndingWhitespace(case_value);

      // if case value is char, convert it to int value
      if (case_value.front() == '\'' && case_value.back() == '\'')
        case_value = ConvertCharStringToIntString(case_value);

      case_value_list.push_back(case_value);

      sc = sc->getNextSwitchCase();
    }

    switchstmt_info_list_.push_back(
        make_pair(SourceRange(ss->getLocStart(), ss->getLocEnd()), 
                  case_value_list));

    return true;
  }

  bool VisitSwitchCase(SwitchCase *sc)
  {
    stmt_context_.setSwitchCaseRange(
        new SourceRange(sc->getLocStart(), sc->getColonLoc()));
    
    // remove switch statements that are already passed
    while (!switchstmt_info_list_.empty() && 
           !LocationIsInRange(
               sc->getLocStart(), switchstmt_info_list_.back().first))
      switchstmt_info_list_.pop_back();

    return true;
  }

  bool VisitStmt(Stmt *s)
  {
    SourceLocation start_loc = s->getLocStart();
    SourceLocation end_loc = s->getLocEnd();

    // set up Proteum-style line number
    if (GetLineNumber(src_mgr_, start_loc) > proteumstyle_stmt_end_line_num_)
    {
      stmt_context_.setProteumStyleLineNum(GetLineNumber(
          src_mgr_, start_loc));
      
      if (isa<IfStmt>(s) || isa<WhileStmt>(s) || isa<SwitchStmt>(s)) 
      {
        SourceLocation end_loc_of_stmt = end_loc;

        if (IfStmt *is = dyn_cast<IfStmt>(s))
          end_loc_of_stmt = is->getCond()->getLocEnd();
        else if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
          end_loc_of_stmt = ws->getCond()->getLocEnd();
        else if (SwitchStmt *ss = dyn_cast<SwitchStmt>(s))
          end_loc_of_stmt = ss->getCond()->getLocEnd();

        while (*(src_mgr_.getCharacterData(end_loc_of_stmt)) != '\n' && 
               *(src_mgr_.getCharacterData(end_loc_of_stmt)) != '{')
          end_loc_of_stmt = end_loc_of_stmt.getLocWithOffset(1);

        proteumstyle_stmt_end_line_num_ = GetLineNumber(src_mgr_, end_loc_of_stmt);
      }
      else if (isa<CompoundStmt>(s) || isa<LabelStmt>(s) || isa<DoStmt>(s) || 
               isa<SwitchCase>(s) || isa<ForStmt>(s))
        proteumstyle_stmt_end_line_num_ = stmt_context_.getProteumStyleLineNum();
      else
        proteumstyle_stmt_end_line_num_ = GetLineNumber(src_mgr_, end_loc);
    }

    if (stmt_context_.IsInArrayDeclSize() && 
        !LocationIsInRange(start_loc, *array_decl_range_))
    {
      stmt_context_.setIsInArrayDeclSize(false);
    }

    for (auto mutant_operator: stmt_mutant_operator_list_)
      if (mutant_operator->CanMutate(s, &context_))
        mutant_operator->Mutate(s, &context_);

    return true;
  }
  
  bool VisitFunctionDecl(FunctionDecl *f) 
  {   
    // Function with nobody, and function declaration within 
    // another function is function prototype.
    // no global or local variable mutation will be applied here.
    if (!f->hasBody() || LocationIsInRange(
        f->getLocStart(), *(stmt_context_.getCurrentlyParsedFunctionRange())))
    {
      functionprototype_range_ = new SourceRange(f->getLocStart(), 
                                                 f->getLocEnd());
    }
    else
    {
      // entering a new local scope
      scope_list_.clear();
      scope_list_.push_back(SourceRange(f->getLocStart(), f->getLocEnd()));

      context_.IncrementFunctionId();

      stmt_context_.setIsInEnumDecl(false);

      stmt_context_.setCurrentlyParsedFunctionRange(
          new SourceRange(f->getLocStart(), f->getLocEnd()));
    }

    return true;
  }
};

class MyASTConsumer : public ASTConsumer
{
public:
  MyASTConsumer(CompilerInstance *CI, 
                LabelStmtToGotoStmtListMap *label_to_gotolist_map, 
                vector<StmtMutantOperator*> &stmt_operator_list,
                vector<ExprMutantOperator*> &expr_operator_list,
                ComutContext &context)
    : Visitor(CI, label_to_gotolist_map,
              stmt_operator_list, expr_operator_list, context) 
  { 
  }

  virtual void HandleTranslationUnit(ASTContext &Context)
  {
    /* we can use ASTContext to get the TranslationUnitDecl, which is
    a single Decl that collectively represents the entire source file */
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

private:
  MyASTVisitor Visitor;
};

InformationGatherer* GetNecessaryDataFromInputFile(char *filename)
{
  // CompilerInstance will hold the instance of the Clang compiler for us,
  // managing the various objects needed to run the compiler.
  CompilerInstance TheCompInst2;
  
  // Diagnostics manage problems and issues in compile 
  TheCompInst2.createDiagnostics(NULL, false);

  // Set target platform options 
  // Initialize target info with the default triple for our platform.
  TargetOptions *TO2 = new TargetOptions();
  TO2->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI2 = TargetInfo::CreateTargetInfo(TheCompInst2.getDiagnostics(), 
                                                TO2);
  TheCompInst2.setTarget(TI2);

  // FileManager supports for file system lookup, file system caching, 
  // and directory search management.
  TheCompInst2.createFileManager();
  FileManager &FileMgr2 = TheCompInst2.getFileManager();
  
  // SourceManager handles loading and caching of source files into memory.
  TheCompInst2.createSourceManager(FileMgr2);
  SourceManager &SourceMgr2 = TheCompInst2.getSourceManager();
  
  // Prreprocessor runs within a single source file
  TheCompInst2.createPreprocessor();
  
  // ASTContext holds long-lived AST nodes (such as types and decls) .
  TheCompInst2.createASTContext();

  // Enable HeaderSearch option
  llvm::IntrusiveRefCntPtr<clang::HeaderSearchOptions> hso2(
      new HeaderSearchOptions());
  HeaderSearch headerSearch2(hso2,
                            TheCompInst2.getFileManager(),
                            TheCompInst2.getDiagnostics(),
                            TheCompInst2.getLangOpts(),
                            TI2);

  // <Warning!!> -- Platform Specific Code lives here
  // This depends on A) that you're running linux and
  // B) that you have the same GCC LIBs installed that I do. 
  /*
  $ gcc -xc -E -v -
  ..
   /usr/local/include
   /usr/lib/gcc/x86_64-linux-gnu/4.4.5/include
   /usr/lib/gcc/x86_64-linux-gnu/4.4.5/include-fixed
   /usr/include
  End of search list.
  */
  const char *include_paths2[] = {"/usr/local/include",
        "/usr/lib/gcc/x86_64-linux-gnu/4.4.6/include",
        "/usr/lib/gcc/x86_64-linux-gnu/4.4.6/include-fixed",
        "/usr/include"};

  for (int i=0; i<4; i++) 
    hso2->AddPath(include_paths2[i], 
          clang::frontend::Angled, 
          false, 
          false);
  // </Warning!!> -- End of Platform Specific Code

  InitializePreprocessor(TheCompInst2.getPreprocessor(), 
                         TheCompInst2.getPreprocessorOpts(),
                         *hso2,
                         TheCompInst2.getFrontendOpts());

  // Set the main file handled by the source manager to the input file.
  const FileEntry *FileIn2 = FileMgr2.getFile(filename);
  SourceMgr2.createMainFileID(FileIn2);
  
  // Inform Diagnostics that processing of a source file is beginning. 
  TheCompInst2.getDiagnosticClient().BeginSourceFile(
      TheCompInst2.getLangOpts(),&TheCompInst2.getPreprocessor());

  // Parse the file to AST, gather labelstmts, goto stmts, 
  // scalar constants, string literals. 
  InformationGatherer *TheGatherer = new InformationGatherer(&TheCompInst2);

  ParseAST(TheCompInst2.getPreprocessor(), TheGatherer, 
           TheCompInst2.getASTContext());

  return TheGatherer;
}

void AddMutantOperator(string mutant_name, 
                       set<string> &domain, set<string> &range, 
                       vector<StmtMutantOperator*> &stmt_operator_list,
                       vector<ExprMutantOperator*> &expr_operator_list)
{
  // Make appropriate MutantOperator based on name
  // Verifiy and set domain and range accordingly

  StmtMutantOperator *new_stmt_operator = nullptr;

  if (mutant_name.compare("SSDL") == 0)
    new_stmt_operator = new SSDL();
  else if (mutant_name.compare("OCNG") == 0)
    new_stmt_operator = new OCNG();

  if (new_stmt_operator != nullptr)
  {
    // Set domain for mutant operator if domain is valid
    if (!new_stmt_operator->ValidateDomain(domain))
    {
      cout << "invalid domain\n";
      exit(1);
    }
    else
      new_stmt_operator->setDomain(domain);

    // Set range for mutant operator if range is valid
    if (!new_stmt_operator->ValidateRange(range))
    {
      cout << "invalid range\n";
      exit(1);
    }
    else
      new_stmt_operator->setRange(range);

    stmt_operator_list.push_back(new_stmt_operator);
    return;
  }

  ExprMutantOperator *new_expr_operator = nullptr;

  if (mutant_name.compare("ORRN") == 0)
    new_expr_operator = new ORRN();
  else if (mutant_name.compare("VTWF") == 0)
    new_expr_operator = new VTWF();
  else if (mutant_name.compare("CRCR") == 0)
    new_expr_operator = new CRCR();
  else if (mutant_name.compare("SANL") == 0)
    new_expr_operator = new SANL();
  else if (mutant_name.compare("SRWS") == 0)
    new_expr_operator = new SRWS();
  else if (mutant_name.compare("SCSR") == 0)
    new_expr_operator = new SCSR();
  else if (mutant_name.compare("VLSF") == 0)
    new_expr_operator = new VLSF();
  else if (mutant_name.compare("VGSF") == 0)
    new_expr_operator = new VGSF();
  else if (mutant_name.compare("VLTF") == 0)
    new_expr_operator = new VLTF();
  else if (mutant_name.compare("VGTF") == 0)
    new_expr_operator = new VGTF();
  else if (mutant_name.compare("VLPF") == 0)
    new_expr_operator = new VLPF();
  else if (mutant_name.compare("VGPF") == 0)
    new_expr_operator = new VGPF();
  else if (mutant_name.compare("VGSR") == 0)
    new_expr_operator = new VGSR();
  else if (mutant_name.compare("VLSR") == 0)
    new_expr_operator = new VLSR();
  else if (mutant_name.compare("VGAR") == 0)
    new_expr_operator = new VGAR();
  else if (mutant_name.compare("VLAR") == 0)
    new_expr_operator = new VLAR();
  else if (mutant_name.compare("VGTR") == 0)
    new_expr_operator = new VGTR();
  else if (mutant_name.compare("VLTR") == 0)
    new_expr_operator = new VLTR();
  else if (mutant_name.compare("VGPR") == 0)
    new_expr_operator = new VGPR();
  else if (mutant_name.compare("VLPR") == 0)
    new_expr_operator = new VLPR();
  else if (mutant_name.compare("VTWD") == 0)
    new_expr_operator = new VTWD();
  else if (mutant_name.compare("VSCR") == 0)
    new_expr_operator = new VSCR();
  else if (mutant_name.compare("CGCR") == 0)
    new_expr_operator = new CGCR();
  else if (mutant_name.compare("CLCR") == 0)
    new_expr_operator = new CLCR();
  else if (mutant_name.compare("CGSR") == 0)
    new_expr_operator = new CGSR();
  else if (mutant_name.compare("CLSR") == 0)
    new_expr_operator = new CLSR();
  else if (mutant_name.compare("OPPO") == 0)
    new_expr_operator = new OPPO();
  else if (mutant_name.compare("OMMO") == 0)
    new_expr_operator = new OMMO();
  else if (mutant_name.compare("OLNG") == 0)
    new_expr_operator = new OLNG();
  else if (mutant_name.compare("OBNG") == 0)
    new_expr_operator = new OBNG();
  else if (mutant_name.compare("OIPM") == 0)
    new_expr_operator = new OIPM();
  else if (mutant_name.compare("OCOR") == 0)
    new_expr_operator = new OCOR();
  else if (mutant_name.compare("OLLN") == 0)
    new_expr_operator = new OLLN();
  else if (mutant_name.compare("OSSN") == 0)
    new_expr_operator = new OSSN();
  else if (mutant_name.compare("OBBN") == 0)
    new_expr_operator = new OBBN();
  else if (mutant_name.compare("OLRN") == 0)
    new_expr_operator = new OLRN();
  else if (mutant_name.compare("ORLN") == 0)
    new_expr_operator = new ORLN();
  else if (mutant_name.compare("OBLN") == 0)
    new_expr_operator = new OBLN();
  else if (mutant_name.compare("OBRN") == 0)
    new_expr_operator = new OBRN();
  else if (mutant_name.compare("OSLN") == 0)
    new_expr_operator = new OSLN();
  else if (mutant_name.compare("OSRN") == 0)
    new_expr_operator = new OSRN();
  else if (mutant_name.compare("OBAN") == 0)
    new_expr_operator = new OBAN();
  else if (mutant_name.compare("OBSN") == 0)
    new_expr_operator = new OBSN();
  else if (mutant_name.compare("OSAN") == 0)
    new_expr_operator = new OSAN();
  else if (mutant_name.compare("OSBN") == 0)
    new_expr_operator = new OSBN();
  else if (mutant_name.compare("OAEA") == 0)
    new_expr_operator = new OAEA();
  else if (mutant_name.compare("OBAA") == 0)
    new_expr_operator = new OBAA();
  else if (mutant_name.compare("OBBA") == 0)
    new_expr_operator = new OBBA();
  else if (mutant_name.compare("OBEA") == 0)
    new_expr_operator = new OBEA();
  else if (mutant_name.compare("OBSA") == 0)
    new_expr_operator = new OBSA();
  else if (mutant_name.compare("OSAA") == 0)
    new_expr_operator = new OSAA();
  else if (mutant_name.compare("OSBA") == 0)
    new_expr_operator = new OSBA();
  else if (mutant_name.compare("OSEA") == 0)
    new_expr_operator = new OSEA();
  else if (mutant_name.compare("OSSA") == 0)
    new_expr_operator = new OSSA();
  else if (mutant_name.compare("OEAA") == 0)
    new_expr_operator = new OEAA();
  else if (mutant_name.compare("OEBA") == 0)
    new_expr_operator = new OEBA();
  else if (mutant_name.compare("OESA") == 0)
    new_expr_operator = new OESA();
  else if (mutant_name.compare("OAAA") == 0)
    new_expr_operator = new OAAA();
  else if (mutant_name.compare("OABA") == 0)
    new_expr_operator = new OABA();
  else if (mutant_name.compare("OASA") == 0)
    new_expr_operator = new OASA();
  else if (mutant_name.compare("OALN") == 0)
    new_expr_operator = new OALN();
  else if (mutant_name.compare("OAAN") == 0)
    new_expr_operator = new OAAN();
  else if (mutant_name.compare("OARN") == 0)
    new_expr_operator = new OARN();
  else if (mutant_name.compare("OABN") == 0)
    new_expr_operator = new OABN();
  else if (mutant_name.compare("OASN") == 0)
    new_expr_operator = new OASN();
  else if (mutant_name.compare("OLAN") == 0)
    new_expr_operator = new OLAN();
  else if (mutant_name.compare("ORAN") == 0)
    new_expr_operator = new ORAN();
  else if (mutant_name.compare("OLBN") == 0)
    new_expr_operator = new OLBN();
  else if (mutant_name.compare("OLSN") == 0)
    new_expr_operator = new OLSN();
  else if (mutant_name.compare("ORSN") == 0)
    new_expr_operator = new ORSN();
  else if (mutant_name.compare("ORBN") == 0)
    new_expr_operator = new ORBN();
  else
  {
    cout << "Unknown mutant operator: " << mutant_name << endl;
    // exit(1);
    return;
  }

  if (new_expr_operator != nullptr)
  {
    // Set domain for mutant operator if domain is valid
    if (!new_expr_operator->ValidateDomain(domain))
    {
      cout << "invalid domain\n";
      exit(1);
    }
    else
      new_expr_operator->setDomain(domain);

    // Set range for mutant operator if range is valid
    if (!new_expr_operator->ValidateRange(range))
    {
      cout << "invalid range\n";
      exit(1);
    }
    else
      new_expr_operator->setRange(range);

    expr_operator_list.push_back(new_expr_operator);
  }
}

// Wrap up currently-entering mutant operator (if can)
// before change the state to kNonAOrBOption.
void clearState(UserInputAnalyzingState &state, string mutant_name, 
                set<string> &domain, set<string> &range,
                vector<StmtMutantOperator*> &stmt_operator_list,
                vector<ExprMutantOperator*> &expr_operator_list)
{
  switch (state)
  {
    case UserInputAnalyzingState::kAnyOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOrBOptionAndMutantName:
    {
      AddMutantOperator(mutant_name, domain, range, 
                        stmt_operator_list, expr_operator_list);

      domain.clear();
      range.clear();

      break;
    }
    case UserInputAnalyzingState::kNonAOrBOption:
      break;
    default:
      PrintUsageErrorMsg();
      exit(1);
  };

  state = UserInputAnalyzingState::kNonAOrBOption;
}

void HandleInput(string input, UserInputAnalyzingState &state, 
                 string &mutant_name, set<string> &domain,
                 set<string> &range, string &output_dir, int &limit, 
                 SourceLocation *start_loc, SourceLocation *end_loc, 
                 int &line_num, int &col_num, SourceManager &src_mgr, 
                 vector<StmtMutantOperator*> &stmt_operator_list,
                 vector<ExprMutantOperator*> &expr_operator_list)
{
  switch (state)
  {
    case UserInputAnalyzingState::kNonAOrBOption:
      PrintUsageErrorMsg();
      exit(1);

    case UserInputAnalyzingState::kMutantName:
      for (int i = 0; i < input.length() ; ++i)
      {
        if (input[i] >= 'a' && input[i] <= 'z')
          input[i] -= 32;
      }

      mutant_name.clear();
      mutant_name = input;

      state = UserInputAnalyzingState::kAnyOptionAndMutantName;
      break;

    case UserInputAnalyzingState::kOutputDir:
      output_dir.clear();
      output_dir = input + "/";
      
      if (!DirectoryExists(output_dir)) 
      {
        cout << "Invalid directory for -o option: " << output_dir << endl;
        exit(1);
      }

      state = UserInputAnalyzingState::kNonAOrBOption;
      break;

    case UserInputAnalyzingState::kRsLine:
      if (!ConvertStringToInt(input, line_num))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      if (line_num <= 0)
      {
        // cout << "Input line number is not positive. Default to 1.\n";
        // line_num = 1;
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      state = UserInputAnalyzingState::kRsColumn;
      break;

    case UserInputAnalyzingState::kRsColumn:
      if (!ConvertStringToInt(input, col_num))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      if (col_num <= 0)
      {
        // cout << "Input col number is not positive. Default to 1.\n";
        // col_num = 1;
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      *start_loc = src_mgr.translateLineCol(src_mgr.getMainFileID(), 
                                            line_num, col_num);
      state = UserInputAnalyzingState::kNonAOrBOption;
      break;

    case UserInputAnalyzingState::kReLine:
      if (!ConvertStringToInt(input, line_num))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      if (line_num <= 0)
      {
        // cout << "Input line number is not positive. Default to 1.\n";
        // line_num = 1;
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      state = UserInputAnalyzingState::kReColumn;
      break;

    case UserInputAnalyzingState::kReColumn:
      if (!ConvertStringToInt(input, col_num))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      if (col_num <= 0)
      {
        // cout << "Input col number is not positive. Default to 1.\n";
        // col_num = 1;
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      *end_loc = src_mgr.translateLineCol(src_mgr.getMainFileID(), 
                                          line_num, col_num);
      state = UserInputAnalyzingState::kNonAOrBOption;
      break;

    case UserInputAnalyzingState::kLimitNumOfMutant:
      if (!ConvertStringToInt(input, limit))
      {
        cout << "Invalid input for -l option, must be an positive integer smaller than 2147483648\nUsage: -l <max>\n";
        exit(1);
      }

      if (limit <= 0)
      {
        cout << "Invalid input for -l option, must be an positive integer smaller than 2147483648\nUsage: -l <max>\n";
        exit(1);
      }      

      state = UserInputAnalyzingState::kNonAOrBOption;
      break;

    case UserInputAnalyzingState::kAnyOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOrBOptionAndMutantName:
    {
      AddMutantOperator(mutant_name, domain, range, 
                        stmt_operator_list, expr_operator_list);

      state = UserInputAnalyzingState::kMutantName;
      domain.clear();
      range.clear();

      HandleInput(input, state, mutant_name, domain, range, 
                  output_dir, limit, start_loc, end_loc, line_num, 
                  col_num, src_mgr, stmt_operator_list,
                  expr_operator_list);
      break;
    }

    case UserInputAnalyzingState::kDomainOfMutantOperator:
      SplitStringIntoSet(input, domain, string(","));
      ValidateDomainOfMutantOperator(mutant_name, domain);
      state = UserInputAnalyzingState::kNonAOptionAndMutantName;
      break;

    case UserInputAnalyzingState::kRangeOfMutantOperator:
      SplitStringIntoSet(input, range, string(","));
      ValidateRangeOfMutantOperator(mutant_name, range);
      state = UserInputAnalyzingState::kNonAOrBOptionAndMutantName;
      break;

    default:
      // cout << "unknown state: " << state << endl;
      exit(1);
  };
}

int main(int argc, char *argv[])
{
  if (argc < 2) 
  {
    PrintUsageErrorMsg();
    return 1;
  }

  // CompilerInstance will hold the instance of the Clang compiler for us,
  // managing the various objects needed to run the compiler.
  CompilerInstance TheCompInst;
  
  // Diagnostics manage problems and issues in compile 
  TheCompInst.createDiagnostics(NULL, false);

  // Set target platform options 
  // Initialize target info with the default triple for our platform.
  TargetOptions *TO = new TargetOptions();
  TO->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI = TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), 
                                                TO);
  TheCompInst.setTarget(TI);

  // FileManager supports for file system lookup, file system caching, 
  // and directory search management.
  TheCompInst.createFileManager();
  FileManager &FileMgr = TheCompInst.getFileManager();
  
  // SourceManager handles loading and caching of source files into memory.
  TheCompInst.createSourceManager(FileMgr);
  SourceManager &SourceMgr = TheCompInst.getSourceManager();
  
  // Prreprocessor runs within a single source file
  TheCompInst.createPreprocessor();
  
  // ASTContext holds long-lived AST nodes (such as types and decls) .
  TheCompInst.createASTContext();

  // Enable HeaderSearch option
  llvm::IntrusiveRefCntPtr<clang::HeaderSearchOptions> hso( 
      new HeaderSearchOptions());
  HeaderSearch headerSearch(hso,
                            TheCompInst.getFileManager(),
                            TheCompInst.getDiagnostics(),
                            TheCompInst.getLangOpts(),
                            TI);

  // <Warning!!> -- Platform Specific Code lives here
  // This depends on A) that you're running linux and
  // B) that you have the same GCC LIBs installed that I do. 
  /*
  $ gcc -xc -E -v -
  ..
   /usr/local/include
   /usr/lib/gcc/x86_64-linux-gnu/4.4.5/include
   /usr/lib/gcc/x86_64-linux-gnu/4.4.5/include-fixed
   /usr/include
  End of search list.
  */
  const char *include_paths[] = {"/usr/local/include",
      "/usr/lib/gcc/x86_64-linux-gnu/4.4.6/include",
      "/usr/lib/gcc/x86_64-linux-gnu/4.4.6/include-fixed",
      "/usr/include"};

  for (int i=0; i<4; i++) 
    hso->AddPath(include_paths[i], clang::frontend::Angled, 
                 false, false);
  // </Warning!!> -- End of Platform Specific Code

  InitializePreprocessor(TheCompInst.getPreprocessor(), 
                         TheCompInst.getPreprocessorOpts(),
                         *hso,
                         TheCompInst.getFrontendOpts());

  // Set the main file Handled by the source manager to the input file.
  const FileEntry *FileIn = FileMgr.getFile(argv[1]);
  SourceMgr.createMainFileID(FileIn);
  
  // Inform Diagnostics that processing of a source file is beginning. 
  TheCompInst.getDiagnosticClient().BeginSourceFile(
      TheCompInst.getLangOpts(),&TheCompInst.getPreprocessor());

  //=======================================================
  //================ USER INPUT ANALYSIS ==================
  //=======================================================
  
  // default output directory is current directory.
  string output_dir = "./";

  // by default, as many mutants will be generated 
  // at a location per mutant operator as possible.
  int limit = INT_MAX;

  // start_loc and end_loc of mutation range by default is 
  // start and end of file
  SourceLocation start_of_mutation_range = SourceMgr.getLocForStartOfFile(
      SourceMgr.getMainFileID());
  SourceLocation end_of_mutation_range = SourceMgr.getLocForEndOfFile(
      SourceMgr.getMainFileID());

  // if -m option is specified, apply only specified operators to input file.
  bool apply_all_mutant_operators = true;

  // Validate and analyze user's inputs.
  UserInputAnalyzingState state = UserInputAnalyzingState::kNonAOrBOption;
  string mutantOpName{""};
  set<string> domain;
  set<string> range;
  int line_num{0};
  int col_num{0};

  vector<MutantOperatorTemplate*> mutant_operator_list;

  vector<ExprMutantOperator*> expr_mutant_operator_list;
  vector<StmtMutantOperator*> stmt_mutant_operator_list;

  for (int i = 2; i < argc; ++i)
  {
    string option = argv[i];
    cout << "handling " << option << endl;

    if (option.compare("-m") == 0)
    {
      apply_all_mutant_operators = false;
      clearState(state, mutantOpName, domain, range, 
                 stmt_mutant_operator_list, expr_mutant_operator_list);
      
      mutantOpName.clear();
      domain.clear();
      range.clear();
      
      state = UserInputAnalyzingState::kMutantName;
    }
    else if (option.compare("-l") == 0)
    {
      clearState(state, mutantOpName, domain, range, 
                 stmt_mutant_operator_list, expr_mutant_operator_list);
      state = UserInputAnalyzingState::kLimitNumOfMutant;
    }
    else if (option.compare("-rs") == 0)
    {
      clearState(state, mutantOpName, domain, range, 
                 stmt_mutant_operator_list, expr_mutant_operator_list);
      state = UserInputAnalyzingState::kRsLine;
    }
    else if (option.compare("-re") == 0)
    {
      clearState(state, mutantOpName, domain, range, 
                 stmt_mutant_operator_list, expr_mutant_operator_list);
      state = UserInputAnalyzingState::kReLine;
    }
    else if (option.compare("-o") == 0)
    {
      clearState(state, mutantOpName, domain, range, 
                 stmt_mutant_operator_list, expr_mutant_operator_list);
      state = UserInputAnalyzingState::kOutputDir;
    }
    else if (option.compare("-A") == 0)
    {
      if (state != UserInputAnalyzingState::kAnyOptionAndMutantName)
      {
        // not expecting -A at this position
        PrintUsageErrorMsg();
        exit(1);
      }
      else
        state = UserInputAnalyzingState::kDomainOfMutantOperator;
    }
    else if (option.compare("-B") == 0)
    {
      if (state != UserInputAnalyzingState::kNonAOptionAndMutantName && 
          state != UserInputAnalyzingState::kAnyOptionAndMutantName)
      {
        // not expecting -B at this position
        PrintUsageErrorMsg();
        exit(1);
      }
      else
        state = UserInputAnalyzingState::kRangeOfMutantOperator;
    }
    else
    {
      HandleInput(option, state, mutantOpName, domain, range,
                  output_dir, limit, &start_of_mutation_range, 
                  &end_of_mutation_range, line_num, col_num, SourceMgr,
                  stmt_mutant_operator_list, expr_mutant_operator_list);
    }
  }

  clearState(state, mutantOpName, domain, range, 
             stmt_mutant_operator_list, expr_mutant_operator_list);

  cout << mutant_operator_list.size() << endl;

  if (apply_all_mutant_operators) 
  {
    // holder->ApplyAllMutantOperators();
  }

  // Make mutation database file named <inputfilename>_mut_db.out
  vector<string> path;
  SplitStringIntoVector(string(argv[1]), path, string("/"));

  // inputfile name is the string after the last slash (/)
  // in the provided path to inputfile
  string inputFilename = path.back();

  string mutDbFilename(output_dir);
  mutDbFilename.append(inputFilename, 0, inputFilename.length()-2);
  mutDbFilename += "_mut_db.out";

  // Open the file with mode TRUNC to create the file if not existed
  // or delete content if existed.
  ofstream out_mutDb(mutDbFilename.data(), ios::trunc);   
  out_mutDb.close();

  // Create Configuration object pointer to pass as attribute for MyASTConsumer
  Configuration *config = new Configuration(inputFilename, mutDbFilename, 
                                       start_of_mutation_range, 
                                       end_of_mutation_range, output_dir, 
                                       limit);

  //=======================================================
  //====================== PARSING ========================
  //=======================================================
  // CompilerInstance will hold the instance of the Clang compiler for us,
  // managing the various objects needed to run the compiler.
  CompilerInstance TheCompInst2;
  
  // Diagnostics manage problems and issues in compile 
  TheCompInst2.createDiagnostics(NULL, false);

  // Set target platform options 
  // Initialize target info with the default triple for our platform.
  TargetOptions *TO2 = new TargetOptions();
  TO2->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI2 = TargetInfo::CreateTargetInfo(TheCompInst2.getDiagnostics(), 
                                                TO2);
  TheCompInst2.setTarget(TI2);

  // FileManager supports for file system lookup, file system caching, 
  // and directory search management.
  TheCompInst2.createFileManager();
  FileManager &FileMgr2 = TheCompInst2.getFileManager();
  
  // SourceManager handles loading and caching of source files into memory.
  TheCompInst2.createSourceManager(FileMgr2);
  SourceManager &SourceMgr2 = TheCompInst2.getSourceManager();
  
  // Prreprocessor runs within a single source file
  TheCompInst2.createPreprocessor();
  
  // ASTContext holds long-lived AST nodes (such as types and decls) .
  TheCompInst2.createASTContext();

  // Enable HeaderSearch option
  llvm::IntrusiveRefCntPtr<clang::HeaderSearchOptions> hso2(
      new HeaderSearchOptions());
  HeaderSearch headerSearch2(hso2,
                            TheCompInst2.getFileManager(),
                            TheCompInst2.getDiagnostics(),
                            TheCompInst2.getLangOpts(),
                            TI2);

  // <Warning!!> -- Platform Specific Code lives here
  // This depends on A) that you're running linux and
  // B) that you have the same GCC LIBs installed that I do. 
  /*
  $ gcc -xc -E -v -
  ..
   /usr/local/include
   /usr/lib/gcc/x86_64-linux-gnu/4.4.5/include
   /usr/lib/gcc/x86_64-linux-gnu/4.4.5/include-fixed
   /usr/include
  End of search list.
  */
  const char *include_paths2[] = {"/usr/local/include",
        "/usr/lib/gcc/x86_64-linux-gnu/4.4.6/include",
        "/usr/lib/gcc/x86_64-linux-gnu/4.4.6/include-fixed",
        "/usr/include"};

  for (int i=0; i<4; i++) 
    hso2->AddPath(include_paths2[i], 
          clang::frontend::Angled, 
          false, 
          false);
  // </Warning!!> -- End of Platform Specific Code

  InitializePreprocessor(TheCompInst2.getPreprocessor(), 
                         TheCompInst2.getPreprocessorOpts(),
                         *hso2,
                         TheCompInst2.getFrontendOpts());

  // Set the main file handled by the source manager to the input file.
  const FileEntry *FileIn2 = FileMgr2.getFile(argv[1]);
  SourceMgr2.createMainFileID(FileIn2);
  
  // Inform Diagnostics that processing of a source file is beginning. 
  TheCompInst2.getDiagnosticClient().BeginSourceFile(
      TheCompInst2.getLangOpts(),&TheCompInst2.getPreprocessor());

  // Parse the file to AST, gather labelstmts, goto stmts, 
  // scalar constants, string literals. 
  InformationGatherer *TheGatherer = new InformationGatherer(&TheCompInst2);

  ParseAST(TheCompInst2.getPreprocessor(), TheGatherer, 
           TheCompInst2.getASTContext());

  ComutContext context(
      &TheCompInst, config, TheGatherer->getLabelToGotoListMap(),
      TheGatherer->getSymbolTable());

  // Create an AST consumer instance which is going to get called by ParseAST.
  MyASTConsumer TheConsumer(
      &TheCompInst, TheGatherer->getLabelToGotoListMap(), 
      stmt_mutant_operator_list, expr_mutant_operator_list, context);

  Sema sema(TheCompInst.getPreprocessor(), TheCompInst.getASTContext(), 
            TheConsumer);

  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(sema);

  return 0;
}
