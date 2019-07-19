#include "music_ast_consumer.h"
#include "music_utility.h"

set<string> rhs_to_const_mutation_operators{"RGCR", "RLCR", "RRCR"};
set<string> rhs_to_var_mutation_operators{"RGSR", "RLSR", "RGPR", "RLPR", 
                                          "RGTR", "RLTR", "RGAR", "RLAR"};

MusicASTVisitor::MusicASTVisitor(
    clang::CompilerInstance *CI, 
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
  stmt_context_.loop_scope_list_ = &loop_scope_list_;
}

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
  // cout << "HandleBinaryOperatorExpr\n" << ConvertToString(e, comp_inst_->getLangOpts()) << endl; 
  BinaryOperator *bo = cast<BinaryOperator>(e);

  // if (bo->getOpcode() == BO_Sub && ExprIsPointer(bo->getLHS()) && ExprIsPointer(bo->getRHS()))
  //   cout << bo->getType().getAsString() << endl;

  // Retrieve the location and the operator of the expression
  string binary_operator = static_cast<string>(bo->getOpcodeStr());
  SourceLocation start_loc = bo->getOperatorLoc();

  // SourceLocation end_loc = src_mgr_.translateLineCol(
  //     src_mgr_.getMainFileID(),
  //     GetLineNumber(src_mgr_, start_loc),
  //     GetColumnNumber(src_mgr_, start_loc) + binary_operator.length());

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
  if (bo->getOpcode() == BO_Add || bo->getOpcode() == BO_Sub ||
      bo->getOpcode() == BO_AddAssign || bo->getOpcode() == BO_SubAssign)
  {
    if ((ExprIsPointer(bo->getLHS()->IgnoreImpCasts()) ||
         ExprIsArray(bo->getLHS()->IgnoreImpCasts())) &&
        !stmt_context_.IsInNonFloatingExprRange(e))
      stmt_context_.setNonFloatingExprRange(new SourceRange(
          bo->getRHS()->getLocStart(), 
          GetEndLocOfExpr(bo->getRHS()->IgnoreImpCasts(), comp_inst_)));
  }

  // cout << "cp" << endl;
  // cout << OperatorIsShiftAssignment(bo) << endl;
  // cout << (binary_operator.compare("%") == 0) << endl;
  // cout << bo->isBitwiseOp() << endl;
  // cout << bo->isShiftOp() << endl;
  // cout << (bo->getOpcode() == BO_RemAssign) << endl;
  // cout << OperatorIsBitwiseAssignment(bo) << endl;
  // cout << bo->isShiftAssignOp() << endl;

  // Modulo, shift and bitwise expressions' values are integral,
  // and they also only take integral operands.
  // So OCOR should not mutate any cast inside these expr to float type
  if (binary_operator.compare("%") == 0 || bo->isBitwiseOp() || 
      bo->isShiftOp() || bo->getOpcode() == BO_RemAssign ||
      OperatorIsBitwiseAssignment(bo) || bo->isShiftAssignOp()) 
  {
    // cout << "cp2" << endl;
    // Setting up for blocking uncompilable mutants for OCOR
    if (!stmt_context_.IsInNonFloatingExprRange(e))
    {
      // cout << "cp3" << endl;
      stmt_context_.setNonFloatingExprRange(new SourceRange(
          bo->getLocStart(), GetEndLocOfExpr(e, comp_inst_)));
      // PrintRange(src_mgr_, *stmt_context_.non_floating_expr_range_);
    }        
  }
}

