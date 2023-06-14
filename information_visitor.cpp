#include "music_utility.h"
#include "information_visitor.h"

#include "llvm/ADT/APFloat.h"

InformationVisitor::InformationVisitor(
    CompilerInstance *CI)
  : comp_inst_(CI), 
    src_mgr_(CI->getSourceManager()), lang_option_(CI->getLangOpts())
{
  SourceLocation start_of_file = src_mgr_.getLocForStartOfFile(src_mgr_.getMainFileID());
  currently_parsed_function_range_ = new SourceRange(
      start_of_file, start_of_file);

  typedefdecl_range_ = new SourceRange(start_of_file, start_of_file);
  function_prototype_range_ = new SourceRange(start_of_file, start_of_file);

  rewriter_.setSourceMgr(src_mgr_, lang_option_);
}

InformationVisitor::~InformationVisitor()
{
  // cout << "InformationVisitor destructor called\n";
}

// Add a new Goto statement location to LabelStmtToGotoStmtListMap.
// Add the label to the map if map does not contain label.
// Else add the Goto location to label's list of Goto locations.
void InformationVisitor::addGotoLocToMap(LabelStmtLocation label_loc, 
                     SourceLocation goto_stmt_loc)
{   
  GotoStmtLocationList newlist{goto_stmt_loc};

  pair<LabelStmtToGotoStmtListMap::iterator,bool> insertResult = \
    label_to_gotolist_map_.insert(
        pair<LabelStmtLocation, GotoStmtLocationList>(label_loc, newlist));

  // LabelStmtToGotoStmtListMap contains the label, insert Goto location.
  if (insertResult.second == false)
  {
    ((insertResult.first)->second).push_back(goto_stmt_loc);
  }
}

bool InformationVisitor::VisitLabelStmt(LabelStmt *ls)
{
  // cout << "VisitLabelStmt" << endl;

  string labelName{ls->getName()};
  SourceLocation start_loc = ls->getBeginLoc();
  SourceLocation end_loc = ls->getEndLoc();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  cout << labelName << endl;
  PrintLocation(src_mgr_, start_loc);

  label_list_.back().push_back(ls);

  // Insert new entity into list of labels and LabelStmtToGotoStmtListMap
  label_to_gotolist_map_.insert(pair<LabelStmtLocation, GotoStmtLocationList>(
      LabelStmtLocation(GetLineNumber(src_mgr_, start_loc),
                        GetColumnNumber(src_mgr_, start_loc)),
      GotoStmtLocationList()));

  return true;
}  

bool InformationVisitor::VisitGotoStmt(GotoStmt * gs)
{
  // cout << "VisitGotoStmt" << endl;

  SourceLocation start_loc = gs->getBeginLoc();
  SourceLocation end_loc = gs->getEndLoc();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // Retrieve LabelStmtToGotoStmtListMap's key which is label declaration location.
  LabelStmt *label = gs->getLabel()->getStmt();
  SourceLocation labelStartLoc = label->getBeginLoc();

  // string temp{gs->getLabel()->getName()};
  // SourceLocation end_loc = gs->getLabelLoc().getLocWithOffset(
  //     temp.length());

  // cout << "testing\n";
  // PrintLocation(src_mgr_, gs->getLabelLoc());
  // cout << temp.length() << endl;
  // PrintLocation(src_mgr_, end_loc);


  addGotoLocToMap(LabelStmtLocation(GetLineNumber(src_mgr_, labelStartLoc),
                                    GetColumnNumber(src_mgr_, labelStartLoc)), 
                  gs->getGotoLoc());

  return true;
}

bool InformationVisitor::VisitReturnStmt(ReturnStmt *rs)
{
  // cout << "VisitReturnStmt" << endl;

  SourceLocation start_loc = rs->getBeginLoc();
  SourceLocation end_loc = rs->getEndLoc();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  return_stmt_list_.back().push_back(rs);
  return true;
}

