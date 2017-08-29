#include "comut_utility.h"
#include "information_visitor.h"

InformationVisitor::InformationVisitor(
    CompilerInstance *CI)
  : comp_inst_(CI), 
    src_mgr_(CI->getSourceManager()), lang_option_(CI->getLangOpts())
{
  label_srclocation_list_ = new vector<SourceLocation>();
  currently_parsed_function_range_ = new SourceRange(
      src_mgr_.getLocForStartOfFile(src_mgr_.getMainFileID()),
      src_mgr_.getLocForStartOfFile(src_mgr_.getMainFileID()));

  rewriter_.setSourceMgr(src_mgr_, lang_option_);
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
  label_srclocation_list_->push_back(start_loc);
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
  {
    string token{rewriter_.ConvertToString(e)};

    // convert to int value if it is a char literal
    if (token.front() == '\'' && token.back() == '\'')
      token = ConvertCharStringToIntString(token);

    // local constants
    if (LocationIsInRange(e->getLocStart(), 
                          *currently_parsed_function_range_))  
    {
      // If the constant is not in the cache, add this new entity into
      // the cache and the vector storing local consts.
      // Else, do nothing.
      if (local_scalar_constant_cache_.find(token) == local_scalar_constant_cache_.end())
      {
        local_scalar_constant_cache_.insert(token);
        local_scalarconstant_list_.push_back(make_pair(
            token, make_pair(e->getLocStart(), 
                             e->getType().getTypePtr()->isFloatingType())));
      }
    }
    // If the constant is not in the cache, add this new entity into
    // the cache and the vector storing global consts.
    // Else, do nothing.
    else if (
      global_scalar_constant_cache_.find(token) == global_scalar_constant_cache_.end())
    {
      global_scalar_constant_cache_.insert(token);
      global_scalarconstant_list_.push_back(make_pair(token,
                    ((e->getType()).getTypePtr())->isFloatingType()));
    }
  }
  else if (isa<StringLiteral>(e))
  {
    SourceLocation start_loc = e->getLocStart();
    string string_literal{rewriter_.ConvertToString(e)};

    if (LocationIsInRange(start_loc, *currently_parsed_function_range_))
    {
      // local string literal
      // if there is the SAME string in the SAME function,
      // then dont add

      auto it = local_stringliteral_list_.begin();

      while (it != local_stringliteral_list_.end() &&
             (it->first.compare(string_literal) != 0 ||
              (it->first.compare(string_literal) == 0 &&
               !LocationIsInRange(it->second, 
                                  *currently_parsed_function_range_))))
        ++it;

      if (it == local_stringliteral_list_.end())
        local_stringliteral_list_.push_back(make_pair(string_literal, 
                                                      start_loc));
    }
    else
    {
      // global string literal
      // Insert if global string vector does not contain this literal
      if (!StringIsInVector(string_literal, global_stringliteral_list_))
        global_stringliteral_list_.push_back(string_literal);
    }
  }

  return true;
}

bool InformationVisitor::VisitFunctionDecl(FunctionDecl *fd)
{
  if (fd->hasBody() && 
      !LocationIsInRange(fd->getLocStart(), 
                         *currently_parsed_function_range_))
  {
    if (currently_parsed_function_range_ != nullptr)
      delete currently_parsed_function_range_;

    currently_parsed_function_range_ = new SourceRange(fd->getLocStart(), 
                                                       fd->getLocEnd());

    // clear the constants of the previously parsed function
    // get ready to contain constants of the new function
    local_scalar_constant_cache_.clear(); 
  }

  return true;
}