bool MusicASTVisitor::ValidateSourceRange(SourceLocation &start_loc, SourceLocation &end_loc)
{
  // I forget the reason why ...
  // if (start_loc.isMacroID() && end_loc.isMacroID())
  //   return false;
  // cout << "checking\n";
  // PrintLocation(src_mgr_, start_loc);
  // PrintLocation(src_mgr_, end_loc);

  if (start_loc.isInvalid() || end_loc.isInvalid())
    return false;

  // if (!src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) &&
  //     !src_mgr_.isInFileID(end_loc, src_mgr_.getMainFileID()))
  //   return false;

  // cout << "MusicASTVisitor cp1\n";
  // cout << start_loc.printToString(src_mgr_) << endl;
  // cout << end_loc.printToString(src_mgr_) << endl;
  // cout << src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) << endl;
  // cout << src_mgr_.isInFileID(end_loc, src_mgr_.getMainFileID()) << endl;

  // Skip nodes that are not in the to-be-mutated file.
  if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(start_loc).getHashValue() &&
      src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(end_loc).getHashValue())
    return false;

  // cout << "cp2\n";

  if (start_loc.isMacroID())
  {
    /* THIS PART WORKS FOR CLANG 6.0.1 */
    // pair<SourceLocation, SourceLocation> expansion_range = 
    //     rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
    // start_loc = expansion_range.first;
    /*=================================*/

    /* THIS PART WORKS FOR CLANG 7.0.1 */
    CharSourceRange expansion_range = 
        rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
    start_loc = expansion_range.getBegin();
    /*=================================*/
  }

  // cout << "cp2\n";

  if (end_loc.isMacroID())
  {
    /* THIS PART WORKS FOR CLANG 6.0.1 */
    // pair<SourceLocation, SourceLocation> expansion_range = 
    //     rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
    // end_loc = expansion_range.second;
    /*=================================*/

    /* THIS PART WORKS FOR CLANG 7.0.1 */
    CharSourceRange expansion_range = 
        rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
    end_loc = expansion_range.getEnd();
    /*=================================*/

    end_loc = Lexer::getLocForEndOfToken(
        src_mgr_.getExpansionLoc(end_loc), 0, src_mgr_, comp_inst_->getLangOpts());

    // Skipping whitespaces (if any)
    while (*(src_mgr_.getCharacterData(end_loc)) == ' ')
      end_loc = end_loc.getLocWithOffset(1);

    // Return if macro is variable-typed. 
    if (*(src_mgr_.getCharacterData(end_loc)) != '(')
      return true;

    // Find the closing bracket of this function-typed macro
    int parenthesis_counter = 1;
    end_loc = end_loc.getLocWithOffset(1);

    while (parenthesis_counter != 0)
    {
      if (*(src_mgr_.getCharacterData(end_loc)) == '(')
        parenthesis_counter++;

      if (*(src_mgr_.getCharacterData(end_loc)) == ')')
        parenthesis_counter--;

      end_loc = end_loc.getLocWithOffset(1);
    }
  }
  // cout << "cp3\n";

  return true;
}