bool InformationVisitor::VisitExpr(Expr *e)
{
  // cout << "VisitExpr" << endl;

  SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = e->getEndLoc();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  // cout << "VisitExpr cp1\n";

  // Collect constants
  if (isa<CharacterLiteral>(e) || 
      isa<FloatingLiteral>(e) || 
      isa<IntegerLiteral>(e))
    CollectScalarConstant(e);
  else if (isa<StringLiteral>(e))
    CollectStringLiteral(e);
  else if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(e))
  {
    if (dre->getFoundDecl()->isDefinedOutsideFunctionOrMethod() &&
        !isa<FunctionDecl>(dre->getFoundDecl()) &&
        LocationIsInRange(start_loc, *currently_parsed_function_range_))
    {
      bool is_global_in_main_file = false;

      for (auto it: global_vardecl_list_)
        if (dre->getFoundDecl()->getNameAsString().compare(it->getNameAsString()) == 0)
        {
          is_global_in_main_file = true;
          break;
        }

      if (!is_global_in_main_file)
        return true;

      bool exist = false;
      for (auto it: func_used_global_list_.back())
        if (dre->getFoundDecl()->getNameAsString().compare(it->getNameAsString()) == 0)
        {
          exist = true;
          break;
        }

      if (exist)
        return true;

      func_used_global_list_.back().push_back(dre->getFoundDecl());
      // cout << ConvertToString(dre, comp_inst_->getLangOpts()) << endl;

      // go through NonUsedGlobalList of current function to remove this variable
      for (auto it = func_not_used_global_list_.back().begin();
           it != func_not_used_global_list_.back().end(); ++it)
        if ((*it)->getNameAsString().compare(dre->getFoundDecl()->getNameAsString()) == 0) 
        {
          func_not_used_global_list_.back().erase(it);
          break;
        }
    }
  }

  return true;
}

bool InformationVisitor::VisitTypedefDecl(TypedefDecl *td)
{
  // cout << "VisitTypedefDecl" << endl;

  SourceLocation start_loc = td->getBeginLoc();
  SourceLocation end_loc = td->getEndLoc();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  if (typedefdecl_range_ != nullptr)
    delete typedefdecl_range_;

  typedefdecl_range_ = new SourceRange(td->getBeginLoc(), td->getEndLoc());

  return true;
}

bool InformationVisitor::VisitVarDecl(VarDecl *vd)
{
  // cout << "VisitVarDecl" << endl;

  SourceLocation start_loc = vd->getBeginLoc();
  SourceLocation end_loc = vd->getEndLoc();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  if (LocationIsInRange(start_loc, *typedefdecl_range_) ||
      LocationIsInRange(start_loc, *function_prototype_range_))
    return true;

  CollectVarDecl(vd);

  return true;
}

bool InformationVisitor::VisitFunctionDecl(FunctionDecl *fd)
{
  // cout << "VisitFunctionDecl" << endl;

  SourceLocation start_loc = fd->getBeginLoc();
  SourceLocation end_loc = fd->getEndLoc();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  if (fd->hasBody() && 
      !LocationIsInRange(start_loc, *currently_parsed_function_range_))
  {
    // if (!func_local_list_.empty())
    // {
    //   for (auto it: func_local_list_.back())
    //     cout << it->getNameAsString() << endl;
    //   cout << "=====================\n";
    // }
    // // cout << "in here\n";
    // // cout << start_loc.printToString(src_mgr_) << endl;
    // cout << "function name: " << fd->getNameAsString() << endl;

    if (currently_parsed_function_range_ != nullptr)
      delete currently_parsed_function_range_;

    currently_parsed_function_range_ = new SourceRange(start_loc, 
                                                       end_loc);

    local_scalar_vardecl_list_.push_back(VarDeclList());
    local_array_vardecl_list_.push_back(VarDeclList());
    local_struct_vardecl_list_.push_back(VarDeclList());
    local_pointer_vardecl_list_.push_back(VarDeclList());
    local_stringliteral_list_.push_back(ExprList());
    local_scalarconstant_list_.push_back(ExprList());
    func_used_global_list_.push_back(std::vector<NamedDecl*>());
    func_param_list_.push_back(std::vector<ParmVarDecl*>());
    func_not_used_global_list_.push_back(VarDeclList(global_vardecl_list_));
    func_local_list_.push_back(VarDeclList());
    label_list_.push_back(LabelList());
    return_stmt_list_.push_back(ReturnStmtList());
    // cout << return_stmt_list_.size() << endl;
    // cout << label_list_.size() << endl;
    // cout << local_scalar_vardecl_list_.size() << endl;

    for (auto it = fd->param_begin(); it != fd->param_end(); ++it)
    {
      func_param_list_.back().push_back(*it);
      // cout << "param name: " << (*it)->getNameAsString() << endl;
    }

    // clear the constants of the previously parsed function
    // get ready to contain constants of the new function
    local_scalar_constant_cache_.clear(); 
  }
  else 
  {
    if (function_prototype_range_ != nullptr)
      delete  function_prototype_range_;

    function_prototype_range_ = new SourceRange(
        start_loc, end_loc);
  }

  return true;
}

