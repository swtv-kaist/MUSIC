#include "music_ast_consumer.h"
#include "music_utility.h"

void MusicASTVisitor::UpdateAddressOfRange(
    UnaryOperator *uo, SourceLocation *start_loc, SourceLocation *end_loc)
{
  // cout << "UpdateAddressOfRange called\n";
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
bool MusicASTVisitor::IsScalarRefMutatableByVtwd(string scalarref_name)
{
  // cout << "IsScalarRefMutatableByVtwd called\n";
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
bool MusicASTVisitor::CollectNonVtwdMutatableScalarRef(
    Expr *e, bool exclude_last_scalarref)
{
  // cout << "CollectNonVtwdMutatableScalarRef called\n";
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
          string reference_name{
              ConvertToString(rhs, comp_inst_->getLangOpts())};

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
          string reference_name{
              ConvertToString(lhs, comp_inst_->getLangOpts())};

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
            string reference_name{
                ConvertToString(lhs, comp_inst_->getLangOpts())};

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

void MusicASTVisitor::HandleUnaryOperatorExpr(UnaryOperator *uo)
{
  // cout << "HandleUnaryOperatorExpr called\n";
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

void MusicASTVisitor::HandleBinaryOperatorExpr(Expr *e)
{
  // cout << "HandleBinaryOperatorExpr called\n";
  BinaryOperator *bo = cast<BinaryOperator>(e);

  // if (bo->getOpcode() == BO_Sub && ExprIsPointer(bo->getLHS()) && ExprIsPointer(bo->getRHS()))
  //   cout << bo->getType().getAsString() << endl;

  // Retrieve the location and the operator of the expression
  string binary_operator = static_cast<string>(bo->getOpcodeStr());
  SourceLocation start_loc = bo->getOperatorLoc();
  SourceLocation end_loc = src_mgr_.translateLineCol(
      src_mgr_.getMainFileID(),
      GetLineNumber(src_mgr_, start_loc),
      GetColumnNumber(src_mgr_, start_loc) + binary_operator.length());
  // cout << "cp consumer\n";

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
  // Store the name of A so that MUSIC do not mutate it.
  if (bo->isAdditiveOp())
    CollectNonVtwdMutatableScalarRef(e, true);

  // In an expression PTR+<expr> or PTR-<EXPR>,
  // right hand side <expr> should not be floating
  if (bo->getOpcode() == BO_Add || bo->getOpcode() == BO_Sub)
  {
    if (ExprIsPointer(bo->getLHS()->IgnoreImpCasts()) &&
        !stmt_context_.IsInNonFloatingExprRange(e))
      stmt_context_.setNonFloatingExprRange(new SourceRange(
          bo->getRHS()->getLocStart(), 
          GetEndLocOfExpr(bo->getRHS()->IgnoreImpCasts(), comp_inst_)));

  }



  // Modulo, shift and bitwise expressions' values are integral,
  // and they also only take integral operands.
  // So OCOR should not mutate any cast inside these expr to float type
  if (binary_operator.compare("%") == 0 || bo->isBitwiseOp() || 
      bo->isShiftOp() || bo->getOpcode() == BO_RemAssign ||
      OperatorIsBitwiseAssignment(bo) || OperatorIsShiftAssignment(bo)) 
  {
    // Setting up for blocking uncompilable mutants for OCOR
    if (!stmt_context_.IsInNonFloatingExprRange(e))
    {
      stmt_context_.setNonFloatingExprRange(new SourceRange(
          bo->getLocStart(), GetEndLocOfExpr(e, comp_inst_)));
    }        
  }
}

MusicASTVisitor::MusicASTVisitor(
    clang::CompilerInstance *CI, 
    LabelStmtToGotoStmtListMap *label_to_gotolist_map, 
    std::vector<StmtMutantOperator*> &stmt_operator_list,
    std::vector<ExprMutantOperator*> &expr_operator_list,
    MusicContext &context) 
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

  context_.switchstmt_info_list_ = &switchstmt_info_list_;
  context_.non_VTWD_mutatable_scalarref_list_ = &non_VTWD_mutatable_scalarref_list_;

  context_.scope_list_ = &scope_list_;
}

bool MusicASTVisitor::VisitStmt(clang::Stmt *s)
{
  SourceLocation start_loc = s->getLocStart();
  SourceLocation end_loc = s->getLocEnd();
  SourceLocation start_spelling_loc = src_mgr_.getSpellingLoc(start_loc);
  SourceLocation end_spelling_loc = src_mgr_.getSpellingLoc(end_loc);

  if (start_loc.isMacroID() && end_loc.isMacroID())
    return true;

  /* This stmt is not written inside current target file */
  // if (src_mgr_.getFileID(start_spelling_loc) != src_mgr_.getMainFileID() &&
  //     src_mgr_.getFileID(end_spelling_loc) != src_mgr_.getMainFileID())
  // {
  //   // cout << "Not a statement inside currently parsed file\n";
  //   // cout << ConvertToString(s, comp_inst_->getLangOpts()) << endl;
  //   // cout << start_loc.printToString(src_mgr_) << endl;
  //   return true;
  // }

  // cout << "VisitStmt\n" << ConvertToString(s, comp_inst_->getLangOpts()) << endl;
  // PrintLocation(src_mgr_, start_loc);
  // start_loc.dump(src_mgr_);

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

  // cout << "mutating\n";

  for (auto mutant_operator: stmt_mutant_operator_list_)
    if (mutant_operator->IsMutationTarget(s, &context_))
      mutant_operator->Mutate(s, &context_);

  return true;
}

bool MusicASTVisitor::VisitCompoundStmt(clang::CompoundStmt *c)
{
  SourceLocation start_spelling_loc = \
      src_mgr_.getSpellingLoc(c->getLocStart());
  SourceLocation end_spelling_loc = \
      src_mgr_.getSpellingLoc(c->getLocEnd());

  if (c->getLBracLoc().isMacroID() && c->getRBracLoc().isMacroID())
    return true;

  /* This expr is not written inside current target file */
  // if (src_mgr_.getFileID(start_spelling_loc) != src_mgr_.getMainFileID() &&
  //     src_mgr_.getFileID(end_spelling_loc) != src_mgr_.getMainFileID())
  // {
  //   return true;
  // }

  // cout << "VisitCompoundStmt called\n";
  // entering a new scope
  scope_list_.push_back(SourceRange(c->getLocStart(), c->getLocEnd()));

  return true;
}

bool MusicASTVisitor::VisitSwitchStmt(clang::SwitchStmt *ss)
{
  SourceLocation start_spelling_loc = \
      src_mgr_.getSpellingLoc(ss->getLocStart());
  SourceLocation end_spelling_loc = \
      src_mgr_.getSpellingLoc(ss->getLocEnd());

  if (ss->getLocStart().isMacroID() && ss->getLocEnd().isMacroID())
    return true;

  /* This expr is not written inside current target file */
  // if (src_mgr_.getFileID(start_spelling_loc) != src_mgr_.getMainFileID() &&
  //     src_mgr_.getFileID(end_spelling_loc) != src_mgr_.getMainFileID())
  // {
  //   return true;
  // }

  // cout << "VisitSwitchStmt called\n";
  stmt_context_.setSwitchStmtConditionRange(
      new SourceRange(ss->getSwitchLoc(), ss->getBody()->getLocStart()));

  // remove switch statements that are already passed
  while (!switchstmt_info_list_.empty() && 
         !LocationIsInRange(
             ss->getLocStart(), switchstmt_info_list_.back().first))
    switchstmt_info_list_.pop_back();

  // cout << "done removing passed switch stmt\n";

  vector<string> case_value_list;
  SwitchCase *sc = ss->getSwitchCaseList();

  // collect all case values' strings
  while (sc != nullptr)
  {
    // cout << "retrieving " << ConvertToString(sc, comp_inst_->getLangOpts()) << endl;
    if (isa<DefaultStmt>(sc))
    {
      sc = sc->getNextSwitchCase();
      continue;
    }

    // cout << "not default\n";

    CaseStmt *cs = dyn_cast<CaseStmt>(sc);
    if (!cs)
    {
      cout << "cannot cast to CaseStmt for " << ConvertToString(sc, comp_inst_->getLangOpts()) << endl;
      exit(1);
    }

    string case_value{ConvertToString(cs->getLHS(), comp_inst_->getLangOpts())};

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

bool MusicASTVisitor::VisitSwitchCase(clang::SwitchCase *sc)
{
  SourceLocation start_spelling_loc = \
      src_mgr_.getSpellingLoc(sc->getLocStart());
  SourceLocation end_spelling_loc = \
      src_mgr_.getSpellingLoc(sc->getLocEnd());

  if (sc->getLocStart().isMacroID() && sc->getLocEnd().isMacroID())
    return true;

  /* This expr is not written inside current target file */
  // if (src_mgr_.getFileID(start_spelling_loc) != src_mgr_.getMainFileID() &&
  //     src_mgr_.getFileID(end_spelling_loc) != src_mgr_.getMainFileID())
  // {
  //   return true;
  // }

  stmt_context_.setSwitchCaseRange(
      new SourceRange(sc->getLocStart(), sc->getColonLoc()));
  
  // remove switch statements that are already passed
  while (!switchstmt_info_list_.empty() && 
         !LocationIsInRange(
             sc->getLocStart(), switchstmt_info_list_.back().first))
    switchstmt_info_list_.pop_back();

  return true;
}

bool MusicASTVisitor::VisitExpr(clang::Expr *e)
{
  // cout << "VisitExpr\n" << ConvertToString(e, comp_inst_->getLangOpts()) << endl;
  // PrintLocation(src_mgr_, e->getLocStart());

  if (e->getLocStart().isMacroID() && e->getLocEnd().isMacroID())
    return true;

  // if (GetLineNumber(src_mgr_, e->getLocStart()) == 49)
  // {
  //   cout << "VisitExpr\n" << ConvertToString(e, comp_inst_->getLangOpts()) << endl;
  //   cout << e->getLocStart().printToString(src_mgr_) << endl;
  // }

  // SourceLocation start_spelling_loc = \
  //     src_mgr_.getSpellingLoc(e->getLocStart());
  // SourceLocation end_spelling_loc = \
  //     src_mgr_.getSpellingLoc(e->getLocEnd());

  // /* This expr is not written inside current target file */
  // if (src_mgr_.getFileID(start_spelling_loc) != src_mgr_.getMainFileID() &&
  //     src_mgr_.getFileID(end_spelling_loc) != src_mgr_.getMainFileID())
  // {
  //   // cout << "Not a expression inside currently parsed file\n";
  //   // cout << ConvertToString(e, comp_inst_->getLangOpts()) << endl;
  //   // cout << e->getLocStart().printToString(src_mgr_) << endl;
  //   return true;
  // }

  // Do not mutate or consider anything inside a typedef definition
  if (stmt_context_.IsInTypedefRange(e))
    return true;

  for (auto mutant_operator: expr_mutant_operator_list_)
    if (mutant_operator->IsMutationTarget(e, &context_))
    {
      // if (GetLineNumber(src_mgr_, e->getLocStart()) == 49)
      //   cout << "yes\n";

      mutant_operator->Mutate(e, &context_);
    }

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

bool MusicASTVisitor::VisitEnumDecl(clang::EnumDecl *ed)
{
  stmt_context_.setIsInEnumDecl(true);
  return true;
}

bool MusicASTVisitor::VisitTypedefDecl(clang::TypedefDecl *td)
{
  stmt_context_.setTypedefDeclRange(
      new SourceRange(td->getLocStart(), td->getLocEnd()));

  return true;
}

bool MusicASTVisitor::VisitFieldDecl(clang::FieldDecl *fd)
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

bool MusicASTVisitor::VisitVarDecl(clang::VarDecl *vd)
{
  SourceLocation start_loc = vd->getLocStart();
  SourceLocation end_loc = vd->getLocEnd();

  SourceLocation start_spelling_loc = \
      src_mgr_.getSpellingLoc(start_loc);
  SourceLocation end_spelling_loc = \
      src_mgr_.getSpellingLoc(end_loc);

  if (start_loc.isMacroID() && end_loc.isMacroID())
    return true;

  /* This expr is not written inside current target file */
  // if (src_mgr_.getFileID(start_spelling_loc) != src_mgr_.getMainFileID() &&
  //     src_mgr_.getFileID(end_spelling_loc) != src_mgr_.getMainFileID())
  // {
  //   return true;
  // }

  stmt_context_.setIsInEnumDecl(false);

  if (stmt_context_.IsInTypedefRange(start_loc))
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

bool MusicASTVisitor::VisitFunctionDecl(clang::FunctionDecl *f) 
{   
  // Function with nobody, and function declaration within 
  // another function is function prototype.
  // no global or local variable mutation will be applied here.
  if (!f->hasBody() || 
      stmt_context_.IsInCurrentlyParsedFunctionRange(f->getLocStart()))
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

    // if (f->getName().compare("read_field_headers") == 0 ||
    //     f->getName().compare("formparse") == 0)
    // {
    //   f->getBody()->dumpColor();
    // }
  }

  return true;
}

MusicASTConsumer::MusicASTConsumer(
    clang::CompilerInstance *CI, 
    LabelStmtToGotoStmtListMap *label_to_gotolist_map, 
    std::vector<StmtMutantOperator*> &stmt_operator_list,
    std::vector<ExprMutantOperator*> &expr_operator_list,
    MusicContext &context)
  : Visitor(CI, label_to_gotolist_map, stmt_operator_list, 
            expr_operator_list, context) 
{ 
}

void MusicASTConsumer::HandleTranslationUnit(clang::ASTContext &Context)
{
  /* we can use ASTContext to get the TranslationUnitDecl, which is
  a single Decl that collectively represents the entire source file */
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}