bool MusicASTVisitor::VisitStmt(clang::Stmt *s)
{
  // cout << "VisitStmt\n";
  SourceLocation start_loc = s->getLocStart();
  SourceLocation end_loc = s->getLocEnd();
  SourceLocation start_spelling_loc = src_mgr_.getSpellingLoc(start_loc);
  SourceLocation end_spelling_loc = src_mgr_.getSpellingLoc(end_loc);

  if (start_loc.isMacroID() && end_loc.isMacroID())
    return true;

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // Skip nodes that are not in the to-be-mutated file.
  // if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(start_loc).getHashValue() &&
  //     src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(end_loc).getHashValue())
  //   return true;

  // if (start_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
  //   start_loc = expansion_range.first;
  // }

  // if (end_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
  //   end_loc = expansion_range.first;
  // }

  // cout << "VisitStmt\n" << ConvertToString(s, comp_inst_->getLangOpts()) << endl;

  // PrintLocation(src_mgr_, start_loc);
  // start_loc.dump(src_mgr_);

  // cout << "we here boys\n";

  //============================================================================
  // set up Proteum-style line number
  // COMMENTED OUT FOR CAUSING ERROR IN MUTATING MARK.C OF VIM V5.3
  /*if (GetLineNumber(src_mgr_, start_loc) > proteumstyle_stmt_end_line_num_)
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
  }*/
  //============================================================================

  if (stmt_context_.IsInArrayDeclSize() && 
      ((!start_loc.isMacroID() && !LocationIsInRange(start_loc, *array_decl_range_)) ||
       (!end_loc.isMacroID() && !LocationIsInRange(end_loc, *array_decl_range_))))
  {
    // cout << "out of array_decl_range_\n";
    // PrintLocation(src_mgr_, start_loc);
    stmt_context_.setIsInArrayDeclSize(false);
  }

  // cout << "array decl size is not a problem\n";

  if (isa<ForStmt>(s))
    scope_list_.push_back(SourceRange(start_loc, end_loc));

  if (isa<ForStmt>(s) || isa<WhileStmt>(s) || isa<DoStmt>(s))
  {
    loop_scope_list_.push_back(
        make_pair(s, SourceRange(start_loc, end_loc)));
  }

  // cout << "loop is not a problem\n";

  if (isa<ReturnStmt>(s))
  {
    stmt_context_.last_return_statement_line_num_ = src_mgr_.getExpansionLineNumber(start_loc);
  }

  // cout << "return is not a problem\n";

  for (auto mutant_operator: stmt_mutant_operator_list_)
  {
    // cout << mutant_operator->getName() << " mutating\n";
    if (mutant_operator->IsMutationTarget(s, &context_))
      mutant_operator->Mutate(s, &context_);
    // cout << "DONE\n";
  }

  return true;
}

bool MusicASTVisitor::VisitCompoundStmt(clang::CompoundStmt *c)
{
  // cout << "VisitCompoundStmt\n";
  // SourceLocation start_spelling_loc = \
  //     src_mgr_.getSpellingLoc(c->getLocStart());
  // SourceLocation end_spelling_loc = \
  //     src_mgr_.getSpellingLoc(c->getLocEnd());

  SourceLocation start_loc = c->getLocStart();
  SourceLocation end_loc = c->getLocEnd();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // Skip nodes that are not in the to-be-mutated file.
  // if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(c->getLocStart()).getHashValue() &&
  //     src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(c->getLocEnd()).getHashValue())
  //   return true;

  // if (start_loc.isMacroID() && end_loc.isMacroID())
  //   return true;

  // if (start_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
  //   start_loc = expansion_range.first;
  // }

  // if (end_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
  //   end_loc = expansion_range.first;
  // }

  /* This expr is not written inside current target file */
  // if (src_mgr_.getFileID(start_spelling_loc) != src_mgr_.getMainFileID() &&
  //     src_mgr_.getFileID(end_spelling_loc) != src_mgr_.getMainFileID())
  // {
  //   return true;
  // }

  // cout << "VisitCompoundStmt called\n";
  // entering a new scope
  scope_list_.push_back(SourceRange(start_loc, end_loc));

  return true;
}

bool MusicASTVisitor::VisitSwitchStmt(clang::SwitchStmt *ss)
{
  // cout << "VisitSwitchStmt\n";
  SourceLocation start_loc = ss->getLocStart();
  SourceLocation end_loc = ss->getLocEnd();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // Skip nodes that are not in the to-be-mutated file.
  // if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(ss->getLocStart()).getHashValue() &&
  //     src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(ss->getLocEnd()).getHashValue())
  //   return true;

  // SourceLocation start_spelling_loc = \
  //     src_mgr_.getSpellingLoc(ss->getLocStart());
  // SourceLocation end_spelling_loc = \
  //     src_mgr_.getSpellingLoc(ss->getLocEnd());

  // if (ss->getLocStart().isMacroID() && ss->getLocEnd().isMacroID())
  //   return true;

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

    ConvertConstIntExprToIntString(cs->getLHS(), comp_inst_, case_value);

    case_value_list.push_back(case_value);

    sc = sc->getNextSwitchCase();
  }

  switchstmt_info_list_.push_back(
      make_pair(SourceRange(start_loc, end_loc), 
                case_value_list));

  return true;
}