bool InformationVisitor::VisitDeclRefExpr(DeclRefExpr *dre)
{
  // cout << "VisitLabelStmt" << endl;

  SourceLocation start_loc = dre->getBeginLoc();
  SourceLocation end_loc = dre->getEndLoc();

  if (!ValidateSourceRange(start_loc, end_loc))
    return true;

  ValueDecl *decl = dre->getDecl();
  if (VarDecl *v_decl = dyn_cast<VarDecl>(decl))
  {
    SourceLocation loc = dre->getLocation();
    unsigned int line = src_mgr_.getExpansionLineNumber(loc);
    line_to_vars_map_[line].insert(v_decl);
  }
   
  return true;
}

SymbolTable* InformationVisitor::getSymbolTable()
{
  return new SymbolTable(
      &global_scalarconstant_list_, &local_scalarconstant_list_,
      &global_stringliteral_list_, &local_stringliteral_list_,
      &global_scalar_vardecl_list_, &local_scalar_vardecl_list_,
      &global_array_vardecl_list_, &local_array_vardecl_list_,
      &global_struct_vardecl_list_, &local_struct_vardecl_list_,
      &global_pointer_vardecl_list_, &local_pointer_vardecl_list_,
      &func_param_list_, &func_used_global_list_, &func_not_used_global_list_, 
      &func_local_list_, &line_to_vars_map_, &line_to_consts_map_, 
      &label_list_, &return_stmt_list_);
}

LabelStmtToGotoStmtListMap* InformationVisitor::getLabelToGotoListMap()
{
  return &label_to_gotolist_map_;
}

void InformationVisitor::CollectVarDecl(VarDecl *vd)
{
  SourceLocation start_loc = vd->getBeginLoc();
  SourceLocation end_loc = vd->getEndLoc();
  string var_name{GetVarDeclName(vd)};

  // If VD is non-named variable (inside a function prototype), or is not
  // declared within target input file, then skip.
  if (var_name.empty())
    return;

  // cout << "information_visitor cp1\n";
  // cout << start_loc.printToString(src_mgr_) << endl;
  // cout << src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) << endl;

  if (start_loc.isInvalid())
    return;
  // else if (!src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()))
    // return;
  else if (src_mgr_.getFileID(start_loc) != src_mgr_.getMainFileID())
    return;   

  // cout << "cp2\n";

  if(vd->isFileVarDecl())
  {
    global_vardecl_list_.push_back(vd);
    if (IsVarDeclScalar(vd))
    {
      global_scalar_vardecl_list_.push_back(vd);
    }
    else if (IsVarDeclArray(vd))
      global_array_vardecl_list_.push_back(vd);
    else if (IsVarDeclStruct(vd))
      global_struct_vardecl_list_.push_back(vd);
    else if (IsVarDeclPointer(vd))
      global_pointer_vardecl_list_.push_back(vd);
  }
  else if (LocationIsInRange(start_loc, *currently_parsed_function_range_))
  {
    if (!isa<ParmVarDecl>(vd))
      func_local_list_.back().push_back(vd);

    if (IsVarDeclScalar(vd))
      local_scalar_vardecl_list_.back().push_back(vd);
    else if (IsVarDeclArray(vd))
      local_array_vardecl_list_.back().push_back(vd);
    else if (IsVarDeclStruct(vd))
      local_struct_vardecl_list_.back().push_back(vd);
    else if (IsVarDeclPointer(vd))
      local_pointer_vardecl_list_.back().push_back(vd);
  }
  else
  {
    cout << "local variable not inside a function at ";
    PrintLocation(src_mgr_, start_loc);
    cout << start_loc.printToString(src_mgr_) << endl;
  }
}

