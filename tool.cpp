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
#include <map>
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
#include "user_input.h"
#include "mutant_operator.h"
#include "mutant_operator_holder.h"

using namespace clang;
using namespace std;

// <line number, column number>
typedef pair<int, int> LabelStmtLocation; 

// list of locations of goto statements
typedef vector<SourceLocation> GotoStmtLocationList;

// Block scope are bounded by curly braces {}.
// The closer the scope is to the end_loc of vector, the smaller it is.
// ScopeRangeList = {scope1, scope2, scope3, ...}
// {...scope1
//   {...scope2
//     {...scope3
//     }
//   }
// }
typedef vector<SourceRange> ScopeRangeList;  

typedef vector<VarDecl *> VarDeclList;

// pair<string representation, isFloat?>
typedef vector<pair<string, bool>> GlobalScalarConstantList;

// pair<string representation, pair<location, isFloat?>>
typedef vector<pair<string, pair<SourceLocation, bool>>> LocalScalarConstantList;

// pair<range of switch statement, list of case values' string representation>
typedef vector<pair<SourceRange, vector<string>>> SwitchStmtInfoList;

typedef vector<string> GlobalStringLiteralList;
typedef vector<pair<string, SourceLocation>> LocalStringLiteralList;

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
typedef map<LabelStmtLocation, GotoStmtLocationList, comp> LabelStmtToGotoStmtListMap;

typedef vector<string> ScalarReferenceNameList;

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

// Print out each elemet of a string vector in a single line.
void PrintStringVector(vector<string> &string_vector)
{
  for (vector<string>::iterator it = string_vector.begin(); 
      it != string_vector.end(); ++it) {
    std::cout << "//" << *it;
  }
  std::cout << "//\n";
}

// remove spaces at the beginning and end_loc of string str
string TrimBeginningAndEndingWhitespace(string str)
{
  string whitespace{" "};

  auto first_non_whitespace_index = str.find_first_not_of(whitespace);

  if (first_non_whitespace_index == string::npos)
    return ""; // no content

  auto last_non_whitespace_index = str.find_last_not_of(whitespace);
  auto string_range = last_non_whitespace_index - first_non_whitespace_index + 1;
  return str.substr(first_non_whitespace_index, string_range);
}

// Divide string into elements of a string set with delimiter
void SplitStringIntoSet(string target, set<string> &out_set, string delimiter)
{
  size_t pos = 0;
  string token;

  while ((pos = target.find(delimiter)) != string::npos) {
    token = target.substr(0, pos);
    out_set.insert(TrimBeginningAndEndingWhitespace(token));
    target.erase(0, pos + delimiter.length());
  }

  out_set.insert(TrimBeginningAndEndingWhitespace(target));
}

void SplitStringIntoVector(string target, vector<string> &out_vector, string delimiter)
{
  size_t pos = 0;
  string token;

  while ((pos = target.find(delimiter)) != string::npos) {
    token = target.substr(0, pos);
    out_vector.push_back(token);
    target.erase(0, pos + delimiter.length());
  }
  out_vector.push_back(target);
}

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

/**
  @param  hexa: hexa string of the following form "'\xF...F'"
  @return string of the integer value of the given hexa string
*/
string ConvertHexaStringToIntString(string hexa_str)
{
  char first_hexa_digit = hexa_str.at(3);

  if (!((first_hexa_digit >= '0' && first_hexa_digit <= '9') || 
    (first_hexa_digit >= 'a' && first_hexa_digit <= 'f') || 
    (first_hexa_digit >= 'A' && first_hexa_digit <= 'F')))
    return hexa_str;

  char secondh_hexa_digit = hexa_str.at(4);
  
  if (secondh_hexa_digit != '\'')
    if (!((secondh_hexa_digit >= '0' && secondh_hexa_digit <= '9') || 
        (secondh_hexa_digit >= 'a' && secondh_hexa_digit <= 'f') || 
        (secondh_hexa_digit >= 'A' && secondh_hexa_digit <= 'F')))
      return hexa_str;

  return to_string(stoul(hexa_str.substr(3, hexa_str.length() - 4), nullptr, 16));
}

string ConvertCharStringToIntString(string s)
{
  // it is a single, non-escaped character like 'a'
  if (s.length() == 3)
    return to_string(int(s.at(1)));

  if (s.at(1) == '\\')
  {
    // it is an escaped character like '\n'

    int length = s.length() - 3;

    switch (s.at(2))
    {
      case 'a':
        if (length == 1)
          return to_string(int('\a'));
        break;
      case 'b':
        if (length == 1)
          return to_string(int('\b'));
        break;
      case 'f':
        if (length == 1)
          return to_string(int('\f'));
        break;
      case 'n':
        if (length == 1)
          return to_string(int('\n'));
        break;
      case 'r':
        if (length == 1)
          return to_string(int('\r'));
        break;
      case 't':
        if (length == 1)
          return to_string(int('\t'));
        break;
      case 'v':
        if (length == 1)
          return to_string(int('\v'));
        break;
      case '0':
        if (length == 1)
          return to_string(int('\0'));
        break; 
      case '\\':
      case '\'':
      case '\"':
        if (length == 1)
          return to_string(int(s.at(2)));
        break;
      case 'x':
      case 'X':
        if (length <= 3 && length > 1)
          return ConvertHexaStringToIntString(s);
        else 
          return s;   // hexadecimal value higher than FF. not a char
      default:
        cout << "cannot convert " << s << " to string of int\n";
        return s;  
    }
  }
  
  // the function does not handle cases like 'abc'
  cout << "cannot convert " << s << " to string of int\n";
  return s;
}

bool StringIsInVector(string s, vector<string> &string_vector)
{
  auto it = string_vector.begin();

  while (it != string_vector.end() && (*it).compare(s) != 0)
    ++it;

  // if the iterator reach the end_loc then the string is NOT in vector v
  return !(it == string_vector.end());
}

// Return the number of not-newline character
int CountNonNewlineChar(string &s)
{  
  int res = 0;
  for (int i = 0; i < s.length(); ++i)
  {
    if (s[i] != '\n')
      ++res;
  }
  return res;
}

int CountNonNullStmtInCompoundStmt(CompoundStmt *c)
{
  int res{0};

  for (CompoundStmt::body_iterator it = c->body_begin(); it != c->body_end(); ++it)
  {
    if (!isa<NullStmt>(*it))
      ++res;
  }

  return res;
}

bool ConvertStringToInt(string s, int &n)
{
  stringstream convert(s);
  if (!(convert >> n))
    return false;
  return true;
}

/**
  @param  s string literal from C input file
      pos index at which to check if the character there is a whitespace
  @return True if the character at pos is a whitespace
      False otherwise
      If the character at pos is whitespace, pos is changed to the
      position after the whitespace
*/
bool IsWhitespace(string s, int& pos)
{
  if (s[pos] == ' ')
  {
    ++pos;
    return true;
  }

  if (s[pos] == '\\')
  {
    if (s[pos+1] == 'f' || s[pos+1] == 'n' ||
      s[pos+1] == 'r' || s[pos+1] == 't' ||
      s[pos+1] == 'v')
    {
      pos += 2;
      return true;
    }

    if (pos + 3 < s.length())
    {
      string temp{s.substr(pos,4)};
      if (temp.compare("\\011") == 0 ||
        temp.compare("\\012") == 0 ||
        temp.compare("\\013") == 0 ||
        temp.compare("\\014") == 0 ||
        temp.compare("\\015") == 0)
      {
        pos += 4;
        return true;
      }
    }
  }

  return false;
}

/**
  @param  s string literal from input file
  @return index of the first non whitespace character (whitespace, \f, \n, \r, \t, \v)

  Return an int higher than 1
*/
int GetFirstNonWhitespaceIndex(string s)
{
  // Skip the first character which is double quote
  int ret{1};

  while (IsWhitespace(s, ret)) {}

  return ret;
}