bool MusicASTVisitor::VisitSwitchCase(clang::SwitchCase *sc)
{
  // cout << "VisitSwitchCase\n";
  // Skip nodes that are not in the to-be-mutated file.
  // if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(sc->getLocStart()).getHashValue() &&
  //     src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(sc->getLocEnd()).getHashValue())
  //   return true;

  // SourceLocation start_spelling_loc = \
  //     src_mgr_.getSpellingLoc(sc->getLocStart());
  // SourceLocation end_spelling_loc = \
  //     src_mgr_.getSpellingLoc(sc->getLocEnd());

  SourceLocation start_loc = sc->getLocStart();
  SourceLocation end_loc = sc->getLocEnd();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // if (start_loc.isMacroID() && end_loc.isMacroID())
  //   return true;

  // if (start_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
  //   start_loc = expansion_range.first;
  // }

  // if (end_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
  //   end_loc = expansion_range.first;
  // }

  /* This expr is not written inside current target file */
  // if (src_mgr_.getFileID(start_spelling_loc) != src_mgr_.getMainFileID() &&
  //     src_mgr_.getFileID(end_spelling_loc) != src_mgr_.getMainFileID())
  // {
  //   return true;
  // }

  stmt_context_.setSwitchCaseRange(
      new SourceRange(start_loc, sc->getColonLoc()));
  
  // remove switch statements that are already passed
  while (!switchstmt_info_list_.empty() && 
         !LocationIsInRange(
             sc->getLocStart(), switchstmt_info_list_.back().first))
    switchstmt_info_list_.pop_back();

  return true;
}

bool MusicASTVisitor::VisitExpr(clang::Expr *e)
{
  // cout << "VisitExpr\n";
  // Skip nodes that are not in the to-be-mutated file.
  // if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(e->getLocStart()).getHashValue() &&
  //     src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(e->getLocEnd()).getHashValue())
  //   return true;

  // cout << "VisitExpr\n" << ConvertToString(e, comp_inst_->getLangOpts()) << endl;
  // PrintLocation(src_mgr_, e->getLocStart());

  SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = e->getLocEnd();

  // cout << "cp1\n";

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // cout << "cp2\n";

  // if (start_loc.isMacroID() && end_loc.isMacroID())
  //   return true;

  // if (start_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
  //   start_loc = expansion_range.first;
  // }

  // if (end_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
  //   end_loc = expansion_range.first;
  // }

  // Do not mutate or consider anything inside a typedef definition
  if (stmt_context_.IsInTypedefRange(e))
    return true;

  // cout << "cp3\n";

  if (StmtExpr *se = dyn_cast<StmtExpr>(e))  
  {
    // set boolean variable signals following stmt are inside stmt expr
    stmt_context_.setIsInStmtExpr(true);
  }
  else if (ArraySubscriptExpr *ase = dyn_cast<ArraySubscriptExpr>(e))
  {
    SourceLocation start_loc = ase->getLocStart();
    SourceLocation end_loc = TryGetEndLocAfterBracketOrSemicolon(ase->getLocEnd(), comp_inst_);

    stmt_context_.setArraySubscriptRange(new SourceRange(start_loc, end_loc));
  }
  else if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
    HandleUnaryOperatorExpr(uo);
  else if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e)) 
    HandleBinaryOperatorExpr(e);

  // cout << "cp4\n";

  for (auto mutant_operator: expr_mutant_operator_list_)
  {
    // cout << mutant_operator->getName() << " mutating\n";
    
    if (mutant_operator->IsMutationTarget(e, &context_))
      mutant_operator->Mutate(e, &context_);

    // cout << "DONE\n";
  }

  // cout << "cp5\n";

  return true;
}