void InformationVisitor::CollectScalarConstant(Expr* e)
{
  SourceLocation loc = e->getBeginLoc();
  unsigned int line = src_mgr_.getExpansionLineNumber(loc);
  line_to_consts_map_[line].insert(e);

  string token{ConvertToString(e, comp_inst_->getLangOpts())};

  llvm::APSInt int_value;

  // Try to convert floating literal expression into a double value.
  if (isa<FloatingLiteral>(e))
    ConvertConstFloatExprToFloatString(e, comp_inst_, token);
  else
    ConvertConstIntExprToIntString(e, comp_inst_, token);

  // local constants
  if (LocationIsInRange(src_mgr_.getExpansionLoc(e->getBeginLoc()), 
                        *currently_parsed_function_range_))  
  {
    // If the constant is not in the cache, add this new entity into
    // the cache and the vector storing local consts.
    // Else, do nothing.
    if (local_scalar_constant_cache_.find(token) == local_scalar_constant_cache_.end())
    {
      local_scalar_constant_cache_.insert(token);
      local_scalarconstant_list_.back().push_back(e);
    }
  }
  // global constants
  // If the constant is not in the cache, add this new entity into
  // the cache and the vector storing global consts.
  // Else, do nothing.
  else if (
    global_scalar_constant_cache_.find(token) == global_scalar_constant_cache_.end())
  {
    global_scalar_constant_cache_.insert(token);
    global_scalarconstant_list_.push_back(e);
  }
}

void InformationVisitor::CollectStringLiteral(Expr *e)
{
  SourceLocation start_loc = e->getBeginLoc();
  string string_literal{ConvertToString(e, comp_inst_->getLangOpts())};

  if (LocationIsInRange(start_loc, *currently_parsed_function_range_))
  {
    // local string literal
    // if there is the SAME string in the SAME function,
    // then dont add
    auto it = local_stringliteral_list_.back().begin();

    while (it != local_stringliteral_list_.back().end() &&
           string_literal.compare(
               ConvertToString(*it, comp_inst_->getLangOpts())) != 0)
      ++it;

    if (it == local_stringliteral_list_.back().end())
      local_stringliteral_list_.back().push_back(e);
  }
  else
  {
    // global string literal
    // Insert if global string vector does not contain this literal
    for (auto it: global_stringliteral_list_)
    {
      if (string_literal.compare(
              ConvertToString(it, comp_inst_->getLangOpts())) == 0)
        return;
    }

    global_stringliteral_list_.push_back(e);
  }
}

bool InformationVisitor::ValidateSourceRange(SourceLocation &start_loc, SourceLocation &end_loc)
{
  // cout << "ValidateSourceRange" << endl;

  // I forget the reason why ...
  // if (start_loc.isMacroID() && end_loc.isMacroID())
  //   return false;

  if (start_loc.isInvalid() || end_loc.isInvalid())
    return false;

  // if (!src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) &&
  //     !src_mgr_.isInFileID(end_loc, src_mgr_.getMainFileID()))
  //   return false;

  // cout << "information_visitor cp1\n";
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

  // cout << "cp3\n";

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

  return true;
}