int GetLastNonWhitespaceIndex(string s)
{
  int ret{GetFirstNonWhitespaceIndex(s)};
  int i = ret;
  int length{(int) s.length()};

  while (i != length-1)
  {   
    // If the character is not whitespace, change ret position.
    if (!IsWhitespace(s, i))
    {
      // i is not changed because this character is NOT whitespace
      if (s[i] == '\\')  // escaped character
        i += 2;
      else
        ++i;

      ret = i - 1;
    }
  }

  return ret;
}

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
  SourceManager &src_mgr_;
  LangOptions &lang_option_;
  CompilerInstance *comp_inst_;
  Rewriter rewriter_;
  // ASTContext &m_astcontext;

  // ID of the next mutated code file
  int next_mutant_file_id_;

  int proteumstyle_stmt_start_line_num_;
  int proteumstyle_stmt_end_line_num_;

  // name of mutated code file = mutant_filename_ + next_mutant_file_id_
  // mutant_filename_ = (input filename without .c) + ".MUT"
  string mutant_filename_;

  // analyzed user's input
  UserInput *userinput_;

  // this object holds all info about mutant operators to be applied
  MutantOperatorHolder *mutantoperator_holder_;

  bool m_checkForZero;

  // True when visit Statement Expression,
  // False when out of Statement Expression.
  bool is_inside_stmtexpr_;

  // True when enter array size of array declaration
  // False when out of array size of array declaration.
  bool is_inside_array_decl_size_;

  // True if the next Binary Operator is the expression evaluating to array size.
  bool first_binaryop_after_arraydecl_;

  bool is_inside_enumdecl_;

  // Expression evaluating to array size.
  // Used to prevent generating-negative-array-size mutants.
  BinaryOperator *arraydecl_size_expr_;

  // Store the locations of all the labels inside input file.
  vector<SourceLocation> *label_srclocation_list_;

  // Map label name to label usages (goto)
  LabelStmtToGotoStmtListMap *label_to_gotolist_map_;

  ScopeRangeList scope_list_;

  // global/local number, char literals
  GlobalScalarConstantList *global_scalarconstant_list_;
  LocalScalarConstantList *local_scalarconstant_list_;

  // global/local string literals
  GlobalStringLiteralList *global_stringliteral_list_;
  LocalStringLiteralList *local_stringliteral_list_;

  // global/local scalar variables (char, int, double, float)
  // local_scalar_vardecl_list_ follows the same nesting rule as ScopeRangeList
  vector<VarDecl *> global_scalar_vardecl_list_;
  vector<VarDeclList> local_scalar_vardecl_list_;

  // global/local array variables
  // local_array_vardecl_list_ follows the same nesting rule as ScopeRangeList
  vector<VarDecl *> global_array_vardecl_list_;
  vector<VarDeclList> local_array_vardecl_list_;

  // global/local struct variables
  // local_struct_vardecl_list_ follows the same nesting rule as ScopeRangeList
  vector<VarDecl *> global_struct_vardecl_list_;
  vector<VarDeclList> local_struct_vardecl_list_;

  // global/local pointers
  // local_pointer_vardecl_list_ follows the same nesting rule as ScopeRangeList
  vector<VarDecl *> global_pointer_vardecl_list_;
  vector<VarDeclList> local_pointer_vardecl_list_;

  // bool isInsideFunctionDecl;

  // Range of the latest parsed array declaration statement
  SourceRange *array_decl_range_;

  // Range of an expression that should not be zero
  SourceRange *target_expr_range_;

  // Range of the currently parsed function
  // used to capture variables, references inside currently parsed ftn
  SourceRange *currently_parsed_function_range_;

  // The following range are used to prevent certain uncompilable mutations
  SourceRange *lhs_of_assignment_range_;     // lhs = rhs
  SourceRange *switchstmt_condition_range_;  // switch (cond)
  SourceRange *typedef_range_;      
  SourceRange *addressop_range_;    // &(var)
  SourceRange *functionprototype_range_;  // Type FunctionName(params);
  SourceRange *unary_increment_range_;   // var++ OR ++var
  SourceRange *unary_decrement_range_;   // var-- OR --var
  SourceRange *arraysubscript_range_;   // arr[index]
  SourceRange *switchcase_range_;     // case label:

  // variable declaration inside a struct or union is called field declaration
  SourceRange *fielddecl_range_; 

  // range of bitwise, shift, and modulo operator
  // OCOR cannot mutate to float type
  SourceRange *non_OCOR_mutatable_expr_range_;

  Sema *sema_;

  SwitchStmtInfoList switchstmt_info_list_;

  ScalarReferenceNameList non_VTWD_mutatable_scalarref_list_;

  bool OCOR_mutates_to_int_type_only;

  MyASTVisitor(SourceManager &src_mgr, LangOptions &lang_option, 
               UserInput *user_input, MutantOperatorHolder *holder, 
               CompilerInstance *CI, vector<SourceLocation> *labels, 
               LabelStmtToGotoStmtListMap *label_to_gotolist_map, 
               GlobalScalarConstantList *global_scalar_constant_list, 
               LocalScalarConstantList *local_scalar_constant_list, 
               GlobalStringLiteralList *global_string_literal_list, 
               LocalStringLiteralList *local_string_literal_list) 
    : src_mgr_(src_mgr), lang_option_(lang_option), 
      mutantoperator_holder_(holder), userinput_(user_input), 
      next_mutant_file_id_(1), comp_inst_(CI), 
      label_srclocation_list_(labels), 
      label_to_gotolist_map_(label_to_gotolist_map),
      global_scalarconstant_list_(global_scalar_constant_list), 
      local_scalarconstant_list_(local_scalar_constant_list),
      global_stringliteral_list_(global_string_literal_list), 
      local_stringliteral_list_(local_string_literal_list)
  {
    proteumstyle_stmt_end_line_num_ = 0;

    // name of mutated code file = mutant_filename_ + next_mutant_file_id_
    // mutant_filename_ = (input filename without .c) + ".MUT"
    mutant_filename_.assign(userinput_->getInputFilename(), 
                          0, 
                          (userinput_->getInputFilename()).length()-2);
    mutant_filename_ += ".MUT";

    // setup for rewriter
    rewriter_.setSourceMgr(src_mgr_, lang_option_);

    m_checkForZero = false;
    is_inside_stmtexpr_ = false;
    is_inside_array_decl_size_ = false;
    first_binaryop_after_arraydecl_ = false;
    // isInsideFunctionDecl = false;
    is_inside_enumdecl_ = false;
    OCOR_mutates_to_int_type_only = false;

    // set range variables to a 0 range, start_loc and end_loc at the same point
    SourceLocation start_of_file;
    start_of_file = src_mgr_.getLocForStartOfFile(src_mgr_.getMainFileID());

    lhs_of_assignment_range_ = new SourceRange(start_of_file, start_of_file);
    switchstmt_condition_range_ = new SourceRange(start_of_file, start_of_file);
    typedef_range_ = new SourceRange(start_of_file, start_of_file);
    addressop_range_ = new SourceRange(start_of_file, start_of_file);
    functionprototype_range_ = new SourceRange(start_of_file, start_of_file);
    unary_increment_range_ = new SourceRange(start_of_file, start_of_file);
    unary_decrement_range_ = new SourceRange(start_of_file, start_of_file);
    currently_parsed_function_range_ = new SourceRange(start_of_file, start_of_file);
    arraysubscript_range_ = new SourceRange(start_of_file, start_of_file);
    fielddecl_range_ = new SourceRange(start_of_file, start_of_file); 
    switchcase_range_ = new SourceRange(start_of_file, start_of_file); 
    non_OCOR_mutatable_expr_range_ = new SourceRange(start_of_file, start_of_file); 

    array_decl_range_ = nullptr;
    target_expr_range_ = nullptr;

    /*cout << "global const: " << endl;
    for (auto e: *global_scalarconstant_list_)
      cout << "//" << e.first;

    cout << endl << "local const:" << endl;
    for (auto e: *local_scalarconstant_list_)
    {
      cout << "const: " << e.first << endl;
      PrintLocation(e.second.first);
    }*/
  }

  /*void PrintIdentifierTable(const ASTContext &astcontext)
  {
    IdentifierTable &identTable = astcontext.Idents;
    // identTable.AddKeywords(lang_option_);
    for (IdentifierTable::iterator e = identTable.begin(); e != identTable.end(); ++e)
    {
      cout << string(e->first()) << endl;
    }
  }*/

  void setSema(Sema *sema)
  {
    sema_ = sema;
  }

  string getVarDeclName(VarDecl *vd)
  {
    return vd->getNameAsString();
  }

  bool VarDeclIsConst(VarDecl *vd)
  {
    return (vd->getType()).isConstQualified();
  }

  bool VarDeclIsPointer(VarDecl *vd)
  {
    return ((vd->getType()).getTypePtr())->isPointerType();   
  }

  bool VarDeclIsArray(VarDecl *vd)
  {
    return ((vd->getType()).getTypePtr())->isArrayType();   
  }

  bool VarDeclIsScalar(VarDecl *vd)
  {
    return ((vd->getType()).getTypePtr())->isScalarType() && 
            !VarDeclIsPointer(vd);   
  }

  bool VarDeclIsFloating(VarDecl *vd)
  {
    return ((vd->getType()).getTypePtr())->isFloatingType();
  }

  bool VarDeclIsStruct(VarDecl *vd)
  {
    return ((vd->getType()).getTypePtr())->isStructureType();
  }

  void PrintAllGlobalScalarVarDecl()
  {
    cout << "all global scalars:\n";
    for (auto e: global_scalar_vardecl_list_)
    {
      cout << getVarDeclName(e) << "\t" << VarDeclIsConst(e) << "\n";
    }
    cout << "end_loc all global scalars\n";
  }

  void PrintAllGlobalArrayVarDecl()
  {
    cout << "all global arrays:\n";
    for (auto e: global_array_vardecl_list_)
    {
      cout << getVarDeclName(e) << "\t" << VarDeclIsConst(e) << "\n";
    }
    cout << "end_loc all global arrays\n";
  }

  void PrintAllLocalScalarVarDecl()
  {
    cout << "all local scalars:\n";
    for (auto e: local_scalar_vardecl_list_)
    {
      cout << "new scope\n";
      for (auto scalar: e)
        cout << getVarDeclName(scalar) << "\t" << VarDeclIsConst(scalar) << "\n";
    }
    cout << "end_loc all local scalars\n";
  }

  void PrintAllLocalArrayVarDecl()
  {
    cout << "all local arrays:\n";
    for (auto e: local_array_vardecl_list_)
    {
      cout << "new scope\n";
      for (auto scalar: e)
        cout << getVarDeclName(scalar) << "\t" << VarDeclIsConst(scalar) << "\n";
    }
    cout << "end_loc all local arrays\n";
  }

  /**
    Return the type of array element
    Example: int[] -> int

    @param: type  type of the array
  */
  string getArrayElementType(QualType type)
  {
    string res;

    // getElementType can only be called from an ArrayType
    if (const ArrayType *type_ptr = dyn_cast<ArrayType>((type.getTypePtr())))
    {
      QualType array_type = type_ptr->getElementType().getCanonicalType();
      res = TrimBeginningAndEndingWhitespace(array_type.getAsString());
    }
    else 
    {
      // Since type parameter is definitely an array type,
      // if somehow cannot convert to ArrayType, use string processing to retrieve element type.
      res = type.getCanonicalType().getAsString();
      auto pos = res.find_last_of('[');
      res = TrimBeginningAndEndingWhitespace(res.substr(0, pos));
    }

    // remove const from element type. why?
    /*string firstWord = res.substr(0, res.find_first_of(' '));
    if (firstWord.compare("const") == 0)
      res = res.substr(6);*/
    
    return res;
  }

  string getStructureType(QualType type)
  {
    return type.getCanonicalType().getAsString();
  }

  // Return the type of the entity the pointer is pointing to.
  string getPointerType(QualType type)
  {
    const Type *type_ptr = cast<PointerType>(type.getCanonicalType().getTypePtr());
    return type_ptr->getPointeeType().getCanonicalType().getAsString();
  }

  // Return True if the 2 types are same
  bool sameArrayElementType(QualType type1, QualType type2)
  {
    if (!type1.getTypePtr()->isArrayType() || 
        !type2.getTypePtr()->isArrayType())
    {
      cout << "sameArrayElementType: one of the type is not array type\n";
      return false;
    }

    string elementType1 = getArrayElementType(type1);
    string elementType2 = getArrayElementType(type2);

    return elementType1.compare(elementType2) == 0;
  }

  // Return True if location loc appears before range. 
  // (location 1,1 appears before every other valid locations)
  bool LocationBeforeRangeStart(SourceLocation loc, SourceRange range)
  {
    SourceLocation start_loc = range.getBegin();

    return (getLineNumber(&loc) < getLineNumber(&start_loc) ||
            (getLineNumber(&loc) == getLineNumber(&start_loc) && 
              getColNumber(&loc) < getColNumber(&start_loc)));
  }

  // Return True if location loc appears after range. 
  // (EOF appears after every other valid locations)
  bool LocationAfterRangeEnd(SourceLocation loc, SourceRange range)
  {
    SourceLocation end_loc = range.getEnd();

    return (getLineNumber(&loc) > getLineNumber(&end_loc) ||
            (getLineNumber(&loc) == getLineNumber(&end_loc) && 
              getColNumber(&loc) > getColNumber(&end_loc)));
  }

  // Return True if the location is inside the range
  // Return False otherwise
  bool LocationIsInRange(SourceLocation loc, SourceRange range)
  {
    SourceLocation start_of_range = range.getBegin();
    SourceLocation end_of_range = range.getEnd();

    // line number of loc is out of range
    if (getLineNumber(&loc) < getLineNumber(&start_of_range) || 
        getLineNumber(&loc) > getLineNumber(&end_of_range))
      return false;

    // line number of loc is same as line number of start_of_range 
    // but col number of loc is out of range
    if (getLineNumber(&loc) == getLineNumber(&start_of_range) && 
        getColNumber(&loc) < getColNumber(&start_of_range))
      return false;

    // line number of loc is same as line number of end_loc 
    // but col number of loc is out of range
    if (getLineNumber(&loc) == getLineNumber(&end_of_range) && 
        getColNumber(&loc) > getColNumber(&end_of_range))
      return false;

    return true;
  }

  // Return True if there is no labels inside specified code range.
  // Return False otherwise.
  bool NoLabelStmtInsideRange(SourceRange range)
  {
    // Return false if there is a label inside the range
    for (auto label_loc: *(label_srclocation_list_))
    {
      if (LocationIsInRange(label_loc, range))
      {
        return false;
      }
    }

    return true;
  }

  // an unremovable label is a label defined inside range stmtRange,
  // but goto-ed outside of range stmtRange.
  // Deleting such label can cause goto-undefined-label error.
  // Return True if there is no such label inside given range
  bool NoUnremovableLabelInsideRange(SourceRange range)
  {
    for (auto element: *label_to_gotolist_map_)
    {
      SourceLocation loc = src_mgr_.translateLineCol(src_mgr_.getMainFileID(),
                              element.first.first,
                              element.first.second);
      // check only those labels inside range
      if (LocationIsInRange(loc, range))
      {
        // check if this label is goto-ed from outside of the statement
        // if yes, then the label is unremovable, return False
        for (auto e: element.second)  
        {   
          // the goto is outside of the statement
          if (!LocationIsInRange(e, range))  
            return false;
        }
      }

      // the LabelStmtToGotoStmtListMap is traversed in the order of 
      // increasing location. If the location is after the range then 
      // all the rest is outside statement range
      if (LocationAfterRangeEnd(loc, range))
        return true;
    }

    return true;
  }

  // Return True if given operator is an arithmetic operator.
  bool OperatorIsArithmetic(const string &mutant_name) 
  {
    return mutant_name == "+" || mutant_name == "-" || 
        mutant_name == "*" || mutant_name == "/" || mutant_name == "%";
  }

  // Return the line number of a source location.
  int getLineNumber(SourceLocation *loc) 
  {
    return static_cast<int>(src_mgr_.getExpansionLineNumber(*loc));
  }

  // Return the column number of a source location.
  int getColNumber(SourceLocation *loc) 
  {
    return static_cast<int>(src_mgr_.getExpansionColumnNumber(*loc));
  }   

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
      if (addressop_range_ != nullptr)
        delete addressop_range_;

      addressop_range_ = new SourceRange(*start_loc, *end_loc);
    }
  }

  string ConvertIntToString(int num)
  {
    stringstream convert;
    convert << num;
    string ret;
    convert >> ret;
    return ret;
  }

  // Return the name of the next mutated code file
  string getNextMutantFilename() {
    string ret{userinput_->getOutputDir()};
    ret += mutant_filename_;
    ret += ConvertIntToString(next_mutant_file_id_);
    ret += ".c";
    return ret;
  }

  /**
    Write information about the next mutated code file into mutation database file.

    @param  mutant_name name of the operator
        start_loc start_loc location of the token before mutation
        end_loc end_loc location of the token before mutation
        newstartloc start_loc location of the token after mutation
        newendloc end_loc location of the token after mutation
        token targeted token
        newtoken the token to be used to replace targeted token
  */
  void WriteMutantInfoToDatabaseFile(string &mutant_name, SourceLocation *start_loc, 
                        SourceLocation *end_loc, SourceLocation *newstartloc, 
                        SourceLocation *newendloc, string &token, 
                        string &newtoken) 
  {
    // Open mutattion database file in APPEND mode (write to the end_loc of file)
    ofstream mutant_db_file((userinput_->getMutationDbFilename()).data(), 
                            ios::app);

    mutant_db_file << userinput_->getInputFilename() << "\t";

    // write output file name
    mutant_db_file << mutant_filename_ << next_mutant_file_id_ << "\t"; 

    // write name of operator  
    mutant_db_file << mutant_name << "\t";                

    mutant_db_file << proteumstyle_stmt_start_line_num_ << "\t"; 
    
    // write information about token BEFORE mutation (range and token)
    mutant_db_file << getLineNumber(start_loc) << "\t";       
    mutant_db_file << getColNumber(start_loc) << "\t";
    mutant_db_file << getLineNumber(end_loc) << "\t";
    mutant_db_file << getColNumber(end_loc) << "\t";
    mutant_db_file << token << "\t";

    // write information about token AFTER mutation (range and token)
    mutant_db_file << getLineNumber(newstartloc) << "\t";
    mutant_db_file << getColNumber(newstartloc) << "\t";
    mutant_db_file << getLineNumber(newendloc) << "\t";
    mutant_db_file << getColNumber(newendloc) << "\t";
    mutant_db_file << newtoken << endl;

    mutant_db_file.close(); 
  }

  /**
    Make a new mutated code file in the specified output directory

    @param  mutant_name name of the operator
        start_loc start_loc location of the token before mutation
        end_loc end_loc location of the token before mutation
        token targeted token
        mutated_token the token to be used to replace targeted token 
  */
  void GenerateMutant(string &mutant_name, SourceLocation *start_loc, 
                    SourceLocation *end_loc, string &token, 
                    string &mutated_token) 
  {
    // Create a Rewriter objected needed for edit input file
    Rewriter rewriter;
    rewriter.setSourceMgr(src_mgr_, lang_option_);
    
    // Replace the string started at start_loc, length equal to token's length
    // with the mutated_token. 
    rewriter.ReplaceText(*start_loc, token.length(), mutated_token);

    string mutant_filename;
    mutant_filename = getNextMutantFilename();

    // Make and write mutated code to output file.
    const RewriteBuffer *rewrite_buf = rewriter.getRewriteBufferFor(src_mgr_.getMainFileID());
    ofstream output(mutant_filename.data());
    output << string(rewrite_buf->begin(), rewrite_buf->end());
    output.close();

    SourceLocation new_end_loc = src_mgr_.translateLineCol(src_mgr_.getMainFileID(),
                              getLineNumber(start_loc),
                              getColNumber(start_loc) + \
                              CountNonNewlineChar(mutated_token));

    // Write information of the new mutated file into mutation database file
    WriteMutantInfoToDatabaseFile(mutant_name, start_loc, end_loc, start_loc, 
                                  &new_end_loc, token, mutated_token);

    // each output mutated code file needs to have different name.
    ++next_mutant_file_id_;
  }

  /*void generateSdlMutant(string &mutant_name, SourceLocation *start_loc, SourceLocation *end_loc, 
            string &token, string &mutated_token) 
  {
    // Create a Rewriter objected needed for edit input file
    Rewriter theRewriter;
    theRewriter.setSourceMgr(src_mgr_, lang_option_);

    int length = src_mgr_.getFileOffset(*end_loc) - src_mgr_.getFileOffset(*start_loc);

    theRewriter.ReplaceText(*start_loc, length, mutated_token);

    string mutant_filename;
    mutant_filename = getNextMutantFilename();

    // Make and write mutated code to output file.
    const RewriteBuffer *RewriteBuf = theRewriter.getRewriteBufferFor(src_mgr_.getMainFileID());
    ofstream output(mutant_filename.data());
    output << string(RewriteBuf->begin(), RewriteBuf->end());
    output.close();

    SourceLocation newendloc = src_mgr_.translateLineCol(src_mgr_.getMainFileID(),
                              getLineNumber(start_loc),
                              getColNumber(start_loc)+CountNonNewlineChar(mutated_token));

    // Write information of the new mutated file into mutation database file
    WriteMutantInfoToDatabaseFile(mutant_name, start_loc, end_loc, start_loc, &newendloc, token, mutated_token);

    // each output mutated code file needs to have different name.
    ++next_mutant_file_id_;
  }*/

  void GenerateMutant_new(string &mutant_name, SourceLocation *start_loc, 
                          SourceLocation *end_loc, string &token, 
                          string &mutated_token) 
  {
    // Create a Rewriter objected needed for edit input file
    Rewriter rewriter;
    rewriter.setSourceMgr(src_mgr_, lang_option_);
    
    int length = src_mgr_.getFileOffset(*end_loc) - \
                  src_mgr_.getFileOffset(*start_loc);

    rewriter.ReplaceText(*start_loc, length, mutated_token);

    string mutant_filename;
    mutant_filename = getNextMutantFilename();

    // Make and write mutated code to output file.
    const RewriteBuffer *RewriteBuf = rewriter.getRewriteBufferFor(src_mgr_.getMainFileID());
    ofstream output(mutant_filename.data());
    output << string(RewriteBuf->begin(), RewriteBuf->end());
    output.close();

    SourceLocation new_end_loc = src_mgr_.translateLineCol(src_mgr_.getMainFileID(),
                                        getLineNumber(start_loc),
                                        getColNumber(start_loc) + \
                                        CountNonNewlineChar(mutated_token));

    // Write information of the new mutated file into mutation database file
    WriteMutantInfoToDatabaseFile(mutant_name, start_loc, end_loc, start_loc, 
                                  &new_end_loc, token, mutated_token);

    // each output mutated code file needs to have different name.
    ++next_mutant_file_id_;
  }

  // Return True if the given range is within the mutation range
  bool TargetRangeIsInMutationRange(SourceLocation *start_loc, 
                                    SourceLocation *end_loc)
  {
    // If line is out of bound, then this token is not mutated
    if ((getLineNumber(start_loc) < getLineNumber(userinput_->getStartOfMutationRange())) ||
      (getLineNumber(end_loc) > getLineNumber(userinput_->getEndOfMutationRange())))
    {
      // cout << "false\n";
      return false;
    }

    // If column is out of bound, then this token is not mutated
    if ((getLineNumber(start_loc) == getLineNumber(userinput_->getStartOfMutationRange())) &&
      (getColNumber(start_loc) < getColNumber(userinput_->getStartOfMutationRange())))
    {
      // cout << "false\n";
      return false;
    }

    if ((getLineNumber(end_loc) == getLineNumber(userinput_->getEndOfMutationRange())) &&
      (getColNumber(end_loc) > getColNumber(userinput_->getEndOfMutationRange())))
    {
      return false;
    }

    return true;
  }

  SourceLocation GetEndLocOfStmt(SourceLocation *loc)
  {
    SourceLocation ret{};

    if (*(src_mgr_.getCharacterData(*loc)) == '}' || 
        *(src_mgr_.getCharacterData(*loc)) == ';' ||
        *(src_mgr_.getCharacterData(*loc)) == ']' || 
        *(src_mgr_.getCharacterData(*loc)) == ')')
      ret = (*loc).getLocWithOffset(1);
    else
      ret = clang::Lexer::findLocationAfterToken(*loc,
                              tok::semi, src_mgr_,
                              lang_option_, 
                              false);
    return ret;
  }

  /**
    @param  start_loc: start_loc location of the targeted literal
    @return end_loc location of number, char literal
  */
  SourceLocation GetEndLocOfConstantLiteral(SourceLocation start_loc)
  {
    int line_num = getLineNumber(&start_loc);
    int col_num = getColNumber(&start_loc);

    SourceLocation ret = src_mgr_.translateLineCol(src_mgr_.getMainFileID(), 
                                                  line_num, col_num);

    // a char starts and ends with a single quote
    bool is_char_literal = false;

    if (*(src_mgr_.getCharacterData(ret)) == '\'')
      is_char_literal = true;

    if (is_char_literal)
    {
      ret = ret.getLocWithOffset(1);

      while (*(src_mgr_.getCharacterData(ret)) != '\'')
      {
        // if there is a backslash then skip the next character
        if (*(src_mgr_.getCharacterData(ret)) == '\\')
        {
          ret = ret.getLocWithOffset(1);
        }

        ret = ret.getLocWithOffset(1);
      }

      // End of while loop result in location right before a single quote
      ret = ret.getLocWithOffset(1);
    }
    else  // not char
    {
      // Here, I am assuming the appearance of these characters
      // signals the end_loc of a number literal.
      while (*(src_mgr_.getCharacterData(ret)) != ' ' &&
          *(src_mgr_.getCharacterData(ret)) != ';' &&
          *(src_mgr_.getCharacterData(ret)) != '+' &&
          *(src_mgr_.getCharacterData(ret)) != '-' &&
          *(src_mgr_.getCharacterData(ret)) != '*' &&
          *(src_mgr_.getCharacterData(ret)) != '/' &&
          *(src_mgr_.getCharacterData(ret)) != '%' &&
          *(src_mgr_.getCharacterData(ret)) != ',' &&
          *(src_mgr_.getCharacterData(ret)) != ')' &&
          *(src_mgr_.getCharacterData(ret)) != ']' &&
          *(src_mgr_.getCharacterData(ret)) != '}' &&
          *(src_mgr_.getCharacterData(ret)) != '>' &&
          *(src_mgr_.getCharacterData(ret)) != '<' &&
          *(src_mgr_.getCharacterData(ret)) != '=' &&
          *(src_mgr_.getCharacterData(ret)) != '!' &&
          *(src_mgr_.getCharacterData(ret)) != '|' &&
          *(src_mgr_.getCharacterData(ret)) != '?' &&
          *(src_mgr_.getCharacterData(ret)) != '&' &&
          *(src_mgr_.getCharacterData(ret)) != '~' &&
          *(src_mgr_.getCharacterData(ret)) != '`' &&
          *(src_mgr_.getCharacterData(ret)) != ':' &&
          *(src_mgr_.getCharacterData(ret)) != '\n')
        ret = ret.getLocWithOffset(1);
    }

    return ret;
  }

  /**
    @param  start_loc: start_loc location of the targeted string literal
    @return end_loc location of string literal
  */
  SourceLocation GetEndLocOfStringLiteral(SourceLocation start_loc)
  {
    int line_num = getLineNumber(&start_loc);
    int col_num = getColNumber(&start_loc) + 1;

    // Get the location right AFTER the first double quote
    SourceLocation ret = src_mgr_.translateLineCol(src_mgr_.getMainFileID(), 
                                                  line_num, col_num);

    while (*(src_mgr_.getCharacterData(ret)) != '\"')
    {
      // if there is a backslash then skip the next character
      if (*(src_mgr_.getCharacterData(ret)) == '\\')
      {
        ret = ret.getLocWithOffset(1);
      }

      ret = ret.getLocWithOffset(1);
    }

    ret = ret.getLocWithOffset(1);
    return ret;
  }

  /**
    @param  uo: pointer to expression with unary operator
    @return end_loc location of given expression
  */
  SourceLocation GetEndLocOfUnaryOpExpr(UnaryOperator *uo)
  {
    SourceLocation ret = uo->getLocEnd();

    if (uo->getOpcode() == UO_PostInc || uo->getOpcode() == UO_PostDec)
      // for post increment/decrement, getLocEnd returns 
      // the location right BEFORE ++/--
      ret = ret.getLocWithOffset(2);
    else
      if (uo->getOpcode() == UO_PreInc || uo->getOpcode() == UO_PreDec ||
          uo->getOpcode() == UO_AddrOf || uo->getOpcode() == UO_Deref ||
          uo->getOpcode() == UO_Plus || uo->getOpcode() == UO_Minus ||
          uo->getOpcode() == UO_Not || uo->getOpcode() == UO_LNot)
      {
        // getLocEnd returns the location right AFTER the unary operator
        // end_loc location of the expression is end_loc location of the sub-expr
        Expr *subexpr = uo->getSubExpr()->IgnoreImpCasts();

        SourceLocation start_loc = uo->getLocStart();
        SourceLocation end_loc = uo->getLocEnd();

        ret = GetEndLocOfExpr(subexpr);
      }
      else  // other cases, if any
      {
        ;   // just return clang end_loc loc
      }

    return ret;
  }

  /**
    This function assumes the given location is either before or after
    a semicolon (;). Though I can use a while loop to go back and forth
    until a semicolon is found, multi stmt on a single line can be 
    confusing

    @param  loc: target location with previous assumption
    @return location after semicolon
        if cannot find then return given location
  */
  SourceLocation GetLocationAfterSemicolon(SourceLocation loc)
  {
    SourceLocation previous_loc = src_mgr_.translateLineCol(src_mgr_.getMainFileID(),
                                            getLineNumber(&loc),
                                            getColNumber(&loc) - 1);

    if (*(src_mgr_.getCharacterData(previous_loc)) == ';')
      return loc;

    if (*(src_mgr_.getCharacterData(loc)) == ';')
      return loc.getLocWithOffset(1);

    return loc;
  }

  SourceLocation GetEndLocOfExpr(Expr *e)
  {
    if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
      return GetEndLocOfUnaryOpExpr(uo);

    if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
      return GetEndLocOfExpr(bo->getRHS()->IgnoreImpCasts());

    SourceLocation ret = e->getLocEnd();

    // classify expression and get end_loc location accordingly
    if (isa<ArraySubscriptExpr>(e))
    {
      ret = e->getLocEnd();
      ret = GetEndLocOfStmt(&ret);
    }
    else if (isa<DeclRefExpr>(e) || isa<MemberExpr>(e))
    {
      int length = rewriter_.ConvertToString(e).length();
      SourceLocation start_loc = e->getLocStart();
      ret = src_mgr_.translateLineCol(src_mgr_.getMainFileID(),
                      getLineNumber(&start_loc),
                      getColNumber(&start_loc) + length);
    }
    else if (isa<CharacterLiteral>(e) || 
              isa<FloatingLiteral>(e) || 
              isa<IntegerLiteral>(e))
    {
      ret = GetEndLocOfConstantLiteral(e->getLocStart());
    }
    else if (isa<CStyleCastExpr>(e))  // explicit cast
    {
      return GetEndLocOfExpr(cast<CStyleCastExpr>(e)->getSubExpr()->IgnoreImpCasts());
    }
    else if (ParenExpr *pe = dyn_cast<ParenExpr>(e))
    {
      ret = pe->getRParen();
      ret = ret.getLocWithOffset(1);
    }
    else if (isa<StringLiteral>(e))
    {
      ret = GetEndLocOfStringLiteral(e->getLocStart());
    }
    else 
    {
      ret = e->getLocEnd();
      ret = GetEndLocOfStmt(&ret);

      if (ret.isInvalid())
        ret = e->getLocEnd();

      // GetEndLocOfStmt sometimes returns location after semicolon
      SourceLocation prevLoc = src_mgr_.translateLineCol(src_mgr_.getMainFileID(),
                                getLineNumber(&ret),
                                getColNumber(&ret) - 1);
      if (*(src_mgr_.getCharacterData(prevLoc)) == ';')
        ret = prevLoc;
    }

    return ret;
  }

  SourceLocation GetLeftBracketOfArraySubscript(ArraySubscriptExpr *ase)
  {
    SourceLocation ret = ase->getLocStart();

    while (*(src_mgr_.getCharacterData(ret)) != '[')
      ret = ret.getLocWithOffset(1);

    return ret;
  }

  /**
    @param  scalarref_name: string of name of a declaration reference
    @return True if reference name is not in the prohibited list
        False otherwise
  */
  bool ScalarRefCanBeMutatedByVtwd(string scalarref_name)
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
            if (ScalarRefCanBeMutatedByVtwd(reference_name))
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
            if (ScalarRefCanBeMutatedByVtwd(reference_name))
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
              if (ScalarRefCanBeMutatedByVtwd(reference_name))
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

  void GenerateCrcrMutant(Expr *e, SourceLocation start_loc, SourceLocation end_loc)
  {
    string reference_name{rewriter_.ConvertToString(e)};
    string mutant_name{"CRCR"};

    int mutant_num_limit = userinput_->getLimit();

    const Type *type_ptr = (e->getType().getCanonicalType()).getTypePtr();
    
    if (type_ptr->isIntegralType(comp_inst_->getASTContext()))
    {
      for (auto e: mutantoperator_holder_->CRCR_integral_range_)
      {
        string mutated_token{"(" + e + ")"};
        GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                            reference_name, mutated_token);

        // Apply limit option on number of generated mutants 
        --mutant_num_limit;
        if (mutant_num_limit == 0)
          break;
      }
      return;
    }

    if (((e->getType().getCanonicalType()).getTypePtr())->isFloatingType())
    {
      for (auto e: mutantoperator_holder_->CRCR_floating_range_)
      {
        string mutated_token{"(" + e + ")"};
        GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                            reference_name, mutated_token);
        
        // Apply limit option on number of generated mutants 
        --mutant_num_limit;
        if (mutant_num_limit == 0)
          break;
      }
      return;
    }
  }

  void GenerateVgsrMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string reference_name, string mutant_name)
  {
    // cannot mutate variable in switch condition to a floating-type variable
    bool skip_float_vardecl = LocationIsInRange(*start_loc, 
                                                *switchstmt_condition_range_);

    // cannot mutate a variable in lhs of assignment to a const variable
    bool skip_const_vardecl = LocationIsInRange(*start_loc, 
                                                *lhs_of_assignment_range_);

    int mutant_num_limit = userinput_->getLimit();

    for (auto vardecl: global_scalar_vardecl_list_)
    {
      if (skip_const_vardecl && VarDeclIsConst(vardecl)) 
        continue;   

      if (skip_float_vardecl && VarDeclIsFloating(vardecl))
        continue;

      string mutated_token{getVarDeclName(vardecl)};

      if (reference_name.compare(mutated_token) != 0)
      {
        GenerateMutant_new(mutant_name, start_loc, end_loc, 
                            reference_name, mutated_token);

        // Apply limit option on number of generated mutants 
        --mutant_num_limit;
        if (mutant_num_limit == 0)
          break;
      }
    }
  }

  void GenerateVlsrMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string reference_name, string mutant_name)
  {
    // cannot mutate variable in switch condition to a floating-type variable
    bool skip_float_vardecl = LocationIsInRange(*start_loc, 
                                                *switchstmt_condition_range_);

    // cannot mutate a variable in lhs of assignment to a const variable
    bool skip_const_vardecl = LocationIsInRange(*start_loc, 
                                                *lhs_of_assignment_range_);

    bool skip_register_vardecl = LocationIsInRange(*start_loc, 
                                                    *addressop_range_);

    int mutant_num_limit = userinput_->getLimit();

    for (auto scope: local_scalar_vardecl_list_)
    {
      for (auto vardecl: scope)
      {
        if (skip_const_vardecl && VarDeclIsConst(vardecl)) 
          continue;   

        if (skip_float_vardecl && VarDeclIsFloating(vardecl))
          continue;

        if (skip_register_vardecl && 
            vardecl->getStorageClass() == SC_Register)
        {
          continue;
        }

        string mutated_token{getVarDeclName(vardecl)};

        if (reference_name.compare(mutated_token) != 0)
        {
          GenerateMutant_new(mutant_name, start_loc, end_loc, 
                              reference_name, mutated_token);

          // Apply limit option on number of generated mutants 
          --mutant_num_limit;
          if (mutant_num_limit == 0)
            return;
        }
      }
    }
  }

  void GenerateVgarMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string reference_name, QualType type, 
                          string mutant_name)
  {
    // cannot mutate variable in switch condition to a floating-type variable
    bool skip_float_vardecl = LocationIsInRange(*start_loc, 
                                                *switchstmt_condition_range_);

    // cannot mutate a variable in lhs of assignment to a const variable
    bool skip_const_vardecl = LocationIsInRange(*start_loc, 
                                                *lhs_of_assignment_range_);

    int mutant_num_limit = userinput_->getLimit();

    for (auto vardecl: global_array_vardecl_list_)
    {
      if (skip_const_vardecl && VarDeclIsConst(vardecl)) 
        continue;

      if (skip_float_vardecl && VarDeclIsFloating(vardecl))
        continue;

      string mutated_token{getVarDeclName(vardecl)};

      if (reference_name.compare(mutated_token) != 0 && 
          sameArrayElementType(type, vardecl->getType()))
      {
        GenerateMutant_new(mutant_name, start_loc, end_loc, 
                            reference_name, mutated_token);

        // Apply limit option on number of generated mutants 
        --mutant_num_limit;
        if (mutant_num_limit == 0)
          break;
      }
    }
  }

  void GenerateVlarMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string reference_name, QualType type, 
                          string mutant_name)
  {
    // cannot mutate variable in switch condition to a floating-type variable
    bool skip_float_vardecl = LocationIsInRange(*start_loc, 
                                                *switchstmt_condition_range_);

    // cannot mutate a variable in lhs of assignment to a const variable
    bool skip_const_vardecl = LocationIsInRange(*start_loc, 
                                                *lhs_of_assignment_range_);

    // cannot mutate a variable inside address op to a register variable
    bool skip_register_vardecl = LocationIsInRange(*start_loc, 
                                                    *addressop_range_);

    int mutant_num_limit = userinput_->getLimit();

    for (auto scope: local_array_vardecl_list_)
    {
      for (auto vardecl: scope)
      {
        if (skip_const_vardecl && VarDeclIsConst(vardecl)) 
          continue;

        if (skip_float_vardecl && VarDeclIsFloating(vardecl))
          continue;

        if (skip_register_vardecl && 
            vardecl->getStorageClass() == SC_Register)
        {
          continue;
        }

        string mutated_token{getVarDeclName(vardecl)};

        if (reference_name.compare(mutated_token) != 0 && 
            sameArrayElementType(type, vardecl->getType()))
        {
          GenerateMutant_new(mutant_name, start_loc, end_loc, 
                            reference_name, mutated_token);

          // Apply limit option on number of generated mutants 
          --mutant_num_limit;
          if (mutant_num_limit == 0)
            return;
        }
      }
    }
  }

  void GenerateVgtrMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string reference_name, QualType type, 
                          string mutant_name)
  {
    // cannot mutate variable in switch condition to a floating-type variable
    bool skip_float_vardecl = LocationIsInRange(*start_loc, 
                                                *switchstmt_condition_range_);

    // cannot mutate a variable in lhs of assignment to a const variable
    bool skip_const_vardecl = LocationIsInRange(*start_loc, 
                                                *lhs_of_assignment_range_);

    string struct_type = getStructureType(type);

    int mutant_num_limit = userinput_->getLimit();

    for (auto structure: global_struct_vardecl_list_)
    {
      if (skip_const_vardecl && VarDeclIsConst(structure)) 
        continue;   

      if (skip_float_vardecl && VarDeclIsFloating(structure))
        continue;

      string mutated_token{getVarDeclName(structure)};

      if (reference_name.compare(mutated_token) != 0 &&
        struct_type.compare(getStructureType(structure->getType())) == 0)
      {
        GenerateMutant_new(mutant_name, start_loc, end_loc, 
                            reference_name, mutated_token);

        // Apply limit option on number of generated mutants 
        --mutant_num_limit;
        if (mutant_num_limit == 0)
          break;
      }
    }
  }

  void GenerateVltrMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string reference_name, QualType type, 
                          string mutant_name)
  {
    // cannot mutate variable in switch condition to a floating-type variable
    bool skip_float_vardecl = LocationIsInRange(*start_loc, 
                                                *switchstmt_condition_range_);

    // cannot mutate a variable in lhs of assignment to a const variable
    bool skip_const_vardecl = LocationIsInRange(*start_loc, 
                                                *lhs_of_assignment_range_);

    // cannot mutate a variable inside address op (&) to a register variable
    bool skip_register_vardecl = LocationIsInRange(*start_loc, 
                                                    *addressop_range_);

    string struct_type = getStructureType(type);

    int mutant_num_limit = userinput_->getLimit();

    for (auto scope: local_struct_vardecl_list_)
    {
      for (auto vardecl: scope)
      {
        if (skip_const_vardecl && VarDeclIsConst(vardecl)) 
          continue;   

        if (skip_float_vardecl && VarDeclIsFloating(vardecl))
          continue;

        if (skip_register_vardecl && 
            vardecl->getStorageClass() == SC_Register)
        {
          cout << "cannot mutate to " << getVarDeclName(vardecl) << endl;
          continue;
        }

        string mutated_token{getVarDeclName(vardecl)};

        if (reference_name.compare(mutated_token) != 0 &&
            struct_type.compare(getStructureType(vardecl->getType())) == 0)
        {
          GenerateMutant_new(mutant_name, start_loc, end_loc, 
                              reference_name, mutated_token);

          // Apply limit option on number of generated mutants 
          --mutant_num_limit;
          if (mutant_num_limit == 0)
            return;
        }
      }
    }
  }

  void GenerateVgprMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string reference_name, QualType type, 
                          string mutant_name)
  {
    // cannot mutate variable in switch condition to a floating-type variable
    bool skip_float_vardecl = LocationIsInRange(*start_loc, 
                                                *switchstmt_condition_range_);

    // cannot mutate a variable in lhs of assignment to a const variable
    bool skip_const_vardecl = LocationIsInRange(*start_loc, 
                                                *lhs_of_assignment_range_);

    string pointee_type = getPointerType(type);

    int mutant_num_limit = userinput_->getLimit();

    for (auto vardecl: global_pointer_vardecl_list_)
    {
      if (skip_const_vardecl && VarDeclIsConst(vardecl)) 
        continue;   

      if (skip_float_vardecl && VarDeclIsFloating(vardecl))
        continue;

      string mutated_token{getVarDeclName(vardecl)};

      if (reference_name.compare(mutated_token) != 0 &&
          pointee_type.compare(getPointerType(vardecl->getType())) == 0)
      {
        GenerateMutant_new(mutant_name, start_loc, end_loc, 
                            reference_name, mutated_token);

        // Apply limit option on number of generated mutants 
        --mutant_num_limit;
        if (mutant_num_limit == 0)
          break;
      }
    }
  }

  void GenerateVlprMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string reference_name, QualType type, 
                          string mutant_name)
  {
    // cannot mutate variable in switch condition to a floating-type variable
    bool skip_float_vardecl = LocationIsInRange(*start_loc, 
                                                *switchstmt_condition_range_);

    // cannot mutate a variable in lhs of assignment to a const variable
    bool skip_const_vardecl = LocationIsInRange(*start_loc, 
                                                *lhs_of_assignment_range_);

    // cannot mutate a variable inside address op (&) to a register variable
    bool skip_register_vardecl = LocationIsInRange(*start_loc, 
                                                  *addressop_range_);

    string pointee_type = getPointerType(type);

    int mutant_num_limit = userinput_->getLimit();

    for (auto scope: local_pointer_vardecl_list_)
    {
      for (auto vardecl: scope)
      {
        if (skip_const_vardecl && VarDeclIsConst(vardecl)) 
          continue;   

        if (skip_float_vardecl && VarDeclIsFloating(vardecl))
          continue;

        if (skip_register_vardecl && 
            vardecl->getStorageClass() == SC_Register)
        {
          continue;
        }

        string mutated_token{getVarDeclName(vardecl)};

        if (reference_name.compare(mutated_token) != 0 &&
            pointee_type.compare(getPointerType(vardecl->getType())) == 0)
        {
          GenerateMutant_new(mutant_name, start_loc, end_loc, 
                              reference_name, mutated_token);

          // Apply limit option on number of generated mutants 
          --mutant_num_limit;
          if (mutant_num_limit == 0)
            return;
        }
      }
    }
  }

  void GenerateVgsfMutant(SourceLocation *start_loc, 
                          SourceLocation *end_loc, string token)
  {
    GenerateVgsrMutant(start_loc, end_loc, token, "VGSF");
  }

  void GenerateVlsfMutant(SourceLocation *start_loc, 
                          SourceLocation *end_loc, string token)
  {
    GenerateVlsrMutant(start_loc, end_loc, token, "VLSF");
  }

  void GenerateVgtfMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string token, QualType type)
  {
    GenerateVgtrMutant(start_loc, end_loc, token, type, "VGTF");
  }

  void GenerateVltfMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string token, QualType type)
  {
    GenerateVltrMutant(start_loc, end_loc, token, type, "VLTF");
  }

  void GenerateVgpfMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string token, QualType type)
  {
    GenerateVgprMutant(start_loc, end_loc, token, type, "VGPF");
  }

  void GenerateVlpfMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string token, QualType type)
  {
    GenerateVlprMutant(start_loc, end_loc, token, type, "VLPF");
  }

  void GenerateVscrMutant(MemberExpr *me, SourceLocation end_loc)
  {
    auto baseQualType = me->getBase()->getType().getCanonicalType();

    // structPointer->structMember
    // baseQualType now is pointer type. what we want is pointee type
    if (me->isArrow())  
    {
      auto pointer_type = cast<PointerType>(baseQualType.getTypePtr());
      baseQualType = pointer_type->getPointeeType().getCanonicalType();
    }

    // type of the member expression
    auto me_qualtype = me->getType().getCanonicalType();

    // name of the specific member
    string reference_name{me->getMemberDecl()->getNameAsString()};

    SourceLocation start_loc = me->getMemberLoc();
    string mutant_name{"VSCR"};

    int mutant_num_limit = userinput_->getLimit();

    if (auto rt = dyn_cast<RecordType>(baseQualType.getTypePtr()))
    {
      RecordDecl *rd = rt->getDecl()->getDefinition();

      for (auto field = rd->field_begin(); field != rd->field_end(); ++field)
      {
        string mutated_token{field->getNameAsString()};

        // AVOID replacing token with the same token
        if (reference_name.compare(mutated_token) == 0)
          continue;

        if (field->isAnonymousStructOrUnion())
          continue;

        auto field_qualtype = field->getType().getCanonicalType();

        if (field_qualtype.getTypePtr()->isScalarType() && 
            !field_qualtype.getTypePtr()->isPointerType() &&
            ExprIsScalar(cast<Expr>(me)))
        {
          // This field & parameter me are both scalar type. 
          // Exact type does NOT matter.
          GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                              reference_name, mutated_token);

          // Apply limit option on number of generated mutants 
          --mutant_num_limit;
          if (mutant_num_limit == 0)
            break;
        }
        else if (field_qualtype.getTypePtr()->isPointerType() && 
                  ExprIsPointer(cast<Expr>(me)))
        {
          // This field & parameter me are both pointer type. 
          string me_pointee_type = getPointerType(me_qualtype);
          string field_pointee_type = getPointerType(field_qualtype);

          // Exact type does matter.
          if (me_pointee_type.compare(field_pointee_type) == 0)
          {
            GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                reference_name, mutated_token);

            // Apply limit option on number of generated mutants 
            --mutant_num_limit;
            if (mutant_num_limit == 0)
              break;
          }
        }
        else if (field_qualtype.getTypePtr()->isArrayType() && 
                  ExprIsArray(cast<Expr>(me)))
        {
          // This field & parameter me are both array type. 
          auto me_member_type = cast<ArrayType>(me_qualtype.getTypePtr())->getElementType();
          string me_member_type_str = me_member_type.getCanonicalType().getAsString();

          auto field_member_type = cast<ArrayType>(field_qualtype.getTypePtr())->getElementType();
          string field_member_type_str = field_member_type.getCanonicalType().getAsString();

          // Exact type does matter.
          if (me_member_type_str.compare(field_member_type_str) == 0)
          {
            GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                reference_name, mutated_token);

            // Apply limit option on number of generated mutants 
            --mutant_num_limit;
            if (mutant_num_limit == 0)
              break;
          }
        }
        else if (field_qualtype.getTypePtr()->isStructureType() && 
                  ExprIsStruct(cast<Expr>(me)))
        {
          // This field & parameter me are both structure type. 
          string me_struct_type = me_qualtype.getAsString();
          string field_struct_type = field_qualtype.getAsString();

          // Exact type does matter.
          if (me_struct_type.compare(field_struct_type) == 0)
          {
            GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                reference_name, mutated_token);

            // Apply limit option on number of generated mutants 
            --mutant_num_limit;
            if (mutant_num_limit == 0)
              break;
          }
        }
      }      
    } 
    else
    {
      cout << "GenerateVscrMutant: cannot convert to record type at "; 
      PrintLocation(start_loc);
      exit(1);
    }  
  }

  void GenerateOppoMutant(UnaryOperator *uo)
  {
    if (!(mutantoperator_holder_->apply_OPPO_))
      return;

    SourceLocation start_loc = uo->getLocStart();
    SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo);

    string mutant_name{"OPPO"};
    string token = rewriter_.ConvertToString(uo);

    if (uo->getOpcode() == UO_PostInc)  // x++
    {   
      // generate ++x
      uo->setOpcode(UO_PreInc);
      string mutated_token = rewriter_.ConvertToString(uo);

      GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                        token, mutated_token);

      // generate x--
      uo->setOpcode(UO_PostDec);
      mutated_token = rewriter_.ConvertToString(uo);

      if (userinput_->getLimit() > 1)
        GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                            token, mutated_token);

      // reset the code structure
      uo->setOpcode(UO_PostInc);
    }
    else  // ++x
    {
      // generate x++
      uo->setOpcode(UO_PostInc);
      string mutated_token = rewriter_.ConvertToString(uo);
      GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                          token, mutated_token);

      // generate --x
      uo->setOpcode(UO_PreDec);
      mutated_token = rewriter_.ConvertToString(uo);

      if (userinput_->getLimit() > 1)
        GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                          token, mutated_token);

      // reset the code structure
      uo->setOpcode(UO_PreInc);
    }
  }

  void GenerateOmmoMutant(UnaryOperator *uo)
  {    
    if (!(mutantoperator_holder_->apply_OMMO_))
      return;

    SourceLocation start_loc = uo->getLocStart();
    SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo);

    string mutant_name{"OMMO"};
    string token = rewriter_.ConvertToString(uo);

    if (uo->getOpcode() == UO_PostDec)  // x--
    {   
      // generate --x
      uo->setOpcode(UO_PreDec);
      string mutated_token = rewriter_.ConvertToString(uo);

      GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                          token, mutated_token);

      // generate x++
      uo->setOpcode(UO_PostInc);
      mutated_token = rewriter_.ConvertToString(uo);
      if (userinput_->getLimit() > 1)
        GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                            token, mutated_token);

      // reset the code structure
      uo->setOpcode(UO_PostDec);
    }
    else  // --x
    {
      // generate x--
      uo->setOpcode(UO_PostDec);
      string mutated_token = rewriter_.ConvertToString(uo);
      GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                          token, mutated_token);

      // generate --x
      uo->setOpcode(UO_PreDec);
      mutated_token = rewriter_.ConvertToString(uo);

      if (userinput_->getLimit() > 1)
        GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                            token, mutated_token);

      // reset the code structure
      uo->setOpcode(UO_PreDec);
    }
  }

  void GenerateVtwdMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string token, string mutant_name)
  {
    string mutated_token = "(" + token + "+1)";
    GenerateMutant_new(mutant_name, start_loc, end_loc, token, mutated_token);

    if (userinput_->getLimit() > 1)
    {
      mutated_token = "(" + token + "-1)";
      GenerateMutant_new(mutant_name, start_loc, end_loc, token, mutated_token);
    }
  }

  void GenerateVtwfMutant(SourceLocation *start_loc, SourceLocation *end_loc, 
                          string token)
  {
    GenerateVtwdMutant(start_loc, end_loc, token, "VTWF");
  }

  void GenerateOipmMutant(UnaryOperator *uo)
  {
    string mutant_name{"OIPM"};
    string token{rewriter_.ConvertToString(uo)};

    Expr *first_non_deref_subexpr = cast<Expr>(uo);
    // int numberOfDeref{0};

    while (true)
    {
      UnaryOperator *subexpr;

      if (subexpr = dyn_cast<UnaryOperator>(first_non_deref_subexpr))
        if (subexpr->getOpcode() == UO_Deref)
        {
          first_non_deref_subexpr = subexpr->getSubExpr()->IgnoreImpCasts();
          // ++numberOfDeref;
          continue;
        }

      break;
    }

    bool subexpr_is_array_subscript{false};
    bool subexpr_is_pointer{false};
    SourceLocation start_of_array_index;
    // QualType type;

    if (ArraySubscriptExpr *ase = dyn_cast<ArraySubscriptExpr>(first_non_deref_subexpr))
    {
      // The given expression has this form *..*arr[idx] or *..*arr[idx]
      subexpr_is_array_subscript = true;

      start_of_array_index = GetLeftBracketOfArraySubscript(ase);
      // type = ase->getBase()->IgnoreImpCasts()->getType().getCanonicalType();
    }

    if (UnaryOperator *uop = dyn_cast<UnaryOperator>(first_non_deref_subexpr))
      if (uop->getOpcode() == UO_PostDec || uop->getOpcode() == UO_PostInc)
      {
        // The given expression has this form *..*ptr++ or *..*ptr--
        subexpr_is_pointer = true;

        // start_of_array_index = uop->getLocEnd();
        // type = uop->getSubExpr()->IgnoreImpCasts()->getType().getCanonicalType();
      }

    if (subexpr_is_pointer)
    {
      SourceLocation start_loc = uo->getLocStart();
      SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo);

      // If the expression is on left hand side of assignment
      // cannot apply OIPM to the first Deref operator -> uncompilable
      if (TargetRangeIsInMutationRange(&start_loc, &end_loc)
        && !uo->getType().getCanonicalType().isConstQualified()
        && start_loc != lhs_of_assignment_range_->getBegin())
      {
        // retrieve the string form of given expression without ++/--
        string mutated_token{rewriter_.ConvertToString(uo)};
        mutated_token.pop_back();
        mutated_token.pop_back();

        string mutated_token1;
        if (uo->getOpcode() == UO_PostDec)
          mutated_token1 = "--(" + mutated_token + ")";
        else
          mutated_token1 = "++(" + mutated_token + ")";

        GenerateMutant_new(mutant_name, &start_loc, &end_loc, token, mutated_token1);

        string mutated_token2;
        if (uo->getOpcode() == UO_PostDec)
          mutated_token2 = "(" + mutated_token + ")--";
        else
          mutated_token2 = "(" + mutated_token + ")++";

        GenerateMutant_new(mutant_name, &start_loc, &end_loc, token, mutated_token2);
      }
    }

    if (subexpr_is_array_subscript)
    {
      SourceLocation start_loc = uo->getLocStart();
      SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo);

      if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
      {
        string array_index;

        while (start_of_array_index != end_loc)
        {
          array_index += *(src_mgr_.getCharacterData(start_of_array_index));
          start_of_array_index = start_of_array_index.getLocWithOffset(1);
        }

        string mutated_token = "(" + token.substr(0, token.length() - \
                                array_index.length()) + ")" + array_index;

        GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                            token, mutated_token);
      }
    }
  }

  void GenerateMutantByNegation(Expr *e, string mutant_name)
  {


    SourceLocation start_loc = e->getLocStart();
    SourceLocation end_loc = GetEndLocOfExpr(e); 
    string token{rewriter_.ConvertToString(e)};    

    string mutated_token = "!(" + token + ")";

    if (mutant_name.compare("OBNG") == 0)
      mutated_token = "~(" + token + ")";

    GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                        token, mutated_token);
  }

  /*SourceLocation GetEndLocOfExpr(Expr *e)
  {
    if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
      return GetEndLocOfUnaryOpExpr(uo);

    if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
      return GetEndLocOfExpr(bo->getRHS()->IgnoreImpCasts());

    return GetEndLocOfExpr(e);
  }*/

  // Called to check if lhs of an additive expression is a pointer
  bool LhsOfExprIsPointer(Expr *e)
  {
    bool ret = e->IgnoreImpCasts()->getType().getTypePtr()->isPointerType();

    if (isa<ParenExpr>(e))
      return ret;
    
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(e))
    {
      string mutant_name = b->getOpcodeStr();
      // if lhs is a multiplicative expression it cannot be a pointer
      if (mutant_name.compare("*") == 0 || 
          mutant_name.compare("/") == 0 || 
          mutant_name.compare("%") == 0)
        return ret;
      else
        return LhsOfExprIsPointer(b->getRHS());
    }
    else
      return ret;
  }

  BinaryOperator::Opcode TranslateToOpcode(string binary_operator)
  {
    if (binary_operator.compare("+") == 0)
      return BO_Add;
    if (binary_operator.compare("-") == 0)
      return BO_Sub;
    if (binary_operator.compare("*") == 0)
      return BO_Mul;
    if (binary_operator.compare("/") == 0)
      return BO_Div;
    if (binary_operator.compare("%") == 0)
      return BO_Rem;
    if (binary_operator.compare("<<") == 0)
      return BO_Shl;
    if (binary_operator.compare(">>") == 0)
      return BO_Shr;
    if (binary_operator.compare("|") == 0)
      return BO_Or;
    if (binary_operator.compare("&") == 0)
      return BO_And;
    if (binary_operator.compare("^") == 0)
      return BO_Xor;
    if (binary_operator.compare("<") == 0)
      return BO_LT;
    if (binary_operator.compare(">") == 0)
      return BO_GT;
    if (binary_operator.compare("<=") == 0)
      return BO_LE;
    if (binary_operator.compare(">=") == 0)
      return BO_GE;
    if (binary_operator.compare("==") == 0)
      return BO_EQ;
    if (binary_operator.compare("!=") == 0)
      return BO_NE;
    if (binary_operator.compare("&&") == 0)
      return BO_LAnd;
    if (binary_operator.compare("||") == 0)
      return BO_LOr;
    cout << "cannot translate " << binary_operator << " to opcode\n";
    exit(1);
  }

  bool MutationCauseExprValueToBeZero(BinaryOperator *b, 
                                      string mutated_operator)
  {
    BinaryOperator::Opcode original_operator = b->getOpcode();
    b->setOpcode(TranslateToOpcode(mutated_operator));
    llvm::APSInt val;

    if (b->EvaluateAsInt(val, comp_inst_->getASTContext()))
    {
      // check if the value is zero.
      // reset the opcode and return true/false depending on the check
      b->setOpcode(original_operator);
      string zero = "0";

      if ((val.toString(10)).compare("0") == 0)
        return true;
      return false;
    }
    else
    {
      // cannot evaluate as integer
      // reset the opcode and return false
      b->setOpcode(original_operator);
      return false;
    }
  }

  bool MutationCauseDivideByZeroError(Expr *rhs)
  {
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
    {
      string binary_operator = b->getOpcodeStr();
      return MutationCauseExprValueToBeZero(b, binary_operator);
    }
    else  // not binary operator
    {
      llvm::APSInt val;
      if (rhs->EvaluateAsInt(val, comp_inst_->getASTContext()))
      {
        // check if the value is zero.
        // reset the opcode and return true/false depending on the check
        string zero = "0";
        if ((val.toString(10)).compare("0") == 0)
          return true;
        return false;
      }
      else
      {
        // cannot evaluate as integer
        return false;
      }
    }
  }

  void PrintLocation(SourceLocation loc)
  {
    cout << getLineNumber(&loc) << "\t" << getColNumber(&loc) << endl;
  }

  void PrintRange(SourceRange range)
  {
    cout << "this range is from ";
    PrintLocation(range.getBegin());
    cout << "        to ";
    PrintLocation(range.getEnd());
  }

  void PrintExprType(Expr *e)
  {
    cout << "expr type: " << e->getType().getCanonicalType().getAsString() << endl;
  }

  void HandleStmtExpr(StmtExpr *se)
  {
    CompoundStmt::body_iterator stmt = (se->getSubStmt())->body_begin();
    CompoundStmt::body_iterator nextStmt = stmt;
    ++nextStmt;

    // retrieve the last statement inside Statement Expression
    while (nextStmt != (se->getSubStmt())->body_end())
    {
      ++stmt;
      ++nextStmt;
    }

    SourceLocation start_loc = (*stmt)->getLocStart();
    SourceLocation end_loc = (*stmt)->getLocEnd();


    if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
    {
      // PROBLEM: everything else in this function might not be necessary
      is_inside_stmtexpr_ = true;
    }
  }

  void HandleArraySubscriptExpr(ArraySubscriptExpr *ase)
  {
    SourceLocation start_loc = ase->getLocStart();
    SourceLocation end_loc = ase->getLocEnd();
    end_loc = GetEndLocOfStmt(&end_loc);

    string reference_name{rewriter_.ConvertToString(ase)};

    if (arraysubscript_range_ != nullptr)
      delete arraysubscript_range_;

    arraysubscript_range_ = new SourceRange(start_loc, end_loc);

    //===============================
    //=== GENERATING VTWD MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_VTWD_
      && ExprIsScalar(cast<Expr>(ase)))
    {
      if (TargetRangeIsInMutationRange(&start_loc, &end_loc)
        && !is_inside_enumdecl_ 
        && !LocationIsInRange(start_loc, *lhs_of_assignment_range_)
        && !LocationIsInRange(start_loc, *unary_increment_range_) 
        && !LocationIsInRange(start_loc, *unary_decrement_range_) 
        && !LocationIsInRange(start_loc, *addressop_range_))
      {
        if (ScalarRefCanBeMutatedByVtwd(reference_name))
          GenerateVtwdMutant(&start_loc, &end_loc, reference_name, "VTWD");
        else
          // Block VTWD mutation once and 
          // remove the reference name from the nonMutatable list.
          for (auto it = non_VTWD_mutatable_scalarref_list_.begin(); 
                it != non_VTWD_mutatable_scalarref_list_.end(); ++it)
            if (reference_name.compare(*it) == 0)
            {
              non_VTWD_mutatable_scalarref_list_.erase(it);
              break;
            }
      }
    }

    //===============================
    //=== GENERATING CRCR MUTANTS ===
    //===============================
    if ((mutantoperator_holder_->CRCR_floating_range_).size() != 0)
    {
      if (!is_inside_array_decl_size_
        && !is_inside_enumdecl_
        && TargetRangeIsInMutationRange(&start_loc, &end_loc)
        && !LocationIsInRange(start_loc, *lhs_of_assignment_range_)
        && !LocationIsInRange(start_loc, *unary_increment_range_)
        && !LocationIsInRange(start_loc, *unary_decrement_range_)
        && !LocationIsInRange(start_loc, *addressop_range_))
        GenerateCrcrMutant(cast<Expr>(ase), start_loc, end_loc);
    }

    //=================================================
    //=== GENERATING Vsrr, Varr, Vtrr, Vprr MUTANTS ===
    //=================================================
    // PROBLEM: most outer if is not necessary
    if (mutantoperator_holder_->apply_VGSR_ || 
        mutantoperator_holder_->apply_VLSR_ || 
        mutantoperator_holder_->apply_VGAR_ || 
        mutantoperator_holder_->apply_VLAR_ ||
        mutantoperator_holder_->apply_VGTR_ || 
        mutantoperator_holder_->apply_VLTR_ || 
        mutantoperator_holder_->apply_VGPR_ || 
        mutantoperator_holder_->apply_VLPR_)
    {
      if (!is_inside_array_decl_size_ && 
          !is_inside_enumdecl_ 
          && TargetRangeIsInMutationRange(&start_loc, &end_loc))
      {
        if (ExprIsScalar(cast<Expr>(ase)))
        {
          if (mutantoperator_holder_->apply_VGSR_)
            GenerateVgsrMutant(&start_loc, &end_loc, reference_name, "VGSR");

          if (mutantoperator_holder_->apply_VLSR_)
            GenerateVlsrMutant(&start_loc, &end_loc, reference_name, "VLSR");
        }
        else if (ExprIsArray(cast<Expr>(ase)))
        {
          if (mutantoperator_holder_->apply_VGAR_)
            GenerateVgarMutant(&start_loc, &end_loc, reference_name, 
                                ase->getType(), "VGAR");

          if (mutantoperator_holder_->apply_VLAR_)
            GenerateVlarMutant(&start_loc, &end_loc, reference_name, 
                                ase->getType(), "VLAR");
        }
        else if (ExprIsStruct(cast<Expr>(ase)))
        {
          if (mutantoperator_holder_->apply_VGTR_)
            GenerateVgtrMutant(&start_loc, &end_loc, reference_name, 
                                ase->getType(), "VGTR");

          if (mutantoperator_holder_->apply_VLTR_)
            GenerateVltrMutant(&start_loc, &end_loc, reference_name, 
                                ase->getType(), "VLTR");
        }
        else if (ExprIsPointer(cast<Expr>(ase)))
        {
          if (mutantoperator_holder_->apply_VGPR_)
            GenerateVgprMutant(&start_loc, &end_loc, reference_name, 
                                ase->getType(), "VGPR");

          if (mutantoperator_holder_->apply_VLPR_)
            GenerateVlprMutant(&start_loc, &end_loc, reference_name, 
                                ase->getType(), "VLPR");
        }
      }
    }
  }

  void HandleDeclRefExpr(DeclRefExpr *dre)
  {
    SourceLocation start_loc = dre->getLocStart();
    string reference_name{dre->getDecl()->getNameAsString()};
    SourceLocation end_loc = src_mgr_.translateLineCol(src_mgr_.getMainFileID(),
                    getLineNumber(&start_loc),
                    getColNumber(&start_loc) + reference_name.length());

    // Enum value used outside of enum declaration scope are 
    // considered as Declaration Reference Expression.
    // Do not mutate those.
    if (isa<EnumConstantDecl>(dre->getDecl()))
      return;

    //===============================
    //=== GENERATING VTWD MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_VTWD_
      && ExprIsScalar(cast<Expr>(dre)))
    {
      if (TargetRangeIsInMutationRange(&start_loc, &end_loc)
        && !is_inside_enumdecl_ 
        && !LocationIsInRange(start_loc, *lhs_of_assignment_range_)
        && !LocationIsInRange(start_loc, *unary_increment_range_) 
        && !LocationIsInRange(start_loc, *unary_decrement_range_) 
        && !LocationIsInRange(start_loc, *addressop_range_))
      {
        // During the process of collect non-mutable reference,
        // rewriter_.ConvertToString was used, so now, I use the
        // function just to prevent anything abnormal from happening.
        string declName{rewriter_.ConvertToString(dre)};

        if (ScalarRefCanBeMutatedByVtwd(declName))
          GenerateVtwdMutant(&start_loc, &end_loc, declName, "VTWD");
        else
          // Block VTWD mutation once and remove the reference name 
          // from the nonMutatable list.
          for (auto it = non_VTWD_mutatable_scalarref_list_.begin(); 
                it != non_VTWD_mutatable_scalarref_list_.end(); ++it)
            if (declName.compare(*it) == 0)
            {
              non_VTWD_mutatable_scalarref_list_.erase(it);
              break;
            }
      }
    }

    //===============================
    //=== GENERATING Ccsr MUTANTS ===
    //===============================
    if ((mutantoperator_holder_->apply_CGSR_ || 
          mutantoperator_holder_->apply_CLSR_) && 
          ExprIsScalar(cast<Expr>(dre)))
    {
      // no mutation applied inside array declaration, enum declaration,
      // outside mutation range, left hand of assignment, inside 
      // address-of operator and left hand of unary increment operator.
      if (!is_inside_array_decl_size_ && 
        !is_inside_enumdecl_ && 
        TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
        !LocationIsInRange(start_loc, *lhs_of_assignment_range_) && 
        !LocationIsInRange(start_loc, *unary_increment_range_) &&
        !LocationIsInRange(start_loc, *unary_decrement_range_) && 
        !LocationIsInRange(start_loc, *addressop_range_))
      {
        // cannot mutate the variable in switch condition or 
        // array subscript to a floating-type variable
        bool skip_float_literal = LocationIsInRange(start_loc, 
                                                    *arraysubscript_range_) ||
                                  LocationIsInRange(start_loc, 
                                                    *switchstmt_condition_range_);

        if (mutantoperator_holder_->apply_CGSR_)
        {
          string mutant_name{"CGSR"};
          int mutant_num_limit = userinput_->getLimit();

          for (auto e: *global_scalarconstant_list_)
          {
            if (skip_float_literal && e.second)
              continue;

            GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                reference_name, e.first);

            // Apply limit option on number of generated mutants 
            --mutant_num_limit;
            if (mutant_num_limit == 0)
              break;
          }
        }

        // CLSR can only be applied to references inside a function, 
        // not a global reference
        if (mutantoperator_holder_->apply_CLSR_ && 
            LocationIsInRange(start_loc, *currently_parsed_function_range_))
        {
          string mutant_name{"CLSR"};

          int mutant_num_limit = userinput_->getLimit();

          // generate mutants with constants inside the current function
          for (auto constant: *local_scalarconstant_list_)
          {
            // all the consts after this are outside the current function
            if (!LocationIsInRange(constant.second.first, 
                                    *currently_parsed_function_range_))
              break;

            if (skip_float_literal && constant.second.second)
              continue;

            GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                reference_name, constant.first);

            // Apply limit option on number of generated mutants 
            --mutant_num_limit;
            if (mutant_num_limit == 0)
              break;
          }
        }
      }
    }

    //===============================
    //=== GENERATING CRCR MUTANTS ===
    //===============================
    if ((mutantoperator_holder_->CRCR_floating_range_).size() != 0)
    {
      if (!is_inside_array_decl_size_ && 
        !is_inside_enumdecl_ && 
        TargetRangeIsInMutationRange(&start_loc, &end_loc))
      {
        if (!LocationIsInRange(start_loc, *lhs_of_assignment_range_) && 
          !LocationIsInRange(start_loc, *unary_increment_range_) && 
          !LocationIsInRange(start_loc, *unary_decrement_range_) && 
          !LocationIsInRange(start_loc, *addressop_range_))
          GenerateCrcrMutant(cast<Expr>(dre), start_loc, end_loc);
      }
    }

    //=================================================
    //=== GENERATING Vsrr, Varr, Vtrr, Vprr MUTANTS ===
    //=================================================
    if (mutantoperator_holder_->apply_VGSR_ || 
        mutantoperator_holder_->apply_VLSR_ || 
        mutantoperator_holder_->apply_VGAR_ || 
        mutantoperator_holder_->apply_VLAR_ || 
        mutantoperator_holder_->apply_VGTR_ || 
        mutantoperator_holder_->apply_VLTR_ || 
        mutantoperator_holder_->apply_VGPR_ || 
        mutantoperator_holder_->apply_VLPR_)
    {
      if (!is_inside_array_decl_size_ && 
          !is_inside_enumdecl_ && 
          TargetRangeIsInMutationRange(&start_loc, &end_loc))
      {
        //===============================
        //=== GENERATING Vsrr MUTANTS ===
        //===============================
        if (ExprIsScalar(cast<Expr>(dre)))
        {
          if (mutantoperator_holder_->apply_VGSR_)
            GenerateVgsrMutant(&start_loc, &end_loc, reference_name, "VGSR");

          if (mutantoperator_holder_->apply_VLSR_)
            GenerateVlsrMutant(&start_loc, &end_loc, reference_name, "VLSR");
        }
        //===============================
        //=== GENERATING Varr MUTANTS ===
        //===============================
        else if (ExprIsArray(cast<Expr>(dre)))
        {
          if (mutantoperator_holder_->apply_VGAR_)
            GenerateVgarMutant(&start_loc, &end_loc, reference_name, 
                                dre->getType(), "VGAR");

          if (mutantoperator_holder_->apply_VLAR_)
            GenerateVlarMutant(&start_loc, &end_loc, reference_name, 
                                dre->getType(), "VLAR");
        }
        //===============================
        //=== GENERATING Vtrr MUTANTS ===
        //===============================
        else if (ExprIsStruct(cast<Expr>(dre)))
        {
          if (mutantoperator_holder_->apply_VGTR_)
            GenerateVgtrMutant(&start_loc, &end_loc, reference_name, 
                                dre->getType(), "VGTR");

          if (mutantoperator_holder_->apply_VLTR_)
            GenerateVltrMutant(&start_loc, &end_loc, reference_name, 
                                dre->getType(), "VLTR");
        }
        //===============================
        //=== GENERATING Vprr MUTANTS ===
        //===============================
        else if (ExprIsPointer(cast<Expr>(dre)))
        {
          if (mutantoperator_holder_->apply_VGPR_)
            GenerateVgprMutant(&start_loc, &end_loc, reference_name, 
                                dre->getType(), "VGPR");

          if (mutantoperator_holder_->apply_VLPR_)
            GenerateVlprMutant(&start_loc, &end_loc, reference_name, 
                                dre->getType(), "VLPR");
        }
      }
    }
  }

  void HandleAbstractConditionalOperator(AbstractConditionalOperator *aco)
  {
    // handle (condition) ? (then) : (else)

    //===============================
    //=== GENERATING OCNG MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_OCNG_ && 
        aco->getCond() != nullptr)
    {
      // Negate the condition of if statement
      Expr *condition = aco->getCond()->IgnoreImpCasts();
      SourceLocation start_loc = condition->getLocStart();
      SourceLocation end_loc = condition->getLocEnd();
      string mutant_name{"OCNG"};

      if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
        GenerateMutantByNegation(condition, mutant_name);
    }
  }

  void HandleCStyleCastExpr(CStyleCastExpr *csce)
  {
    //===============================
    //=== GENERATING OCOR MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_OCOR_)
    {
      SourceLocation start_loc = csce->getLocStart();
      SourceLocation end_loc = GetEndLocOfExpr(cast<Expr>(csce));
      const Type *type{csce->getTypeAsWritten().getCanonicalType().getTypePtr()};

      if (TargetRangeIsInMutationRange(&start_loc, &end_loc)
        && !LocationIsInRange(start_loc, *fielddecl_range_)
        && (type->isIntegerType() || 
            type->isCharType() || 
            type->isFloatingType()))
      {
        vector<string> integral_type_list{"int", "unsigned", "short", "long", 
                                          "unsigned long", "char", 
                                          "unsigned char", "signed char"};
        vector<string> floating_type_list{"float", "double", "long double"};

        string type_str{csce->getTypeAsWritten().getCanonicalType().getAsString()};

        if (type_str.compare("unsigned int") == 0)
          type_str = "unsigned";

        start_loc = csce->getLParenLoc();
        end_loc = csce->getRParenLoc();
        end_loc = end_loc.getLocWithOffset(1);

        // retrieve token which is the type specified for conversion
        string token;
        SourceLocation walk = start_loc;

        while (walk != end_loc)
        {
          token += *(src_mgr_.getCharacterData(walk));
          walk = walk.getLocWithOffset(1);           
        }

        string mutant_name{"OCOR"};

        for (auto e: integral_type_list)
        {
          if (e.compare(type_str) != 0)
          {
            string mutated_token = "(" + e + ")";
            GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                token, mutated_token);
          }
        }

        // AVOID mutate to floating-type if sub-expression is pointer
        auto subexpr_type = csce->getSubExpr()->IgnoreImpCasts()->getType();
        subexpr_type = subexpr_type.getCanonicalType();
        bool subExprIsPointer{subexpr_type.getTypePtr()->isPointerType()};

        // OCOR_mutates_to_int_type_only is True when an assignment has the form "var = (int) ptr"
        // maybe redundant because subExprIsPointer covers this case
        if (OCOR_mutates_to_int_type_only)
          OCOR_mutates_to_int_type_only = false;
        else if (!LocationIsInRange(start_loc, *arraysubscript_range_) &&
                 !LocationIsInRange(start_loc, *switchcase_range_) &&
                 !LocationIsInRange(start_loc, *switchstmt_condition_range_) &&
                 !LocationIsInRange(start_loc, *non_OCOR_mutatable_expr_range_) &&
                 !subExprIsPointer)
        {
          for (auto e: floating_type_list)
          {
            if (e.compare(type_str) != 0)
            {
              string mutated_token = "(" + e + ")";
              GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                  token, mutated_token);
            }
          }
        }
      }
    }
  }

  void HandleUnaryOperator(UnaryOperator *uo)
  {
    SourceLocation start_loc = uo->getLocStart();
    SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo);

    if (uo->getOpcode() == UO_Deref)
    {
      //===============================
      //=== GENERATING OIPM MUTANTS ===
      //===============================
      if (mutantoperator_holder_->apply_OIPM_ && !is_inside_enumdecl_)
      {
        GenerateOipmMutant(uo);
      }

      //===============================
      //=== GENERATING VTWD MUTANTS ===
      //===============================
      if (mutantoperator_holder_->apply_VTWD_ && 
          ExprIsScalar(cast<Expr>(uo)))
      {
        if (TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
            !is_inside_enumdecl_ &&
            !LocationIsInRange(start_loc, *lhs_of_assignment_range_) &&
            !LocationIsInRange(start_loc, *unary_increment_range_) &&
            !LocationIsInRange(start_loc, *unary_decrement_range_) &&
            !LocationIsInRange(start_loc, *addressop_range_))
        {
          string reference_name{rewriter_.ConvertToString(uo)};

          if (ScalarRefCanBeMutatedByVtwd(reference_name))
            GenerateVtwdMutant(&start_loc, &end_loc, reference_name, "VTWD");
          else
            // Block VTWD mutation once and 
            // remove the reference name from the nonMutatable list.
            for (auto it = non_VTWD_mutatable_scalarref_list_.begin(); 
                  it != non_VTWD_mutatable_scalarref_list_.end(); ++it)
              if (reference_name.compare(*it) == 0)
              {
                non_VTWD_mutatable_scalarref_list_.erase(it);
                break;
              }
        }
      }

      //===============================
      //=== GENERATING CRCR MUTANTS ===
      //===============================
      if ((mutantoperator_holder_->CRCR_floating_range_).size() != 0)
      {
        if (!is_inside_array_decl_size_ &&
            !is_inside_enumdecl_ &&
            TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
            !LocationIsInRange(start_loc, *lhs_of_assignment_range_) &&
            !LocationIsInRange(start_loc, *unary_increment_range_) &&
            !LocationIsInRange(start_loc, *unary_decrement_range_) &&
            !LocationIsInRange(start_loc, *addressop_range_))
          GenerateCrcrMutant(cast<Expr>(uo), start_loc, end_loc);
      }

      //=================================================
      //=== GENERATING Vsrr, Varr, Vtrr, Vprr MUTANTS ===
      //=================================================
      if (!is_inside_array_decl_size_ && !is_inside_enumdecl_ && 
          TargetRangeIsInMutationRange(&start_loc, &end_loc))
      {
        string reference_name{rewriter_.ConvertToString(uo)};

        //===============================
        //=== GENERATING Vsrr MUTANTS ===
        //===============================
        if (ExprIsScalar(cast<Expr>(uo)))
        {
          if (mutantoperator_holder_->apply_VGSR_)
            GenerateVgsrMutant(&start_loc, &end_loc, reference_name, "VGSR");

          if (mutantoperator_holder_->apply_VLSR_)
            GenerateVlsrMutant(&start_loc, &end_loc, reference_name, "VLSR");
        }
        //===============================
        //=== GENERATING Varr MUTANTS ===
        //===============================
        else if (ExprIsArray(cast<Expr>(uo)))
        {
          if (mutantoperator_holder_->apply_VGAR_)
            GenerateVgarMutant(&start_loc, &end_loc, reference_name, 
                                uo->getType(), "VGAR");

          if (mutantoperator_holder_->apply_VLAR_)
            GenerateVlarMutant(&start_loc, &end_loc, reference_name, 
                                uo->getType(), "VLAR");
        }
        //===============================
        //=== GENERATING Vtrr MUTANTS ===
        //===============================
        else if (ExprIsStruct(cast<Expr>(uo)))
        {
          if (mutantoperator_holder_->apply_VGTR_)
            GenerateVgtrMutant(&start_loc, &end_loc, reference_name, 
                                uo->getType(), "VGTR");

          if (mutantoperator_holder_->apply_VLTR_)
            GenerateVltrMutant(&start_loc, &end_loc, reference_name, 
                                uo->getType(), "VLTR");
        }
        //===============================
        //=== GENERATING Vprr MUTANTS ===
        //===============================
        else if (ExprIsPointer(cast<Expr>(uo)))
        {
          if (mutantoperator_holder_->apply_VGPR_)
            GenerateVgprMutant(&start_loc, &end_loc, reference_name, 
                                uo->getType(), "VGPR");

          if (mutantoperator_holder_->apply_VLPR_)
            GenerateVlprMutant(&start_loc, &end_loc, reference_name, 
                                uo->getType(), "VLPR");
        }
      }
    }

    // Retrieve the range of UnaryOperator getting address of scalar reference
    // to prevent getting-address-of-a-register error.
    if (uo->getOpcode() == UO_AddrOf)
      UpdateAddressOfRange(uo, &start_loc, &end_loc);
    else if (uo->getOpcode() == UO_PostInc || 
              uo->getOpcode() == UO_PreInc)
    {
      unary_increment_range_ = new SourceRange(start_loc, end_loc);

      //===============================
      //=== GENERATING OPPO MUTANTS ===
      //===============================
      if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
        GenerateOppoMutant(uo);
    }   
    else if (uo->getOpcode() == UO_PostDec || 
              uo->getOpcode() == UO_PreDec)
    {
      unary_decrement_range_ = new SourceRange(start_loc, end_loc);

      //===============================
      //=== GENERATING OMMO MUTANTS ===
      //===============================
      if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
        GenerateOmmoMutant(uo);
    }
  }

  void HandleScalarConstant(Expr *e)
  {
    SourceLocation start_loc = e->getLocStart();

    string token{rewriter_.ConvertToString(e)};
    SourceLocation end_loc = GetEndLocOfConstantLiteral(start_loc);

    //===============================
    //=== GENERATING Cccr MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_CGCR_ || 
        mutantoperator_holder_->apply_CLCR_)
    {
      // cannot mutate the variable in switch condition, case value, 
      // array subscript to a floating-type variable
      bool skip_float_literal = LocationIsInRange(start_loc, 
                                                  *arraysubscript_range_) ||
                                LocationIsInRange(start_loc, 
                                                  *switchstmt_condition_range_) ||
                                LocationIsInRange(start_loc, *switchcase_range_);

      if (!is_inside_array_decl_size_ && 
          !is_inside_enumdecl_ &&
          TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
          !LocationIsInRange(start_loc, *fielddecl_range_))
      {
        if (mutantoperator_holder_->apply_CGCR_)
        {
          string mutant_name{"CGCR"};

          int mutant_num_limit = userinput_->getLimit();

          for (auto element: *global_scalarconstant_list_)
          {
            if (skip_float_literal && element.second)
              continue;

            // Avoid mutating to the same scalar constant
            // If token is char, then convert it to iint string for comparison
            if (token.front() == '\'' && token.back() == '\'')
              token = ConvertCharStringToIntString(token);

            if (token.compare(element.first) == 0)
              continue;

            token = rewriter_.ConvertToString(e);

            // Mitigate mutation from causing duplicate-case-label error.
            // If this constant is in range of a case label
            // then check if the replacing token is same with any other label.
            bool duplicatedCase = false;

            if (LocationIsInRange(start_loc, *switchcase_range_))
            {   
              string temp = element.first;

              // Convert char value to int for convenient comparison
              if (temp.front() == '\'' && temp.back() == '\'')
                temp = ConvertCharStringToIntString(temp);

              for (auto case_value: switchstmt_info_list_.back().second)
                if (temp.compare(case_value) == 0)
                {
                  duplicatedCase = true;
                  break;
                }
            }

            if (!duplicatedCase)
            {
              GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                  token, element.first);

              // Apply limit option on number of generated mutants 
              --mutant_num_limit;
              if (mutant_num_limit == 0)
                break;            
            }
          }
        }

        // CLCR is only applied to constants inside a function
        if (mutantoperator_holder_->apply_CLCR_ && 
            LocationIsInRange(start_loc, *currently_parsed_function_range_))
        {
          string mutant_name{"CLCR"};

          int mutant_num_limit = userinput_->getLimit();

          // generate mutants with constants inside the current function
          for (auto element: *local_scalarconstant_list_)
          {
            if (LocationBeforeRangeStart(element.second.first, 
                                        *currently_parsed_function_range_))
            {
              // cout << "before range. NEXT.\n";
              continue;
            }

            // all the consts after this are outside this function
            if (!LocationIsInRange(element.second.first, 
                                    *currently_parsed_function_range_))
            {
              // cout << "out of the function\n";
              break;
            }

            // Prevent mutating anything inside an array subscript to floating type
            if (skip_float_literal && element.second.second)
            {
              // cout << "skip floating\n";
              continue;
            }

            // Avoid mutating to the same scalar constant
            // If token is char, then convert it to int string for comparison
            if (token.front() == '\'' && token.back() == '\'')
              token = ConvertCharStringToIntString(token);

            if (token.compare(element.first) == 0)
            {
              // cout << "exact same\n";
              continue;
            }

            token = rewriter_.ConvertToString(e);

            // Mitigate mutation from causing duplicate-case-label error.
            // If this constant is in range of a case label
            // then check if the replacing token is same with any other label.
            bool duplicatedCase = false;

            if (LocationIsInRange(start_loc, *switchcase_range_))
            {
              string temp = element.first;
              if (temp.front() == '\'' && temp.back() == '\'')
                temp = ConvertCharStringToIntString(temp);

               for (auto case_value: switchstmt_info_list_.back().second)
                if (temp.compare(case_value) == 0)
                {
                  duplicatedCase = true;
                  break;
                }
            }

            if (!duplicatedCase)
            {
              GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                  token, element.first);

              // Apply limit option on number of generated mutants 
              --mutant_num_limit;
              if (mutant_num_limit == 0)
                break;
            }
          }
        }
      }
    }
  }

  void HandleStringLiteral(StringLiteral *sl)
  {
    SourceLocation start_loc = sl->getLocStart();
    SourceLocation end_loc = GetEndLocOfStringLiteral(start_loc);
    string token{rewriter_.ConvertToString(sl)};

    //===============================
    //=== GENERATING SANL MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_SANL_)
    {
      if (!is_inside_enumdecl_ &&
          TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
          !LocationIsInRange(start_loc, *fielddecl_range_))
      {
        string mutant_name{"SANL"};

        string mutated_token{token};

        // remove the last double quote
        mutated_token.pop_back();

        mutated_token += "\\n\"";

        GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                            token, mutated_token);
      }
    }

    //===============================
    //=== GENERATING SRWS MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_SRWS_)
    {
      if (!is_inside_enumdecl_ &&
          TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
          !LocationIsInRange(start_loc, *fielddecl_range_))
      {
        string mutant_name{"SRWS"};

        int firstNonWhitespace = GetFirstNonWhitespaceIndex(token);
        int lastNonWhitespace = GetLastNonWhitespaceIndex(token);

        // Generate mutant only when there is some whitespace in front
        if (firstNonWhitespace != 1)
        {
          string mutated_token = "\"" + token.substr(firstNonWhitespace);
          GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                              token, mutated_token);
        }

        // Generate mutant only when there is whitespace in the back
        if (lastNonWhitespace < token.length()-2)
        {
          string mutated_token = token.substr(0, lastNonWhitespace+1) + "\"";
          GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                              token, mutated_token);

          // Generate the third mutant only when there are whitespaces
          // in both the front and the back of the string
          if (firstNonWhitespace != 1)
          {
            string mutated_token = "\"";
            int str_length = lastNonWhitespace - firstNonWhitespace + 1;

            mutated_token += token.substr(firstNonWhitespace, str_length);
            mutated_token += "\"";

            GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                token, mutated_token);
          }
        }
      }
    }

    //===============================
    //=== GENERATING SCSR MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_SCSR_)
    {
      if (!is_inside_enumdecl_ &&
          TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
          !LocationIsInRange(start_loc, *fielddecl_range_))
      {
        string mutant_name{"SCSR"};

        // use to prevent duplicate mutants from local/global string literals
        set<string> stringCache;

        // All string literals from global list are distinct 
        // (filtered from InformationGatherer).
        for (auto literal: *global_stringliteral_list_)
          if (literal.compare(token) != 0)
          {
            GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                token, literal);

            stringCache.insert(literal);
          }

        // mutate to local strings only if this token is inside a function
        if (LocationIsInRange(start_loc, *currently_parsed_function_range_))
        {
          for (auto literal: *local_stringliteral_list_)
          {
            // Do not mutate to any string literals outside current function
            if (LocationBeforeRangeStart(literal.second, 
                                        *currently_parsed_function_range_))
              continue;

            // A string literal outside current function range signals
            // all following string literals are also outside.
            if (!LocationIsInRange(literal.second, 
                                    *currently_parsed_function_range_))
              break;

            // mutate if the literal is not the same as the token
            // and prevent duplicate if the literal is already in the cache
            if (literal.first.compare(token) != 0 &&
                stringCache.find(literal.first) == stringCache.end())
            {
              stringCache.insert(literal.first);

              GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                  token, literal.first);
            }
          }
        }
      }
    }
  }

  void HandleFunctionCall(CallExpr *ce)
  {
    SourceLocation start_loc = ce->getLocStart();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    string token{rewriter_.ConvertToString(ce)};

    // cout << rewriter_.ConvertToString(ce) << endl;
    // cout << ce->getCallReturnType().getCanonicalType().getAsString() << endl;

    if (TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
        !is_inside_enumdecl_)
    {
      if (ExprIsScalar(cast<Expr>(ce)))
      {
        if (mutantoperator_holder_->apply_VGSF_)
          GenerateVgsfMutant(&start_loc, &end_loc, token);

        if (mutantoperator_holder_->apply_VLSF_)
          GenerateVlsfMutant(&start_loc, &end_loc, token);

        // different from declaration reference, function call
        // cannot be lhs of assignment (foo() = ...), operand of
        // unary increment/decrement or address operator.
        if (mutantoperator_holder_->apply_VTWF_)
          GenerateVtwfMutant(&start_loc, &end_loc, token);
      }
      else if (ExprIsStruct(cast<Expr>(ce)))
      {
        if (mutantoperator_holder_->apply_VGTF_)
          GenerateVgtfMutant(&start_loc, &end_loc, token, 
                              ce->getType().getCanonicalType());

        if (mutantoperator_holder_->apply_VLTF_)
          GenerateVltfMutant(&start_loc, &end_loc, token, 
                              ce->getType().getCanonicalType());
      }
      else if (ExprIsPointer(cast<Expr>(ce)))
      {
        if (mutantoperator_holder_->apply_VGPF_)
          GenerateVgpfMutant(&start_loc, &end_loc, token, 
                              ce->getType().getCanonicalType());

        if (mutantoperator_holder_->apply_VLPF_)
          GenerateVlpfMutant(&start_loc, &end_loc, token, 
                              ce->getType().getCanonicalType());
      }
    }
  }

  int GetPrecedenceOfBinaryOperator(BinaryOperator::Opcode mutant_name)
  {
    switch (mutant_name)
    {
      case BO_Mul:
      case BO_Div:
      case BO_Rem:
        return 10;
      case BO_Add:
      case BO_Sub:
        return 9;
      case BO_Shl:
      case BO_Shr:
        return 8;
      case BO_LT:
      case BO_GT:
      case BO_LE:
      case BO_GE:
        return 7;
      case BO_EQ:
      case BO_NE:
        return 6;
      case BO_And:
        return 5;
      case BO_Xor:
        return 4;
      case BO_Or:
        return 3;
      case BO_LAnd:
        return 2;
      case BO_LOr:
        return 1;
      default:
        cout << "Unknown opcode\n";
        exit(1);
    }
    return 0;
  }

  // CURRENTLY UNUSED
  /*BinaryOperator* getRightmostBinaryOperator(BinaryOperator *b)
  {
    Expr *rhs = b->getRHS()->IgnoreImpCasts();
    if (BinaryOperator *e = dyn_cast<BinaryOperator>(rhs))
      return getRightmostBinaryOperator(e);
    else
      return b;
  }

  // CURRENTLY UNUSED
  // Return True if this operator has higher precedence than the previous operator on its left.
  // This means mutation can cause a change in calculation order => potential uncompilable mutants.
  bool higherPrecedenceThanLeftOperator(Expr *lhs, string mutant_name)
  {
    if (!isa<BinaryOperator>(lhs))  // there is no operator on the left
      return false;

    int opPrecedence(0);
    if (mutant_name.compare(">>") == 0)
      opPrecedence = GetPrecedenceOfBinaryOperator(BO_Shr);
    else if (mutant_name.compare("<<") == 0)
      opPrecedence = GetPrecedenceOfBinaryOperator(BO_Shl);
    // cout << "mutant_name precedence is: " << opPrecedence << endl;

    BinaryOperator *e{nullptr};
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
      e = getRightmostBinaryOperator(b);
    else
    {
      cout << "Error: higherPrecedenceThanLeftOperator: cannot convert to BinaryOperator\n";
      cout << rewriter_.ConvertToString(lhs) << endl;
      exit(1);
    }

    cout << "left mutant_name precedence: " << GetPrecedenceOfBinaryOperator(e->getOpcode()) << endl;

    return opPrecedence > GetPrecedenceOfBinaryOperator(e->getOpcode());
  }

  // CURRENTLY UNUSED
  BinaryOperator* getLeftmostBinaryOperator(BinaryOperator *b)
  {
    Expr *lhs = b->getLHS()->IgnoreImpCasts();
    if (BinaryOperator *e = dyn_cast<BinaryOperator>(lhs))
      return getLeftmostBinaryOperator(e);
    else
      return b;
  }

  // CURRENTLY UNUSED
  // Return True if this operator has higher precedence than the next operator on its right.
  // This means mutation can cause a change in calculation order => potential uncompilable mutants.
  bool higherPrecedenceThanRightOperator(Expr *rhs, string mutant_name)
  {

    if (!isa<BinaryOperator>(rhs))
      return false;

    int opPrecedence(0);
    if (mutant_name.compare(">>") == 0)
      opPrecedence = GetPrecedenceOfBinaryOperator(BO_Shr);
    else if (mutant_name.compare("<<") == 0)
      opPrecedence = GetPrecedenceOfBinaryOperator(BO_Shl);

    BinaryOperator *e{nullptr};
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
      e = getLeftmostBinaryOperator(b);
    else
    {
      cout << "Error: higherPrecedenceThanRightOperator: cannot convert to BinaryOperator\n";
      cout << rewriter_.ConvertToString(rhs) << endl;
      exit(1);
    }

    return opPrecedence > GetPrecedenceOfBinaryOperator(e->getOpcode());
  }

  // CURRENTLY UNUSED
  bool leftLongestArithmeticExprYieldPtr(Expr *lhs, int opPrecedence)
  {
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
    {
      if (opPrecedence > GetPrecedenceOfBinaryOperator(b->getOpcode()))
        return leftLongestArithmeticExprYieldPtr(b->getRHS()->IgnoreImpCasts(), opPrecedence);
      else
        return (((b->getRHS()->IgnoreImpCasts())->getType()).getTypePtr())->isPointerType() ||
            leftLongestArithmeticExprYieldPtr(b->getLHS()->IgnoreImpCasts(), opPrecedence);
    }
    else
      return ((lhs->getType()).getTypePtr())->isPointerType();
  }*/

  // Mutating an operator can make its lhs target change.
  // Return True if the new lhs of the mutated operator is pointer type.
  // Applicable for operator with precedence lower than +/-
  // Refer to function GetPrecedenceOfBinaryOperator() for precedence order
  bool LeftOperandAfterMutationIsPointer(
      Expr *lhs, BinaryOperator::Opcode mutant_name)
  {
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
    {
      BinaryOperator::Opcode opcodeOfB = b->getOpcode();

      // precedence of mutant_name is higher, analyze rhs of expr lhs
      if (opcodeOfB >= mutant_name) 
        return LeftOperandAfterMutationIsPointer(
            b->getRHS()->IgnoreParenImpCasts(), mutant_name);
      else if (opcodeOfB > BO_Sub || opcodeOfB < BO_Add)
        // result of not addittion/subtraction operator is not pointer
        return false;   
      else  
        // addition/substraction, if one of operands is pointer type 
        // then the expr is pointer type
        return LeftOperandAfterMutationIsPointer(
            b->getLHS()->IgnoreParenImpCasts(), BO_Assign) ||
               LeftOperandAfterMutationIsPointer(
                   b->getRHS()->IgnoreParenImpCasts(), BO_Assign);
    }
    else
      return ((lhs->getType()).getTypePtr())->isPointerType();
  }

  // Mutating an operator can make its rhs target change.
  // Return True if the new rhs of the mutated operator is pointer type.
  // Applicable for operator with precedence lower than +/-
  // Refer to function GetPrecedenceOfBinaryOperator() for precedence order
  bool RightOperandAfterMutationIsPointer(
      Expr *rhs, BinaryOperator::Opcode mutant_name)
  {
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
    {
      BinaryOperator::Opcode opcodeOfB = b->getOpcode();

      // precedence of mutant_name is higher, analyze lhs of expr rhs
      if (opcodeOfB >= mutant_name)  
        return RightOperandAfterMutationIsPointer(
            b->getLHS()->IgnoreParenImpCasts(), mutant_name);
      else if (opcodeOfB > BO_Sub || opcodeOfB < BO_Add)  
        // result of not addittion/subtraction operator is not pointer
        return false;
      else
        // addition/substraction, if one of operands is pointer type 
        // then the expr is pointer type
        return RightOperandAfterMutationIsPointer(
            b->getLHS()->IgnoreParenImpCasts(), BO_Assign) ||
               RightOperandAfterMutationIsPointer(
                   b->getRHS()->IgnoreParenImpCasts(), BO_Assign);
    }
    else
      return ((rhs->getType()).getTypePtr())->isPointerType();
  }

  // Mutating an operator can make its lhs target change.
  // Return the new lhs of the mutated operator.
  // Applicable for multiplicative operator
  Expr* GetLeftOperandAfterMutationToMultiplicativeOp(Expr *lhs)
  {
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
    {
      BinaryOperator::Opcode opcodeOfB = b->getOpcode();
      if (opcodeOfB > BO_Rem)
      {
        // multiplicative operator has the highest precedence.
        // go rhs right away if the operator is not multiplicative.
        return GetLeftOperandAfterMutationToMultiplicativeOp(
            b->getRHS()->IgnoreImpCasts());
      }
      else 
      {
        // * / % yield non-pointer only.
        return lhs;   
      }
    }
    else
      return lhs->IgnoreParens()->IgnoreImpCasts();
  }

  // Mutating an operator can make its rhs target change.
  // Return the new rhs of the mutated operator.
  // Applicable for multiplicative operator
  Expr* GetRightOperandAfterMutationToMultiplicativeOp(Expr *rhs)
  {
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
    {
      BinaryOperator::Opcode opcodeOfB = b->getOpcode();
      return GetRightOperandAfterMutationToMultiplicativeOp(
          b->getLHS()->IgnoreImpCasts());
    }
    else
      return rhs->IgnoreParens()->IgnoreImpCasts();
  }

  // Mutating an operator can make its lhs target change.
  // Return the new lhs of the mutated operator.
  // Applicable for additive operator
  Expr* GetLeftOperandAfterMutationToAdditiveOp(Expr *lhs)
  {
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
    {
      BinaryOperator::Opcode opcodeOfB = b->getOpcode();

      if (opcodeOfB < BO_Add) // multiplicative operator
        // they have higher precedence, calculated first
        return lhs; 
      else if (opcodeOfB > BO_Sub)
        // recursive call to rhs of lhs
        return GetLeftOperandAfterMutationToAdditiveOp(
            b->getRHS()->IgnoreImpCasts());  
      else  // addition or subtraction
        return lhs;
    }
    else
      return lhs->IgnoreParens()->IgnoreImpCasts();
  }

  // Mutating an operator can make its rhs target change.
  // Return the new rhs of the mutated operatore.
  // Applicable for additive operator
  Expr* GetRightOperandAfterMutationToAdditiveOp(Expr *rhs)
  {
    if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
    {
      BinaryOperator::Opcode opcodeOfB = b->getOpcode();
      if (opcodeOfB < BO_Add) // multiplicative operator
        // they have higher precedence, calculated first
        return rhs;
      else
        // if it is additive, calculate from left to right -> go left.
        // if it is lower precedence than additive,
        // then go left because additive is calculated first
        return GetRightOperandAfterMutationToAdditiveOp(
            b->getLHS()->IgnoreImpCasts());
    }
    else
      return rhs->IgnoreParens()->IgnoreImpCasts();
  }

  bool OperatorIsArithmeticAssignment(BinaryOperator *b)
  {
    return b->getOpcode() >= BO_MulAssign && 
           b->getOpcode() <= BO_SubAssign;
  }

  bool OperatorIsShiftAssignment(BinaryOperator *b)
  {
    return b->getOpcode() == BO_ShlAssign && 
           b->getOpcode() == BO_ShrAssign;
  }

  bool OperatorIsBitwiseAssignment(BinaryOperator *b)
  {
    return b->getOpcode() >= BO_AndAssign && 
           b->getOpcode() <= BO_OrAssign;
  }

  bool ExprIsDeclRefExpr(Expr *e)
  {
    // An Enum value is not a declaration reference
    if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(e))
    {
      if (isa<EnumConstantDecl>(dre->getDecl()))
        return false;
      return true;
    }
    return false;
  }

  bool ExprIsPointerDereferenceExpr(Expr *e)
  {
    if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
    {
      if (uo->getOpcode() == UO_Deref)
        return true;
      return false;
    }
    return false;
  }

  bool ExprIsScalarReference(Expr *e)
  {
    if (ExprIsDeclRefExpr(e) ||  ExprIsPointerDereferenceExpr(e) ||
        isa<ArraySubscriptExpr>(e) || isa<MemberExpr>(e))
      if (ExprIsScalar(e))
        return true;

    return false;
  }

  bool ExprIsPointer(Expr *e)
  {
    return e->getType().getCanonicalType().getTypePtr()->isPointerType();
  }

  bool ExprIsFloat(Expr *e)
  {
    return ((e->getType()).getCanonicalType().getTypePtr())->isFloatingType();
  }

  bool ExprIsIntegral(Expr *e)
  {
    auto type = ((e->getType()).getCanonicalType().getTypePtr());
    return type->isIntegralType(comp_inst_->getASTContext());
  }

  bool ExprIsScalar(Expr *e)
  {
    auto type = ((e->getType().getCanonicalType()).getTypePtr());
    return type->isScalarType() && !ExprIsPointer(e); 
  }

  bool ExprIsArray(Expr *e)
  {
    return ((e->getType().getCanonicalType()).getTypePtr())->isArrayType(); 
  }

  bool ExprIsStruct(Expr *e)
  {
    return e->getType().getCanonicalType().getTypePtr()->isStructureType(); 
  }

  bool VisitEnumDecl(EnumDecl *ed)
  {
    SourceLocation start_loc = ed->getLocStart();
    SourceLocation end_loc = ed->getLocEnd();

    is_inside_enumdecl_ = true;
    return true;
  }

  bool VisitTypedefDecl(TypedefDecl *td)
  {
    typedef_range_ = new SourceRange(td->getLocStart(), td->getLocEnd());
    return true;
  }

  bool VisitExpr(Expr *e)
  {
    // Do not mutate or consider anything inside a typedef definition
    if (LocationIsInRange(e->getLocStart(), *typedef_range_))
      return true;

    if (isa<CharacterLiteral>(e) || 
        isa<FloatingLiteral>(e) || 
        isa<IntegerLiteral>(e))
    {
      HandleScalarConstant(e);
    }
    else if (CallExpr *ce = dyn_cast<CallExpr>(e))
    {
      HandleFunctionCall(ce);
    }
    else if (StringLiteral *sl = dyn_cast<StringLiteral>(e))
    {
      HandleStringLiteral(sl);
    }
    else if (CStyleCastExpr *csce = dyn_cast<CStyleCastExpr>(e))
    {
      HandleCStyleCastExpr(csce);
    }
    else if (AbstractConditionalOperator *aco = dyn_cast<AbstractConditionalOperator>(e))
    {
      // condition ? then : else
      HandleAbstractConditionalOperator(aco);
    }
    else if (StmtExpr *se = dyn_cast<StmtExpr>(e))  
    {
      HandleStmtExpr(se);
    }
    else if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(e))
    {
      HandleDeclRefExpr(dre);
    }
    else if (ArraySubscriptExpr *ase = dyn_cast<ArraySubscriptExpr>(e))
    {
      HandleArraySubscriptExpr(ase);
    }
    else if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
    {
      HandleUnaryOperator(uo);
    }
    else if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e)) 
    {
      //===============================
      //=== GENERATING Obom MUTANTS ===
      //===============================

      // Retrieve the location and the operator of the expression
      string binary_operator = static_cast<string>(bo->getOpcodeStr());
      SourceLocation start_loc = bo->getOperatorLoc();
      SourceLocation end_loc = src_mgr_.translateLineCol(
          src_mgr_.getMainFileID(),
          getLineNumber(&start_loc),
          getColNumber(&start_loc) + binary_operator.length());

      // Certain mutations are NOT syntactically correct for left side of 
      // assignment. Store location for prevention of generating 
      // uncompilable mutants.
      if (bo->isAssignmentOp())
      {
        if (lhs_of_assignment_range_ != nullptr)
          delete lhs_of_assignment_range_;

        lhs_of_assignment_range_ = new SourceRange(
            bo->getLHS()->getLocStart(), start_loc);

        // Expr *lhs = IgnoreParenExpr(bo->getLHS()->IgnoreImpCasts());
        // cout << rewriter_.ConvertToString(lhs) << endl;
        // PrintLocation(start_loc);
      }

      // No mutation needs to be done if the token is not in the mutation range, 
      // or is inside array decl size or enum decl
      if (TargetRangeIsInMutationRange(&start_loc, &end_loc) && 
          !is_inside_array_decl_size_ && !is_inside_enumdecl_)
      {
        // cout << rewriter_.ConvertToString(e) << endl;
        // cout << binary_operator << endl;

        vector<MutantOperator>::iterator it_start;
        vector<MutantOperator>::iterator it_end;

        // Classify the type of operator to traverse the correct MutantOperator vector
        if (OperatorIsArithmetic(binary_operator)) 
        {
          it_start = mutantoperator_holder_->arithmetic_operator_mutators_.begin();
          it_end = mutantoperator_holder_->arithmetic_operator_mutators_.end();

          // Setting up for blocking uncompilable mutants for OCOR
          if (mutantoperator_holder_->apply_OCOR_ && 
            binary_operator.compare("%") == 0)
          {
            if (!LocationIsInRange(bo->getLocStart(), 
                                   *non_OCOR_mutatable_expr_range_))
            {
              if (non_OCOR_mutatable_expr_range_ != nullptr)
                delete non_OCOR_mutatable_expr_range_;

              non_OCOR_mutatable_expr_range_ = new SourceRange(
                  bo->getLocStart(), GetEndLocOfExpr(e));
            }
          }

          // mark an expression that should not be 0 to prevent divide-by-zero error.
          if (binary_operator.compare("/") == 0)
          {
            m_checkForZero = true;
            Expr *rhs = bo->getRHS()->IgnoreParenImpCasts();
            
            if (target_expr_range_ != nullptr)
              delete target_expr_range_;
            
            target_expr_range_ = new SourceRange(
                rhs->getLocStart(), rhs->getLocEnd());
          }

          // Setting up for prevent redundant VTWD mutants
          if (bo->isAdditiveOp() && mutantoperator_holder_->apply_VTWD_)
          {
            CollectNonVtwdMutatableScalarRef(e, true);
          }
        }
        else if (bo->isLogicalOp())
        {
          it_start = (mutantoperator_holder_->logical_operator_mutators_).begin();
          it_end = (mutantoperator_holder_->logical_operator_mutators_).end();

          //===============================
          //=== GENERATING OLNG MUTANTS ===
          //===============================
          if (mutantoperator_holder_->apply_OLNG_)
          {
            string olng{"OLNG"};

            GenerateMutantByNegation(e, olng);

            if (userinput_->getLimit() > 1)
              GenerateMutantByNegation(
                  bo->getRHS()->IgnoreImpCasts(), olng);

            if (userinput_->getLimit() > 2)
              GenerateMutantByNegation(
                  bo->getLHS()->IgnoreImpCasts(), olng);
          }
        }
        else if (bo->isRelationalOp() || bo->isEqualityOp())
        {
          it_start = mutantoperator_holder_->relational_operator_mutators_.begin();
          it_end = mutantoperator_holder_->relational_operator_mutators_.end();
        }
        else if (bo->isBitwiseOp())
        {
          // Setting up for blocking uncompilable mutants for OCOR
          if (mutantoperator_holder_->apply_OCOR_)
          {
            if (!LocationIsInRange(bo->getLocStart(), 
                                   *non_OCOR_mutatable_expr_range_))
            {
              if (non_OCOR_mutatable_expr_range_ != nullptr)
                delete non_OCOR_mutatable_expr_range_;

              non_OCOR_mutatable_expr_range_ = new SourceRange(
                  bo->getLocStart(), GetEndLocOfExpr(e));
            }
          }

          it_start = mutantoperator_holder_->bitwise_operator_mutators_.begin();
          it_end = mutantoperator_holder_->bitwise_operator_mutators_.end();

          //===============================
          //=== GENERATING OBNG MUTANTS ===
          //===============================
          if (mutantoperator_holder_->apply_OBNG_)
          {
            string obng{"OBNG"};

            GenerateMutantByNegation(e, obng);

            if (userinput_->getLimit() > 1)
              GenerateMutantByNegation(bo->getRHS()->IgnoreImpCasts(), 
                                       obng);

            if (userinput_->getLimit() > 2)
              GenerateMutantByNegation(bo->getLHS()->IgnoreImpCasts(), 
                                       obng);
          }
        }
        else if (bo->isShiftOp())
        {
          it_start = mutantoperator_holder_->shift_operator_mutators_.begin();
          it_end = (mutantoperator_holder_->shift_operator_mutators_).end();

          // Setting up for blocking uncompilable mutants for OCOR
          if (mutantoperator_holder_->apply_OCOR_)
          {
            if (!LocationIsInRange(bo->getLocStart(), 
                                   *non_OCOR_mutatable_expr_range_))
            {
              if (non_OCOR_mutatable_expr_range_ != nullptr)
                delete non_OCOR_mutatable_expr_range_;

              non_OCOR_mutatable_expr_range_ = new SourceRange(
                  bo->getLocStart(), GetEndLocOfExpr(e));
            }
          }
        }
        else if (OperatorIsArithmeticAssignment(bo))
        {
          it_start = (mutantoperator_holder_->arith_assign_operator_mutators_).begin();
          it_end = (mutantoperator_holder_->arith_assign_operator_mutators_).end();
        }
        else if (bo->isShiftAssignOp())
        {
          it_start = (mutantoperator_holder_->shift_assign_operator_mutators_).begin();
          it_end = (mutantoperator_holder_->shift_assign_operator_mutators_).end();
        }
        else if (OperatorIsBitwiseAssignment(bo))
        {
          it_start = (mutantoperator_holder_->bitwise_assign_operator_mutators_).begin();
          it_end = (mutantoperator_holder_->bitwise_assign_operator_mutators_).end();
        }
        else if (bo->getOpcode() == BO_Assign)
        {
          it_start = (mutantoperator_holder_->plain_assign_operator_mutators_).begin();
          it_end = (mutantoperator_holder_->plain_assign_operator_mutators_).end();

          if (CStyleCastExpr *csce = dyn_cast<CStyleCastExpr>(
              bo->getRHS()->IgnoreImpCasts()))
          {
            auto cast_type = csce->getTypeAsWritten().getCanonicalType();
            auto lhs_type = bo->getLHS()->IgnoreImpCasts()->getType();

            if (lhs_type.getCanonicalType().getTypePtr()->isPointerType()
              && (cast_type.getTypePtr()->isIntegerType() || 
                  cast_type.getTypePtr()->isCharType()))
            {
              OCOR_mutates_to_int_type_only = true;
            }
          }
        }
        else
        {
        }

        // Check whether left side, right side are pointers or integers
        // to prevent generating uncompliable mutants by OAAN
        Expr *left_expr = bo->getLHS()->IgnoreImpCasts();
        Expr *right_expr = bo->getRHS()->IgnoreImpCasts();

        bool rhs_is_pointer = right_expr->getType().getTypePtr()->isPointerType();
        bool rhs_is_int = right_expr->getType().getTypePtr()->isIntegralType(
            comp_inst_->getASTContext());

        bool lhs_is_int = left_expr->getType().getTypePtr()->isIntegralType(
            comp_inst_->getASTContext());
        bool lhs_is_pointer = left_expr->getType().getTypePtr()->isPointerType();
        bool lhs_is_struct = left_expr->getType().getTypePtr()->isStructureType();

        // traverse each mutant operator that can be applied for this binary_operator
        for (auto it = it_start; it != it_end; ++it)
        {
          bool only_add_or_subtract = false;
          bool only_subtract_assign = false;

          if ((it->getName()).compare("OAAN") == 0)
          {
            // only (int + ptr) and (ptr - ptr) are allowed.
            if (rhs_is_pointer)
              continue;

            // When changing from additive operator to other arithmetic 
            // operator, if the right operand is integral and the closest 
            // operand on the left is a pointer, then only mutate to 
            // either + or -.
            if (rhs_is_int && (binary_operator.compare("+") == 0 || 
                binary_operator.compare("-") == 0))
            {
              // PROBLEM: using the function here may not be necessary
              if (LhsOfExprIsPointer(bo->getLHS()->IgnoreImpCasts()))
                only_add_or_subtract = true;
            }
          }

          if ((it->getName()).compare("OAAA") == 0)
          {
            if (lhs_is_pointer)
              only_add_or_subtract = true;
          }

          // do not mutate to bitwise assignment or shift if
          // one of the operand is not integer type
          if ((it->getName()).compare("OABA") == 0 ||
              (it->getName()).compare("OASA") == 0)
            if (!lhs_is_int || !rhs_is_int)
              continue;

          if ((it->getName()).compare("OESA") == 0 ||
              (it->getName()).compare("OEBA") == 0)
            if (!lhs_is_int || !rhs_is_int)
              continue;

          if ((it->getName()).compare("OEAA") == 0)
          {
            if (lhs_is_struct)
              continue;

            if (lhs_is_pointer)
            {
              string rhs_type_str{
                  bo->getRHS()->IgnoreImpCasts()->getType().getAsString()};

              if (rhs_type_str.compare("void *") == 0)
                continue;
              else
                only_subtract_assign = true;
            }
          }

          // if the arithmetic expression is expected to be pointer-type
          // do not mutate it to logical or relational operators.
          if (((it->getName()).compare("OALN") == 0
            || (it->getName()).compare("OARN") == 0) 
            && ExprIsPointer(e))
            continue;

          // Generate mutants only for operators that can mutate this operator
          if ((it->domain_).find(binary_operator) != (it->domain_).end())
          {
            // Getting the max # of mutants per mutation location.
            int mutant_num_limit = userinput_->getLimit();

            for (auto mutated_token: it->range_)
            {
              if ((it->getName()).compare("OLSN") == 0 ||
                  (it->getName()).compare("OLBN") == 0 ||
                  (it->getName()).compare("ORBN") == 0 ||
                  (it->getName()).compare("ORSN") == 0 ||
                  (it->getName()).compare("OASN") == 0 ||
                  (it->getName()).compare("OABN") == 0)
              {
                // if new lhs or rhs is pointer
                // then COMUT should NOT mutate to bitwise or shift operator
                if (LeftOperandAfterMutationIsPointer(
                    bo->getLHS(), TranslateToOpcode(mutated_token)) ||
                    RightOperandAfterMutationIsPointer(
                        bo->getRHS(), TranslateToOpcode(mutated_token)))
                  continue;
              }

              // lhs and rhs of logical and relational operators vary
              // so when mutating to arithmetic operator, we need to consider
              // uncompilable cases related to pointer
              if ((it->getName()).compare("OLAN") == 0 ||
                (it->getName()).compare("ORAN") == 0)
              {
                Expr *new_left_expr = GetLeftOperandAfterMutationToAdditiveOp(
                    bo->getLHS()->IgnoreImpCasts());
                Expr *new_right_expr = GetRightOperandAfterMutationToAdditiveOp(
                    bo->getRHS()->IgnoreImpCasts());

                bool new_lhs_is_pointer = ExprIsPointer(new_left_expr);
                bool new_lhs_is_integral = ExprIsIntegral(new_left_expr);
                bool new_rhs_is_pointer = ExprIsPointer(new_right_expr);
                bool new_rhs_is_integral = ExprIsIntegral(new_right_expr);

                // cannot perform addition subtraction on non-pointer, non-integral
                // PROBLEM: forget floating
                if ((!new_lhs_is_pointer && !new_lhs_is_integral) ||
                    (!new_rhs_is_pointer && !new_rhs_is_integral))
                  continue;

                if (mutated_token.compare("+") == 0)
                {
                  // Adding 2 pointers is syntactically incorrect.
                  if (new_lhs_is_pointer && new_rhs_is_pointer)
                    continue;
                }
                else if (mutated_token.compare("-") == 0)
                {
                  // int-ptr and ptr-ptr are the only allowed syntax for -
                  if ((new_lhs_is_integral && new_rhs_is_pointer) ||
                      (new_lhs_is_pointer && new_rhs_is_pointer))
                    continue;
                }
                else  // multiplicative operator
                {
                  // multiplicative operators can only be applied to numbers
                  new_lhs_is_integral = ExprIsIntegral(
                      GetLeftOperandAfterMutationToMultiplicativeOp(
                          bo->getLHS()->IgnoreImpCasts()));
                  new_rhs_is_integral = ExprIsIntegral(
                      GetRightOperandAfterMutationToMultiplicativeOp(
                          bo->getRHS()->IgnoreImpCasts()));
                  
                  if (!new_lhs_is_integral || !new_rhs_is_integral)
                    continue;
                }
              }  

              // Do not replace the token with itself
              if (mutated_token.compare(binary_operator) == 0)
                continue;

              if (only_add_or_subtract)
              {
                if (mutated_token.compare("+") != 0 && 
                    mutated_token.compare("-") != 0 &&
                    mutated_token.compare("+=") != 0 && 
                    mutated_token.compare("-=") != 0)
                  continue;
              }

              if (only_subtract_assign && mutated_token.compare("-=") != 0)
                continue;

              // Prevent generation of umcompilable mutant because of divide-by-zero
              // divide-by-zero is actually a warning. further analysis needed
              if (m_checkForZero && 
                  *target_expr_range_ == SourceRange(
                      bo->getLocStart(), bo->getLocEnd()))
              {
                if (MutationCauseExprValueToBeZero(bo, mutated_token))
                {
                  continue;
                }
              }

              if (mutated_token.compare("/") == 0 &&
                MutationCauseDivideByZeroError(
                    bo->getRHS()->IgnoreParenImpCasts()))
                continue;

              if (first_binaryop_after_arraydecl_)
              {
                arraydecl_size_expr_ = bo;
                first_binaryop_after_arraydecl_ = false;
              }

              if (is_inside_array_decl_size_ && 
                  LocationIsInRange(start_loc, *array_decl_range_))
              {
                bo->setOpcode(TranslateToOpcode(mutated_token));
                llvm::APSInt val;

                if (arraydecl_size_expr_->EvaluateAsInt(
                    val, comp_inst_->getASTContext()) && val.isNegative())
                {
                  bo->setOpcode(TranslateToOpcode(binary_operator));
                  continue;
                }

                bo->setOpcode(TranslateToOpcode(binary_operator));
              }

              GenerateMutant(it->getName(), &start_loc, &end_loc, 
                             binary_operator, mutated_token);

              // make sure not to generate more mutants than user wants/specifies
              --mutant_num_limit;
              if (mutant_num_limit == 0)
              {
                break;
              }
            }
          }
        }

        if (m_checkForZero && 
            *target_expr_range_ == SourceRange(
                bo->getLocStart(), bo->getLocEnd()))
          m_checkForZero = false;
      }   
    }

    return true;
  }

  bool VisitIfStmt(IfStmt *is)
  {
    //===============================
    //=== GENERATING SSDL MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_SSDL_)
    {
      if (Stmt *then_stmt = is->getThen())
      {
        if (CompoundStmt *c = dyn_cast<CompoundStmt>(then_stmt))
        {          
          deleteCompoundContent(c);
        }
        // Apply SSDL for single non-compound stmt of THEN part of if stmt
        else if (!isa<NullStmt>(then_stmt))
        {
          string mutant_name{"SSDL"};
          string token{rewriter_.ConvertToString(then_stmt)};
          SourceLocation start_loc = then_stmt->getLocStart();
          SourceLocation end_loc = then_stmt->getLocEnd();

          // make replacing token
          string mutated_token{";"};
          mutated_token.append(
              getLineNumber(&end_loc) - getLineNumber(&start_loc), '\n');

          end_loc = GetEndLocOfStmt(&end_loc);

          if (end_loc.isInvalid())
          {
            cout << "error getting end_loc location. no SSDL mutants generated here.\n";
            cout << "error end_loc is: " << getLineNumber(&end_loc) << ":\n";
            cout << getColNumber(&end_loc) << endl;
            // exit(1);
          }
          else
          {
            end_loc = GetLocationAfterSemicolon(end_loc);

            if (TargetRangeIsInMutationRange(&start_loc, &end_loc) && 
                NoLabelStmtInsideRange(SourceRange(start_loc, end_loc)))
              GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                token, mutated_token);
          }
        }  
      }
      else 
      {
        // cout << "no then\n";
        ;
      }

      // generate SSDL mutant deleting the whole ELSE part of if statement
      if (Stmt *else_stmt = is->getElse())
      {
        if (!isa<NullStmt>(else_stmt))
        {
          string mutant_name{"SSDL"};
          string token{rewriter_.ConvertToString(else_stmt)};
          SourceLocation start_loc = is->getElseLoc();
          SourceLocation end_loc = else_stmt->getLocEnd();

          // make replacing token
          string mutated_token{";"};
          mutated_token.append(
              getLineNumber(&end_loc) - getLineNumber(&start_loc), '\n');

          end_loc = GetEndLocOfStmt(&end_loc);

          if (end_loc.isInvalid())
          {
            cout << "error getting end_loc location. no SSDL mutants generated here.\n";
            cout << "error end_loc is: " << getLineNumber(&end_loc) << ":\n";
            cout << getColNumber(&end_loc) << endl;
            // exit(1);
          }
          else 
          {
            if (TargetRangeIsInMutationRange(&start_loc, &end_loc) && 
                NoUnremovableLabelInsideRange(SourceRange(start_loc, end_loc)))
              GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                token, mutated_token);
          }            
        }
      }
      else
        // cout << "no else\n";
        ;
    }

    //===============================
    //=== GENERATING OCNG MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_OCNG_ && is->getCond() != nullptr)
    {
      // Negate the condition of if statement
      Expr *condition = is->getCond()->IgnoreImpCasts();
      SourceLocation start_loc = condition->getLocStart();
      SourceLocation end_loc = GetEndLocOfExpr(condition);
      string mutant_name{"OCNG"};

      if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
        GenerateMutantByNegation(condition, mutant_name);
    }

    return true;
  }

  bool VisitWhileStmt(WhileStmt *ws)
  {
    //===============================
    //=== GENERATING OCNG MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_OCNG_ && ws->getCond() != nullptr)
    {
      // Negate the condition of while statement
      Expr *condition = ws->getCond()->IgnoreImpCasts();
      SourceLocation start_loc = condition->getLocStart();
      SourceLocation end_loc = condition->getLocEnd();
      string mutant_name{"OCNG"};

      if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
        GenerateMutantByNegation(condition, mutant_name);
    }

    //===============================
    //=== GENERATING SSDL MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_SSDL_)
    {
      if (Stmt *body = ws->getBody())
      {
        if (CompoundStmt *c = dyn_cast<CompoundStmt>(body))
        {
          deleteCompoundContent(c);
        }
        else if (!isa<NullStmt>(body))
        {
          string mutant_name{"SSDL"};
          string token{rewriter_.ConvertToString(body)};
          SourceLocation start_loc = body->getLocStart();
          SourceLocation end_loc = body->getLocEnd();

          // make replacing token
          string mutated_token{";"};
          mutated_token.append(
              getLineNumber(&end_loc) - getLineNumber(&start_loc), '\n');

          end_loc = GetEndLocOfStmt(&end_loc);

          if (end_loc.isInvalid())
          {
            cout << "VisitWhileStmt: error getting end_loc location. no SSDL mutants generated here.\n";
            cout << "error end_loc is: "; 
            PrintLocation(end_loc);
            // exit(1);
          }
          else
          {
            end_loc = GetLocationAfterSemicolon(end_loc);

            if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
              GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                token, mutated_token);
          }
        }
      }
      else
        cout << "while stmt with no body\n";
    }

    return true;
  }

  bool VisitDoStmt(DoStmt *ds)
  {
    //===============================
    //=== GENERATING OCNG MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_OCNG_ && ds->getCond() != nullptr)
    {
      // Negate the condition of if statement
      Expr *condition = ds->getCond()->IgnoreImpCasts();
      SourceLocation start_loc = condition->getLocStart();
      SourceLocation end_loc = condition->getLocEnd();
      string mutant_name{"OCNG"};

      if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
        GenerateMutantByNegation(condition, mutant_name);
    }

    //===============================
    //=== GENERATING SSDL MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_SSDL_)
    {
      if (Stmt *body = ds->getBody())
      {
        if (CompoundStmt *c = dyn_cast<CompoundStmt>(body))
        {
          deleteCompoundContent(c);
        }
        else if (!isa<NullStmt>(body))
        {
          string mutant_name{"SSDL"};
          string token{rewriter_.ConvertToString(body)};
          SourceLocation start_loc = body->getLocStart();
          SourceLocation end_loc = body->getLocEnd();

          // make replacing token
          string mutated_token{";"};
          mutated_token.append(
              getLineNumber(&end_loc) - getLineNumber(&start_loc), '\n');

          end_loc = GetEndLocOfStmt(&end_loc);

          if (end_loc.isInvalid())
          {
            cout << "VisitDoStmt: error getting end_loc location. no SSDL mutants generated here.\n";
            cout << "error end_loc is: "; 
            PrintLocation(end_loc);
            // exit(1);
          }
          else
          {
            end_loc = GetLocationAfterSemicolon(end_loc);

            if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
              GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                token, mutated_token);
          }
        }
      }
      else
        cout << "do stmt with no body\n";
    }

    return true;
  }

  bool VisitForStmt(ForStmt *fs)
  {
    //===============================
    //=== GENERATING OCNG MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_OCNG_ && fs->getCond() != nullptr)
    {
      // Negate the condition of if statement
      Expr *condition = fs->getCond()->IgnoreImpCasts();
      SourceLocation start_loc = condition->getLocStart();
      SourceLocation end_loc = condition->getLocEnd();
      string mutant_name{"OCNG"};

      if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
        GenerateMutantByNegation(condition, mutant_name);
    }

    //===============================
    //=== GENERATING SSDL MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_SSDL_)
    {
      if (Stmt *body = fs->getBody())
      {
        if (CompoundStmt *c = dyn_cast<CompoundStmt>(body))
        {
          deleteCompoundContent(c);
        }
        else if (!isa<NullStmt>(body))
        {
          string mutant_name{"SSDL"};
          string token{rewriter_.ConvertToString(body)};
          SourceLocation start_loc = body->getLocStart();
          SourceLocation end_loc = body->getLocEnd();

          // make replacing token
          string mutated_token{";"};
          mutated_token.append(
              getLineNumber(&end_loc) - getLineNumber(&start_loc), '\n');

          end_loc = GetEndLocOfStmt(&end_loc);

          if (end_loc.isInvalid())
          {
            cout << "VisitForStmt: error getting end_loc location. no SSDL mutants generated here.\n";
            cout << "error end_loc is: "; 
            PrintLocation(end_loc);
            // exit(1);
          }
          else
          {
            end_loc = GetLocationAfterSemicolon(end_loc);

            if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
              GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                                  token, mutated_token);
          }
        }
      }
      else
        cout << "for stmt with no body\n";
    }

    return true;
  }

  void deleteStatement(Stmt *s)
  {
    // cannot delete declaration statement
    // deleting null statement causes equivalent mutant
    if (isa<DeclStmt>(s) || isa<NullStmt>(s))
      return; 

    // Do not delete the label, case, default but only the statement right below them.
    if (SwitchCase *sc = dyn_cast<SwitchCase>(s))
    {
      deleteStatement(sc->getSubStmt()); 
      return;
    }
    else 
      if (DefaultStmt *ds = dyn_cast<DefaultStmt>(s))
      {
        deleteStatement(ds->getSubStmt());
        return;
      }
      else
        if (LabelStmt *ls = dyn_cast<LabelStmt>(s))
        {
          deleteStatement(ls->getSubStmt());
          return;
        }
        else
          ;   // execute the rest of the function.


    string mutant_name = "SSDL";
    string token{rewriter_.ConvertToString(s)};
    SourceLocation start_loc = (s)->getLocStart();
    SourceLocation end_loc = (s)->getLocEnd();

    // make replacing token
    string mutated_token{";"};
    mutated_token.append(
        getLineNumber(&end_loc) - getLineNumber(&start_loc), '\n');

    end_loc = GetEndLocOfStmt(&end_loc);

    if (end_loc.isInvalid())
    {
      cout << "error end_loc is: " << getLineNumber(&end_loc) << ":\n";
      cout << getColNumber(&end_loc) << endl;
      cout << "error getting real end_loc location\n";
      // exit(1);
    }
    else
    {
      if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
      {
        // For stmts that can have label inside their body, make sure
        // there is no label that will be goto from outside before mutating
        if ((isa<DoStmt>(s) || isa<ForStmt>(s) || isa<WhileStmt>(s) || 
             isa<IfStmt>(s) || isa<SwitchStmt>(s)) &&
            !NoUnremovableLabelInsideRange(SourceRange(start_loc, end_loc)))
          return;
        
        GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                            token, mutated_token);
      }
    }      
  }

  void deleteCompoundContent(CompoundStmt *c)
  {  
    // No point deleting a CompoundStmt full of NullStmt
    // If there is only 1 non-NullStmt, then this mutant is equivalent to deleting that single statement.
    if (CountNonNullStmtInCompoundStmt(c) <= 1)
      return;

    SourceLocation start_loc = c->getLBracLoc();
    SourceLocation end_loc = c->getRBracLoc().getLocWithOffset(1);

    if (!NoUnremovableLabelInsideRange(SourceRange(start_loc, end_loc)))
      return;

    string mutant_name{"SSDL"};
    string token{rewriter_.ConvertToString(c)};
    
    
    // make replacing token
    string mutated_token{"{"};
    mutated_token.append(
        getLineNumber(&end_loc) - getLineNumber(&start_loc), '\n');
    mutated_token.append(1, '}');


    if (TargetRangeIsInMutationRange(&start_loc, &end_loc))
      GenerateMutant_new(mutant_name, &start_loc, &end_loc, 
                         token, mutated_token);
  }

  bool VisitCompoundStmt(CompoundStmt *c)
  {
    // entering a new scope
    scope_list_.push_back(SourceRange(c->getLocStart(), c->getLocEnd()));
    local_scalar_vardecl_list_.push_back(VarDeclList());
    local_array_vardecl_list_.push_back(VarDeclList());
    local_struct_vardecl_list_.push_back(VarDeclList());
    local_pointer_vardecl_list_.push_back(VarDeclList());

    if (!mutantoperator_holder_->apply_SSDL_)
      return true;

    // Generate SSDL mutants for each statement in the compound that is not
    // declaration, switch case, default case, label or null statement.
    for (CompoundStmt::body_iterator it = c->body_begin(); 
         it != c->body_end(); ++it)
    {
      // Do not apply SSDL to the last statement of statement expression
      if (is_inside_stmtexpr_)
      {
        CompoundStmt::body_iterator nextStmt = it;
        ++nextStmt;

        if (nextStmt == c->body_end())  
        {
          // this is the last statement of a statement expression
          is_inside_stmtexpr_ = false;

          // no SDL mutants generated for this statement
          continue;   
        }
      }

      deleteStatement(*it);
    }

    return true;
  }

  bool VisitFieldDecl(FieldDecl *fd)
  {
    SourceLocation start_loc = fd->getLocStart();
    SourceLocation end_loc = fd->getLocEnd();

    if (fielddecl_range_ != nullptr)
      delete fielddecl_range_;

    fielddecl_range_ = new SourceRange(start_loc, end_loc);

    if (fd->getType().getTypePtr()->isArrayType())
    {
      is_inside_array_decl_size_ = true;
      first_binaryop_after_arraydecl_ = true;

      array_decl_range_ = new SourceRange(fd->getLocStart(), fd->getLocEnd());
    }
    return true;
  }

  bool VisitVarDecl(VarDecl *vd)
  {
    SourceLocation start_loc = vd->getLocStart();
    SourceLocation end_loc = vd->getLocEnd();

    is_inside_enumdecl_ = false;

    if (LocationIsInRange(start_loc, *typedef_range_))
      return true;

    if (LocationIsInRange(start_loc, *functionprototype_range_))
      return true;

    if (VarDeclIsScalar(vd))
    {
      if (vd->isFileVarDecl())  
      {
        // store global scalar variable
        global_scalar_vardecl_list_.push_back(vd);
      }
      else
      {
        // This is a local variable. scope_list_ vector CANNOT be empty.
        if (local_scalar_vardecl_list_.empty())
        {
          cout << "local_scalar_vardecl_list_ is empty when meeting a local variable declaration at "; 
          PrintLocation(start_loc);
          exit(1);
        }
        local_scalar_vardecl_list_.back().push_back(vd);
      }
    }
    else if (VarDeclIsArray(vd))
    {
      auto type = vd->getType().getCanonicalType().getTypePtr();

      if (auto array_type =  dyn_cast_or_null<ConstantArrayType>(type)) 
      {  
        is_inside_array_decl_size_ = true;
        first_binaryop_after_arraydecl_ = true;

        array_decl_range_ = new SourceRange(start_loc, end_loc);
      }

      if (vd->isFileVarDecl())  // global variable
      {
        global_array_vardecl_list_.push_back(vd);
      }
      else
      {
        // This is a local variable. scope_list_ vector CANNOT be empty.
        if (local_array_vardecl_list_.empty())
        {
          cout << "local_array_vardecl_list_ is empty when meeting a local variable declaration at "; 
          PrintLocation(start_loc);
          exit(1);
        }

        local_array_vardecl_list_.back().push_back(vd);
      }
    }
    else if (VarDeclIsStruct(vd))
    {
      if (vd->isFileVarDecl())  // global variable
      {
        global_struct_vardecl_list_.push_back(vd);
      }
      else
      {
        // This is a local variable. scope_list_ vector CANNOT be empty.
        if (local_struct_vardecl_list_.empty())
        {
          cout << "local_array_vardecl_list_ is empty when meeting a local variable declaration at "; 
          PrintLocation(start_loc);
          exit(1);
        }

        local_struct_vardecl_list_.back().push_back(vd);
      }
    }
    else if (VarDeclIsPointer(vd))
    {
      if (vd->isFileVarDecl())  // global variable
      {
        global_pointer_vardecl_list_.push_back(vd);
      }
      else
      {
        // This is a local variable. scope_list_ vector CANNOT be empty.
        if (local_pointer_vardecl_list_.empty())
        {
          cout << "local_pointer_vardecl_list_ is empty when meeting a local variable declaration at "; 
          PrintLocation(start_loc);
          exit(1);
        }

        local_pointer_vardecl_list_.back().push_back(vd);
      }
    }

    return true;
  }

  bool VisitSwitchStmt(SwitchStmt *ss)
  {
    if (switchstmt_condition_range_ != nullptr)
      delete switchstmt_condition_range_;

    switchstmt_condition_range_ = new SourceRange(
        ss->getSwitchLoc(), ss->getBody()->getLocStart());

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

    /*cout << "============ start_loc ==============" << endl;
    for (auto e: switchstmt_info_list_)
    {
      PrintRange(e.first);
      PrintStringVector(e.second);
    }
    cout << "=================================" << endl;*/

    return true;
  }

  bool VisitSwitchCase(SwitchCase *sc)
  {
    if (switchcase_range_ != nullptr)
      delete switchcase_range_;

    switchcase_range_ = new SourceRange(sc->getLocStart(), sc->getColonLoc());

    // remove switch statements that are already passed
    while (!switchstmt_info_list_.empty() && 
           !LocationIsInRange(
               sc->getLocStart(), switchstmt_info_list_.back().first))
      switchstmt_info_list_.pop_back();

    return true;
  }

  bool VisitMemberExpr(MemberExpr *me)
  {
    SourceLocation start_loc = me->getLocStart();
    string reference_name{rewriter_.ConvertToString(me)};
    SourceLocation end_loc = src_mgr_.translateLineCol(
        src_mgr_.getMainFileID(),
        getLineNumber(&start_loc),
        getColNumber(&start_loc) + reference_name.length());

    //===============================
    //=== GENERATING VTWD MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_VTWD_ &&
        ExprIsScalar(cast<Expr>(me)))
    {
      if (TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
          !is_inside_enumdecl_ &&
          !LocationIsInRange(start_loc, *lhs_of_assignment_range_) &&
          !LocationIsInRange(start_loc, *unary_increment_range_) &&
          !LocationIsInRange(start_loc, *unary_decrement_range_) &&
          !LocationIsInRange(start_loc, *addressop_range_))
      {
        if (ScalarRefCanBeMutatedByVtwd(reference_name))
          GenerateVtwdMutant(&start_loc, &end_loc, reference_name, "VTWD");
        else
          // Block VTWD mutation once and 
          // remove the reference name from the nonMutatable list.
          for (auto it = non_VTWD_mutatable_scalarref_list_.begin(); 
               it != non_VTWD_mutatable_scalarref_list_.end(); ++it)
            if (reference_name.compare(*it) == 0)
            {
              non_VTWD_mutatable_scalarref_list_.erase(it);
              break;
            }
      }
    }

    //===============================
    //=== GENERATING VSCR MUTANTS ===
    //===============================
    if (mutantoperator_holder_->apply_VSCR_)
    {
      if (!is_inside_array_decl_size_ && !is_inside_enumdecl_ && 
          TargetRangeIsInMutationRange(&start_loc, &end_loc))
        GenerateVscrMutant(me, end_loc);
    }

    //===============================
    //=== GENERATING CRCR MUTANTS ===
    //===============================
    if ((mutantoperator_holder_->CRCR_floating_range_).size() != 0)
    {
      if (!is_inside_array_decl_size_ &&
          !is_inside_enumdecl_ &&
          TargetRangeIsInMutationRange(&start_loc, &end_loc) &&
          !LocationIsInRange(start_loc, *lhs_of_assignment_range_) &&
          !LocationIsInRange(start_loc, *unary_increment_range_) &&
          !LocationIsInRange(start_loc, *unary_decrement_range_) &&
          !LocationIsInRange(start_loc, *addressop_range_))
        GenerateCrcrMutant(cast<Expr>(me), start_loc, end_loc);
    }

    //=================================================
    //=== GENERATING Vsrr, Varr, Vtrr, Vprr MUTANTS ===
    //=================================================
    if (mutantoperator_holder_->apply_VGSR_ || 
        mutantoperator_holder_->apply_VLSR_ || 
        mutantoperator_holder_->apply_VGAR_ || 
        mutantoperator_holder_->apply_VLAR_ ||
        mutantoperator_holder_->apply_VGTR_ || 
        mutantoperator_holder_->apply_VLTR_ || 
        mutantoperator_holder_->apply_VGPR_ || 
        mutantoperator_holder_->apply_VLPR_)
    {
      if (!is_inside_array_decl_size_ && !is_inside_enumdecl_ && 
          TargetRangeIsInMutationRange(&start_loc, &end_loc))
      {
        if (ExprIsScalar(cast<Expr>(me)))
        {
          if (mutantoperator_holder_->apply_VGSR_)
            GenerateVgsrMutant(&start_loc, &end_loc, reference_name, "VGSR");

          if (mutantoperator_holder_->apply_VLSR_)
            GenerateVlsrMutant(&start_loc, &end_loc, reference_name, "VLSR");
        }
        else if (ExprIsArray(cast<Expr>(me)))
        {
          if (mutantoperator_holder_->apply_VGAR_)
            GenerateVgarMutant(&start_loc, &end_loc, reference_name, 
                               me->getType(), "VGAR");

          if (mutantoperator_holder_->apply_VLAR_)
            GenerateVlarMutant(&start_loc, &end_loc, reference_name, 
                               me->getType(), "VLAR");
        }
        else if (ExprIsStruct(cast<Expr>(me)))
        {
          if (mutantoperator_holder_->apply_VGTR_)
            GenerateVgtrMutant(&start_loc, &end_loc, reference_name, 
                               me->getType(), "VGTR");

          if (mutantoperator_holder_->apply_VLTR_)
            GenerateVltrMutant(&start_loc, &end_loc, reference_name, 
                               me->getType(), "VLTR");
        }
        else if (ExprIsPointer(cast<Expr>(me)))
        {
          if (mutantoperator_holder_->apply_VGPR_)
            GenerateVgprMutant(&start_loc, &end_loc, reference_name, 
                               me->getType(), "VGPR");

          if (mutantoperator_holder_->apply_VLPR_)
            GenerateVlprMutant(&start_loc, &end_loc, reference_name, 
                               me->getType(), "VLPR");
        }
      }
    }

    return true;
  }

  bool VisitStmt(Stmt *s)
  {
    SourceLocation start_loc = s->getLocStart();
    SourceLocation end_loc = s->getLocEnd();

    // set up Proteum-style line number
    if (getLineNumber(&start_loc) > proteumstyle_stmt_end_line_num_)
    {
      proteumstyle_stmt_start_line_num_ = getLineNumber(&start_loc);
      
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

        proteumstyle_stmt_end_line_num_ = getLineNumber(&end_loc_of_stmt);
      }
      else if (isa<CompoundStmt>(s) || isa<LabelStmt>(s) || isa<DoStmt>(s) || 
               isa<SwitchCase>(s) || isa<ForStmt>(s))
        proteumstyle_stmt_end_line_num_ = proteumstyle_stmt_start_line_num_;
      else
        proteumstyle_stmt_end_line_num_ = getLineNumber(&end_loc);
    }

    while (!scope_list_.empty())
    {
      if (!LocationIsInRange(start_loc, scope_list_.back()))
      {
        scope_list_.pop_back();
        local_scalar_vardecl_list_.pop_back();
        local_array_vardecl_list_.pop_back();
        local_struct_vardecl_list_.pop_back();
        local_pointer_vardecl_list_.pop_back();
      }
      else
      {
        break;
      }
    }

    if (is_inside_array_decl_size_ && 
        !LocationIsInRange(start_loc, *array_decl_range_))
    {
      is_inside_array_decl_size_ = false;
    }

    return true;
  }
  
  bool VisitFunctionDecl(FunctionDecl *f) 
  {   
    // Function with nobody, and function declaration within 
    // another function is function prototype.
    // no global or local variable mutation will be applied here.
    if (!f->hasBody() || LocationIsInRange(f->getLocStart(), 
                                           *currently_parsed_function_range_))
    {
      functionprototype_range_ = new SourceRange(f->getLocStart(), 
                                                 f->getLocEnd());
    }
    else
    {
      // entering a new local scope
      local_scalar_vardecl_list_.clear();
      local_array_vardecl_list_.clear();
      local_pointer_vardecl_list_.clear();
      local_struct_vardecl_list_.clear();
      scope_list_.clear();

      scope_list_.push_back(SourceRange(f->getLocStart(), f->getLocEnd()));
      local_scalar_vardecl_list_.push_back(VarDeclList());
      local_array_vardecl_list_.push_back(VarDeclList());
      local_struct_vardecl_list_.push_back(VarDeclList());
      local_pointer_vardecl_list_.push_back(VarDeclList());

      is_inside_enumdecl_ = false;
      // isInsideFunctionDecl = true;

      if (currently_parsed_function_range_ != nullptr)
        delete currently_parsed_function_range_;

      currently_parsed_function_range_ = new SourceRange(f->getLocStart(), 
                                                         f->getLocEnd());

      // remove local constants appearing before currently parsed function
      auto constant_iter = local_scalarconstant_list_->begin();
      for ( ; constant_iter != local_scalarconstant_list_->end(); ++constant_iter)
        if (!LocationBeforeRangeStart(constant_iter->second.first, 
                                      *currently_parsed_function_range_))
          break;
      
      if (constant_iter != local_scalarconstant_list_->begin())
        local_scalarconstant_list_->erase(local_scalarconstant_list_->begin(), 
                                          constant_iter);

      // remove local stirngs appearing before currently parsed function
      auto string_iter = local_stringliteral_list_->begin();

      for ( ; string_iter != local_stringliteral_list_->end(); ++string_iter)
        if (!LocationBeforeRangeStart(string_iter->second, 
                                      *currently_parsed_function_range_))
          break;
      
      if (string_iter != local_stringliteral_list_->begin())
        local_stringliteral_list_->erase(local_stringliteral_list_->begin(), 
                                         string_iter);
    }

    return true;
  }
};