bool MusicASTVisitor::VisitEnumDecl(clang::EnumDecl *ed)
{
  // cout << "VisitEnumDecl\n";
  SourceLocation start_loc = ed->getLocStart();
  SourceLocation end_loc = ed->getLocEnd();

  // Skip nodes that are not in the to-be-mutated file.
  if (start_loc.isInvalid() || end_loc.isInvalid())
    return true;

  // if (!src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) &&
  //     !src_mgr_.isInFileID(end_loc, src_mgr_.getMainFileID()))
  //   return false;

  // cout << "MusicASTVisitor cp1\n";
  // cout << start_loc.printToString(src_mgr_) << endl;
  // cout << end_loc.printToString(src_mgr_) << endl;
  // cout << src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) << endl;
  // cout << src_mgr_.isInFileID(end_loc, src_mgr_.getMainFileID()) << endl;

  if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(start_loc).getHashValue() &&
      src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(end_loc).getHashValue())
    return true;

  // cout << "cp2\n";

  stmt_context_.setIsInEnumDecl(true);
  return true;
}

bool MusicASTVisitor::VisitTypedefDecl(clang::TypedefDecl *td)
{
  // cout << "VisitTypedefDecl\n";
  // Skip nodes that are not in the to-be-mutated file.
  // if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(td->getLocStart()).getHashValue() &&
  //     src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(td->getLocEnd()).getHashValue())
  //   return true;

  SourceLocation start_loc = td->getLocStart();
  SourceLocation end_loc = td->getLocEnd();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // if (start_loc.isMacroID() && end_loc.isMacroID())
  //   return true;

  // if (start_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
  //   start_loc = expansion_range.first;
  // }

  // if (end_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
  //   end_loc = expansion_range.first;
  // }

  stmt_context_.setTypedefDeclRange(new SourceRange(start_loc, end_loc));
  return true;
}

bool MusicASTVisitor::VisitFieldDecl(clang::FieldDecl *fd)
{
  // cout << "VisitFieldDecl\n";
  // Skip nodes that are not in the to-be-mutated file.
  // if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(fd->getLocStart()).getHashValue() &&
  //     src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(fd->getLocEnd()).getHashValue())
  //   return true;

  SourceLocation start_loc = fd->getLocStart();
  SourceLocation end_loc = fd->getLocEnd();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // if (start_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
  //   start_loc = expansion_range.first;
  // }

  // if (end_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
  //   end_loc = expansion_range.first;
  // }

  stmt_context_.setFieldDeclRange(new SourceRange(start_loc, end_loc));
  
  if (fd->getType().getTypePtr()->isArrayType())
  {
    if ((!start_loc.isMacroID() && !LocationIsInRange(start_loc, *array_decl_range_)) ||
        (!end_loc.isMacroID() && !LocationIsInRange(end_loc, *array_decl_range_)))
    {  
      stmt_context_.setIsInArrayDeclSize(true);
      array_decl_range_ = new SourceRange(start_loc, end_loc);
      // cout << "setting array_decl_range_ in FieldDecl\n";
      // cout << fd->getNameAsString() << endl;
      // PrintRange(src_mgr_, *array_decl_range_);
      // PrintLocation(src_mgr_, src_mgr_.getSpellingLoc(start_loc));
      // cout << src_mgr_.getFileID(start_loc).getHashValue() << endl;
      // cout << src_mgr_.getMainFileID().getHashValue() << endl;
    }
  }
  return true;
}

