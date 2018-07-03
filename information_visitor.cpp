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
  string labelName{ls->getName()};
  SourceLocation start_loc = ls->getLocStart();

  // Insert new entity into list of labels and LabelStmtToGotoStmtListMap
  label_to_gotolist_map_.insert(pair<LabelStmtLocation, GotoStmtLocationList>(
      LabelStmtLocation(GetLineNumber(src_mgr_, start_loc),
                        GetColumnNumber(src_mgr_, start_loc)),
      GotoStmtLocationList()));

  return true;
}  

bool InformationVisitor::VisitGotoStmt(GotoStmt * gs)
{
  // Retrieve LabelStmtToGotoStmtListMap's key which is label declaration location.
  LabelStmt *label = gs->getLabel()->getStmt();
  SourceLocation labelStartLoc = label->getLocStart();

  addGotoLocToMap(LabelStmtLocation(GetLineNumber(src_mgr_, labelStartLoc),
                                    GetColumnNumber(src_mgr_, labelStartLoc)), 
                  gs->getGotoLoc());

  return true;
}

bool InformationVisitor::VisitExpr(Expr *e)
{
  // Collect constants
  if (isa<CharacterLiteral>(e) || 
      isa<FloatingLiteral>(e) || 
      isa<IntegerLiteral>(e))
    CollectScalarConstant(e);
  else if (isa<StringLiteral>(e))
    CollectStringLiteral(e);

  return true;
}

bool InformationVisitor::VisitTypedefDecl(TypedefDecl *td)
{
  if (typedefdecl_range_ != nullptr)
    delete typedefdecl_range_;

  typedefdecl_range_ = new SourceRange(td->getLocStart(), td->getLocEnd());

  return true;
}

bool InformationVisitor::VisitVarDecl(VarDecl *vd)
{
  SourceLocation start_loc = vd->getLocStart();
  SourceLocation end_loc = vd->getLocEnd();

  if (LocationIsInRange(start_loc, *typedefdecl_range_) ||
      LocationIsInRange(start_loc, *function_prototype_range_))
    return true;

  CollectVarDecl(vd);

  return true;
}

bool InformationVisitor::VisitFunctionDecl(FunctionDecl *fd)
{
  if (fd->hasBody() && 
      !LocationIsInRange(fd->getLocStart(), 
                         *currently_parsed_function_range_)/* &&
      src_mgr_.getFileID(src_mgr_.getSpellingLoc(fd->getLocStart())) == \
      src_mgr_.getMainFileID()*/)
  {
    // cout << "in here\n";
    // cout << fd->getLocStart().printToString(src_mgr_) << endl;

    if (currently_parsed_function_range_ != nullptr)
      delete currently_parsed_function_range_;

    currently_parsed_function_range_ = new SourceRange(fd->getLocStart(), 
                                                       fd->getLocEnd());

    local_scalar_vardecl_list_.push_back(VarDeclList());
    local_array_vardecl_list_.push_back(VarDeclList());
    local_struct_vardecl_list_.push_back(VarDeclList());
    local_pointer_vardecl_list_.push_back(VarDeclList());
    local_stringliteral_list_.push_back(ExprList());
    local_scalarconstant_list_.push_back(ExprList());

    // clear the constants of the previously parsed function
    // get ready to contain constants of the new function
    local_scalar_constant_cache_.clear(); 
  }
  else 
  {
    if (function_prototype_range_ != nullptr)
      delete  function_prototype_range_;

    function_prototype_range_ = new SourceRange(
        fd->getLocStart(), fd->getLocEnd());
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
      &global_pointer_vardecl_list_, &local_pointer_vardecl_list_);
}

LabelStmtToGotoStmtListMap* InformationVisitor::getLabelToGotoListMap()
{
  return &label_to_gotolist_map_;
}

void InformationVisitor::CollectVarDecl(VarDecl *vd)
{
  SourceLocation start_loc = vd->getLocStart();
  SourceLocation end_loc = vd->getLocEnd();
  string var_name{GetVarDeclName(vd)};

  // If VD is non-named variable (inside a function prototype), or is not
  // declared within target input file, then skip.
  if (src_mgr_.getFileID(src_mgr_.getSpellingLoc(start_loc)) != \
      src_mgr_.getMainFileID() || var_name.empty())
    return;

  if(vd->isFileVarDecl())
  {
    if (IsVarDeclScalar(vd))
    {
      global_scalar_vardecl_list_.push_back(vd);

      // cout << "collect global scalar vardecl " << GetVarDeclName(vd) << endl;
      // cout << vd->getLocStart().printToString(src_mgr_) << endl;
      // cout << src_mgr_.getFileID(start_loc).getHashValue() << endl;
      // cout << src_mgr_.getExpansionLoc(start_loc).printToString(src_mgr_) << endl;
      // cout << src_mgr_.getFileID(src_mgr_.getExpansionLoc(start_loc)).getHashValue() << endl;
      // cout << src_mgr_.getSpellingLoc(start_loc).printToString(src_mgr_) << endl;
      // cout << src_mgr_.getFileID(src_mgr_.getSpellingLoc(start_loc)).getHashValue() << endl;
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
  // if (src_mgr_.getFileID(src_mgr_.getSpellingLoc(e->getLocStart())) != \
  //     src_mgr_.getMainFileID())
  //   return;

  string token{ConvertToString(e, comp_inst_->getLangOpts())};

  llvm::APSInt int_value;

  // Try to convert floating literal expression into a double value.
  if (isa<FloatingLiteral>(e))
    ConvertConstFloatExprToFloatString(e, comp_inst_, token);
  else
    ConvertConstIntExprToIntString(e, comp_inst_, token);

  // local constants
  if (LocationIsInRange(src_mgr_.getExpansionLoc(e->getLocStart()), 
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
  // If the constant is not in the cache, add this new entity into
  // the cache and the vector storing global consts.
  // Else, do nothing.
  else if (
    global_scalar_constant_cache_.find(token) == global_scalar_constant_cache_.end())
  {
    // cout << "found " << token << " at\n";
    // cout << e->getLocStart().isMacroID() << endl;
    // cout << src_mgr_.getMainFileID().getHashValue() << endl;
    // cout << e->getLocStart().printToString(src_mgr_) << endl;
    // cout << src_mgr_.getFileID(e->getLocStart()).getHashValue() << endl;
    // cout << src_mgr_.getExpansionLoc(e->getLocStart()).printToString(src_mgr_) << endl;
    // cout << src_mgr_.getFileID(src_mgr_.getExpansionLoc(e->getLocStart())).getHashValue() << endl;
    // cout << src_mgr_.getSpellingLoc(e->getLocStart()).printToString(src_mgr_) << endl;
    // cout << src_mgr_.getFileID(src_mgr_.getSpellingLoc(e->getLocStart())).getHashValue() << endl;

    global_scalar_constant_cache_.insert(token);
    global_scalarconstant_list_.push_back(e);
  }
}

void InformationVisitor::CollectStringLiteral(Expr *e)
{
  SourceLocation start_loc = e->getLocStart();
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