class MyASTConsumer : public ASTConsumer
{
public:
  MyASTConsumer(SourceManager &source_mgr, LangOptions &lang_option, 
                UserInput *user_input, MutantOperatorHolder *holder, 
                CompilerInstance *CI, vector<SourceLocation> *labels, 
                LabelStmtToGotoStmtListMap *label_to_gotolist_map, 
                GlobalScalarConstantList *global_scalar_constant_list,
                LocalScalarConstantList *local_scalar_constant_list, 
                GlobalStringLiteralList *global_string_literal_list,
                LocalStringLiteralList *local_string_literal_list) 
    : Visitor(source_mgr, lang_option, user_input, holder, CI, labels, 
              label_to_gotolist_map, global_scalar_constant_list, 
              local_scalar_constant_list, global_string_literal_list, 
              local_string_literal_list) 
  { 
  }

  void setSema(Sema *sema)
  {
    Visitor.setSema(sema);
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

class InformationVisitor : public RecursiveASTVisitor<InformationVisitor>
{
public:
  CompilerInstance *comp_inst_;
  MutantOperatorHolder *mutantoperator_holder_;

  SourceManager &src_mgr_;
  LangOptions &lang_option_;

  Rewriter rewriter_;

  // List of locations of label declarations
  vector<SourceLocation> *label_srclocation_list_; 

  // Map from label declaration location to locations of Goto statements
  // pointing to that label.
  LabelStmtToGotoStmtListMap label_to_gotolist_map_;

  // Global/Local numbers, chars
  GlobalScalarConstantList global_scalarconstant_list_;
  LocalScalarConstantList local_scalarconstant_list_;

  // Last (or current) range of the function COMUT is traversing
  SourceRange *currently_parsed_function_range_;

  // A set holding all distinguished consts in currently/last parsed fuction.
  set<string> local_scalar_constant_cache_;

  // A set holding all distinguished consts with global scope
  set<string> global_scalar_constant_cache_;

  // A vector holding string literals used outside a function (global scope)
  GlobalStringLiteralList global_stringliteral_list_;

  // A vector holding string literals used inside a function (local scope)
  // and their location.
  LocalStringLiteralList local_stringliteral_list_;

  InformationVisitor(CompilerInstance *CI, MutantOperatorHolder *holder)
    : comp_inst_(CI), mutantoperator_holder_(holder), 
      src_mgr_(CI->getSourceManager()), lang_option_(CI->getLangOpts())
  {
    label_srclocation_list_ = new vector<SourceLocation>();
    currently_parsed_function_range_ = new SourceRange(
        src_mgr_.getLocForStartOfFile(src_mgr_.getMainFileID()),
        src_mgr_.getLocForStartOfFile(src_mgr_.getMainFileID()));

    rewriter_.setSourceMgr(src_mgr_, lang_option_);
  }

  // Return the line number of a source location.
  int getLineNumber(SourceLocation *location) 
  {
    return static_cast<int>(src_mgr_.getExpansionLineNumber(*location));
  }

  // Return the column number of a source location.
  int getColNumber(SourceLocation *location) 
  {
    return static_cast<int>(src_mgr_.getExpansionColumnNumber(*location));
  }

  // Return True if the location is inside the range (inclusive)
  // Return False otherwise
  bool LocationIsInRange(SourceLocation location, SourceRange range)
  {
    SourceLocation start_loc = range.getBegin();
    SourceLocation end_loc = range.getEnd();

    // line number of location is out of range
    if (getLineNumber(&location) < getLineNumber(&start_loc) || 
        getLineNumber(&location) > getLineNumber(&end_loc))
      return false;

    // line number of location is same as line number of start_loc but col number of location is out of range
    if (getLineNumber(&location) == getLineNumber(&start_loc) && 
        getColNumber(&location) < getColNumber(&start_loc))
      return false;

    // line number of location is same as line number of end_loc but col number of location is out of range
    if (getLineNumber(&location) == getLineNumber(&end_loc) && 
        getColNumber(&location) > getColNumber(&end_loc))
      return false;

    return true;
  }

  void PrintLocation(SourceLocation location)
  {
    cout << getLineNumber(&location) << "\t" << getColNumber(&location) << endl;
  }

  // Add a new Goto statement location to LabelStmtToGotoStmtListMap.
  // Add the label to the map if map does not contain label.
  // Else add the Goto location to label's list of Goto locations.
  void addGotoLocToMap(LabelStmtLocation label_loc, 
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
  
  bool VisitLabelStmt(LabelStmt *ls)
  {
    string labelName{ls->getName()};
    SourceLocation start_loc = ls->getLocStart();

    // Insert new entity into list of labels and LabelStmtToGotoStmtListMap
    label_srclocation_list_->push_back(start_loc);
    label_to_gotolist_map_.insert(pair<LabelStmtLocation, GotoStmtLocationList>(
        LabelStmtLocation(getLineNumber(&start_loc),
                          getColNumber(&start_loc)),
        GotoStmtLocationList()));

    return true;
  }  

  bool VisitGotoStmt(GotoStmt * gs)
  {
    // Retrieve LabelStmtToGotoStmtListMap's key which is label declaration location.
    LabelStmt *label = gs->getLabel()->getStmt();
    SourceLocation labelStartLoc = label->getLocStart();

    addGotoLocToMap(LabelStmtLocation(getLineNumber(&labelStartLoc), 
                                      getColNumber(&labelStartLoc)), 
                    gs->getGotoLoc());

    return true;
  }



  bool VisitExpr(Expr *e)
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

  bool VisitFunctionDecl(FunctionDecl *fd)
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
};

class InformationGatherer : public ASTConsumer
{
public:
  InformationGatherer(CompilerInstance *CI, MutantOperatorHolder *holder)
    :Visitor(CI, holder)
  {
  }

  // ~InformationGatherer();

  virtual void HandleTranslationUnit(ASTContext &Context)
  {
    /* we can use ASTContext to get the TranslationUnitDecl, which is
    a single Decl that collectively represents the entire source file */
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

  vector<SourceLocation>* getLabels()
  {
    return Visitor.label_srclocation_list_;
  }

  LabelStmtToGotoStmtListMap* getLabelToGotoListMap()
  {
    return &(Visitor.label_to_gotolist_map_);
  }

  GlobalScalarConstantList* getAllGlobalScalarConstants()
  {
    return &(Visitor.global_scalarconstant_list_);
  }

  LocalScalarConstantList* getAllLocalScalarConstants()
  {
    return &(Visitor.local_scalarconstant_list_);
  }

  GlobalStringLiteralList* getAllGlobalStringLiterals()
  {
    return &(Visitor.global_stringliteral_list_);
  }

  LocalStringLiteralList* getAllLocalStringLiterals()
  {
    return &(Visitor.local_stringliteral_list_);
  }
  
private:
  InformationVisitor Visitor;
};

/** 
  Check if this directory exists

  @param  directory the directory to be checked
  @return True if the directory exists
      False otherwise
*/
bool DirectoryExists( const std::string &directory )
{
  if( !directory.empty() )
  {
    if( access(directory.c_str(), 0) == 0 )
    {
      struct stat status;
      stat( directory.c_str(), &status );
      if( status.st_mode & S_IFDIR )
        return true;
    }
  }
  // if any condition fails
  return false;
}

void PrintUsageErrorMsg()
{
  cout << "Invalid command.\n";
  cout << "Usage: tool <filename> [-m <mutant_name> [-A \"<domain>\"] ";
  cout << "[-B \"<range>\"]] [-rs <line #> <col #>] [-re <line #> <col #>]";
  cout << " [-l <max>] [-o <dir>]" << endl;
}

void PrintLineColNumberErrorMsg()
{
  cout << "Invalid line/column number\n";
  cout << "Usage: [-rs <line #> <col #>] [-re <line #> <col #>]\n";
}

// Called when incrementing iterator variable i in a loop
// When incrementing i in a loop, I expected that i will not exceed max.
// However if user made mistake in input command, it can happen.
void increment_i(int *i, int max)
{
  ++(*i);
  if ((*i) >= max)
  {
    PrintUsageErrorMsg();
    exit(1);
  }
}

InformationGatherer* GetNecessaryDataFromInputFile(
    char *filename, MutantOperatorHolder *holder)
{
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
    hso->AddPath(include_paths[i], 
          clang::frontend::Angled, 
          false, 
          false);
  // </Warning!!> -- End of Platform Specific Code

  InitializePreprocessor(TheCompInst.getPreprocessor(), 
                         TheCompInst.getPreprocessorOpts(),
                         *hso,
                         TheCompInst.getFrontendOpts());

  // Set the main file handled by the source manager to the input file.
  const FileEntry *FileIn = FileMgr.getFile(filename);
  SourceMgr.createMainFileID(FileIn);
  
  // Inform Diagnostics that processing of a source file is beginning. 
  TheCompInst.getDiagnosticClient().BeginSourceFile(
      TheCompInst.getLangOpts(),&TheCompInst.getPreprocessor());

  // Parse the file to AST, gather labelstmts, goto stmts, 
  // scalar constants, string literals. 
  InformationGatherer *TheGatherer = new InformationGatherer(&TheCompInst, 
                                                             holder);

  ParseAST(TheCompInst.getPreprocessor(), TheGatherer, 
           TheCompInst.getASTContext());

  return TheGatherer;
}

// Wrap up currently-entering mutant operator (if can)
// before change the state to kNonAOrBOption.
void clearState(UserInputAnalyzingState &state, string mutant_name, 
                set<string> &domain, set<string> &range, 
                MutantOperatorHolder *holder)
{
  switch (state)
  {
    case UserInputAnalyzingState::kAnyOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOrBOptionAndMutantName:
    {
      // there is a mutant operator to be wrapped
      ValidateDomainOfMutantOperator(mutant_name, domain);
      ValidateRangeOfMutantOperator(mutant_name, range);
      MutantOperator new_mutant_operator{mutant_name, domain, range};

      if (!(holder->AddMutantOperator(new_mutant_operator)))
      {
        cout << "Error adding operator " << mutant_name << endl;
        exit(1);
      }

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
                 set<string> &range, MutantOperatorHolder *holder, 
                 string &output_dir, int &limit, SourceLocation *start_loc, 
                 SourceLocation *end_loc, int &line_num, int &col_num, 
                 SourceManager &src_mgr)
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
        cout << "Input line number is not positive. Default to 1.\n";
        line_num = 1;
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
        cout << "Input col number is not positive. Default to 1.\n";
        col_num = 1;
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
        cout << "Input line number is not positive. Default to 1.\n";
        line_num = 1;
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
        cout << "Input col number is not positive. Default to 1.\n";
        col_num = 1;
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
      ValidateDomainOfMutantOperator(mutant_name, domain);
      ValidateRangeOfMutantOperator(mutant_name, range);
      MutantOperator new_mutant_operator{mutant_name, domain, range};

      if (!(holder->AddMutantOperator(new_mutant_operator)))
      {
        cout << "Error adding operator " << mutant_name << endl;
        exit(1);
      }

      state = UserInputAnalyzingState::kMutantName;
      domain.clear();
      range.clear();

      HandleInput(input, state, mutant_name, domain, range, holder, 
                  output_dir, limit, start_loc, end_loc, line_num, 
                  col_num, src_mgr);
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
    hso->AddPath(include_paths[i], 
          clang::frontend::Angled, 
          false, 
          false);
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

  MutantOperatorHolder *holder = new MutantOperatorHolder();

  // if -m option is specified, apply only specified operators to input file.
  bool apply_all_mutant_operators = true;

  // Validate and analyze user's inputs.
  UserInputAnalyzingState state = UserInputAnalyzingState::kNonAOrBOption;
  string mutantOpName{""};
  set<string> domain;
  set<string> range;
  int line_num{0};
  int col_num{0};

  for (int i = 2; i < argc; ++i)
  {
    string option = argv[i];
    cout << "handling " << option << endl;

    if (option.compare("-m") == 0)
    {
      apply_all_mutant_operators = false;
      clearState(state, mutantOpName, domain, range, holder);
      
      mutantOpName.clear();
      domain.clear();
      range.clear();
      
      state = UserInputAnalyzingState::kMutantName;
    }
    else if (option.compare("-l") == 0)
    {
      clearState(state, mutantOpName, domain, range, holder);
      state = UserInputAnalyzingState::kLimitNumOfMutant;
    }
    else if (option.compare("-rs") == 0)
    {
      clearState(state, mutantOpName, domain, range, holder);
      state = UserInputAnalyzingState::kRsLine;
    }
    else if (option.compare("-re") == 0)
    {
      clearState(state, mutantOpName, domain, range, holder);
      state = UserInputAnalyzingState::kReLine;
    }
    else if (option.compare("-o") == 0)
    {
      clearState(state, mutantOpName, domain, range, holder);
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
      HandleInput(option, state, mutantOpName, domain, range, holder,
                  output_dir, limit, &start_of_mutation_range, 
                  &end_of_mutation_range, line_num, col_num, SourceMgr);
      // cout << "mutant mutant_name name: " << mutantOpName << endl;
      // cout << "domain: "; PrintStringSet(domain);
      // cout << "range "; PrintStringSet(range);
      // cout << "out dir: " << output_dir << endl;
      // cout << "limit: " << limit << endl;
      // cout << "start_loc: " << start_of_mutation_range.printToString(SourceMgr) << endl;
      // cout << "end_loc: " << end_of_mutation_range.printToString(SourceMgr) << endl;
      // cout << "line col: " << line_num << " " << col_num << endl;
      // cout << "==========================================\n\n";
    }
  }

  clearState(state, mutantOpName, domain, range, holder);

  if (apply_all_mutant_operators) 
  {
    // holder->useAll();
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

  // Create UserInput object pointer to pass as attribute for MyASTConsumer
  UserInput *userInput = new UserInput(inputFilename, mutDbFilename, 
                                       start_of_mutation_range, 
                                       end_of_mutation_range, output_dir, 
                                       limit);

  //=======================================================
  //====================== PARSING ========================
  //=======================================================
  InformationGatherer *TheGatherer = GetNecessaryDataFromInputFile(
      argv[1], holder);

  // Create an AST consumer instance which is going to get called by ParseAST.
  MyASTConsumer TheConsumer(SourceMgr, TheCompInst.getLangOpts(), userInput, 
                            holder, &TheCompInst, TheGatherer->getLabels(), 
                            TheGatherer->getLabelToGotoListMap(), 
                            TheGatherer->getAllGlobalScalarConstants(), 
                            TheGatherer->getAllLocalScalarConstants(), 
                            TheGatherer->getAllGlobalStringLiterals(),
                            TheGatherer->getAllLocalStringLiterals());

  Sema sema(TheCompInst.getPreprocessor(), TheCompInst.getASTContext(), 
            TheConsumer);
  TheConsumer.setSema(&sema);

  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(sema);

  // if (TheGatherer != nullptr)
  //   delete TheGatherer;
  
  // if (userInput != nullptr)
  //   delete userInput;

  return 0;
}