bool MusicASTVisitor::VisitVarDecl(clang::VarDecl *vd)
{
  // cout << "VisitVarDecl\n";
  // Skip nodes that are not in the to-be-mutated file.
  // if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(vd->getLocStart()).getHashValue() &&
  //     src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(vd->getLocEnd()).getHashValue())
  //   return true;

  SourceLocation start_loc = vd->getLocStart();
  SourceLocation end_loc = vd->getLocEnd();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // if (start_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
  //   start_loc = expansion_range.first;
  // }

  // if (end_loc.isMacroID())
  // {
  //   pair<SourceLocation, SourceLocation> expansion_range = 
  //       rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
  //   end_loc = expansion_range.first;
  // }

  if (vd->hasInit())
  {
    // cout << "visit scalar var declaration statement at ";
    // PrintLocation(src_mgr_, start_loc);
    
    Stmt** init = vd->getInitAddress();
    // cout << "init stmt\n" << ConvertToString(*init, comp_inst_->getLangOpts()) << endl;

    if (Expr* e = dyn_cast<Expr>(*init))
    {
      e = e->IgnoreImpCasts();
      for (auto mutation_operator: expr_mutant_operator_list_)
      {
        if (rhs_to_const_mutation_operators.find(mutation_operator->getName()) != 
                rhs_to_const_mutation_operators.end())
        {          
          // cout << mutation_operator->IsInitMutationTarget(e, &context_) << endl;
          if (mutation_operator->IsInitMutationTarget(e, &context_))
            mutation_operator->Mutate(e, &context_);
        }

        // only mutate local definition to variable
        // cannot mutate global definition to variable (any variable is not 
        // considered constant)
        if (!vd->isFileVarDecl()) 
        {
          if (rhs_to_var_mutation_operators.find(mutation_operator->getName()) != 
                  rhs_to_var_mutation_operators.end())
          {          
            // cout << mutation_operator->IsInitMutationTarget(e, &context_) << endl;
            if (mutation_operator->IsInitMutationTarget(e, &context_))
              mutation_operator->Mutate(e, &context_);
          }
        }
      }
    }      
  }

  SourceLocation start_spelling_loc = \
      src_mgr_.getSpellingLoc(start_loc);
  SourceLocation end_spelling_loc = \
      src_mgr_.getSpellingLoc(end_loc);

  // if (start_loc.isMacroID() && end_loc.isMacroID())
  //   return true;

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
      if ((!start_loc.isMacroID() && !LocationIsInRange(start_loc, *array_decl_range_)) ||
          (!end_loc.isMacroID() && !LocationIsInRange(end_loc, *array_decl_range_)))
      {
        stmt_context_.setIsInArrayDeclSize(true);
        array_decl_range_ = new SourceRange(start_loc, end_loc);
      }
    }
  }

  return true;
}

bool MusicASTVisitor::VisitFunctionDecl(clang::FunctionDecl *f) 
{
  // cout << "VisitFunctionDecl\n";
  SourceLocation start_loc = f->getLocStart();
  SourceLocation end_loc = f->getLocEnd();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // Function with nobody, and function declaration within 
  // another function is function prototype.
  // no global or local variable mutation will be applied here.
  if (!f->hasBody() || 
      stmt_context_.IsInCurrentlyParsedFunctionRange(start_loc))
  {
    functionprototype_range_ = new SourceRange(start_loc, end_loc);
  }
  else
  {
    string ftn_name{f->getNameAsString()};
    cout << "====== entering " << ftn_name << endl;
    // cout << "start_loc = "; PrintLocation(src_mgr_, start_loc);
    // cout << "end_loc = "; PrintLocation(src_mgr_, end_loc);
    // entering a new local scope
    scope_list_.clear();
    loop_scope_list_.clear();
    scope_list_.push_back(SourceRange(start_loc, end_loc));

    context_.IncrementFunctionId();

    stmt_context_.setIsInEnumDecl(false);

    stmt_context_.setCurrentlyParsedFunctionRange(
        new SourceRange(start_loc, end_loc));

    stmt_context_.setCurrentlyParsedFunctionName(ftn_name);

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
    std::vector<StmtMutantOperator*> &stmt_operator_list,
    std::vector<ExprMutantOperator*> &expr_operator_list,
    MusicContext &context)
  : Visitor(CI, stmt_operator_list, 
            expr_operator_list, context) 
{ 
}

void MusicASTConsumer::HandleTranslationUnit(clang::ASTContext &Context)
{
  /* we can use ASTContext to get the TranslationUnitDecl, which is
  a single Decl that collectively represents the entire source file */
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}