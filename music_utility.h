#ifndef MUSIC_UTILITY_H_
#define MUSIC_UTILITY_H_

#include <cstdio>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <iomanip>
#include <sys/stat.h>
#include <unistd.h>
#include <cctype>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ParentMapContext.h"
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

#include "mutant_database.h"

using namespace clang;
using namespace std;

// extern set<string> arithemtic_operators;
// extern set<string> bitwise_operators;
// extern set<string> shift_operators;
// extern set<string> logical_operators;
// extern set<string> relational_operators;
// extern set<string> arith_assignment_operators;
// extern set<string> bitwise_assignment_operators;
// extern set<string> shift_assignment_operators;
// extern set<string> assignment_operator;

string ConvertToString(Stmt *from, LangOptions &LangOpts);

// Print out each element of a string set in a single line.
void PrintStringSet(std::set<std::string> &string_set);

void PrintStringVector(const std::vector<std::string> &string_vector);

// remove spaces at the beginning and end_loc of string str
string TrimBeginningAndEndingWhitespace(const string &str);

// Divide string into elements of a string set with delimiter
void SplitStringIntoSet(string target, set<string> &out_set, 
                        const string delimiter);

void SplitStringIntoVector(string target, vector<string> &out_vector, 
                           const string delimiter);

/**
  @param  hexa: hexa string of the following form "'\xF...F'"
  @return string of the integer value of the given hexa string
*/
string ConvertHexaStringToIntString(const string hexa_str);

string ConvertCharStringToIntString(const string s);

bool IsStringElementOfVector(string s, vector<string> &string_vector);

bool IsStringElementOfSet(string s, set<string> &set);

bool ConvertStringToInt(string s, int &n);

/**
  @param  s string literal from C input file
          pos index at which to check if the character there is a whitespace
  @return True if the character at pos is a whitespace
          False otherwise
          If the character at pos is whitespace, pos is changed to the
          position after the whitespace
*/
bool IsWhitespace(const string &s, int &pos);

bool IsValidVariableName(string s);

/**
  @param  s string literal from input file
  @return index of the first non whitespace character (space, \f, \n, \r, \t, \v)

  Return an int higher than 1
*/
int GetFirstNonWhitespaceIndex(const string &s);

int GetLastNonWhitespaceIndex(const string &s);

/**
  Check if a given string is a float number. (existence of dot(.))

  @param  num targeted string
  @return true if it is floating type
          false otherwise
*/
bool NumIsFloat(std::string num);

/**
  Check whether a string is a number (int or real)

  @param  target targeted string
  @return true if it is a number
      		false otherwise
*/
bool StringIsANumber(std::string target);

/** 
  Check whether the domain of an operator specified by user is correct

  @param  mutant_name name of the operator
          domain string set containing targeted tokens
*/
void ValidateDomainOfMutantOperator(std::string &mutant_name, 
																		std::set<std::string> &domain);

/** 
  Check whether the range of an operator specified by user is correct

  @param  mutant_name name of the operator
          range string set containing tokens to mutate with
*/
void ValidateRangeOfMutantOperator(std::string &mutant_name, 
																	 std::set<std::string> &range);

int GetLineNumber(SourceManager &src_mgr, SourceLocation loc);

int GetColumnNumber(SourceManager &src_mgr, SourceLocation loc);

SourceLocation TryGetEndLocAfterBracketOrSemicolon(SourceLocation loc, 
                               CompilerInstance *comp_inst);

void PrintLocation(SourceManager &src_mgr, SourceLocation loc);

void PrintRange(SourceManager &src_mgr, SourceRange range);

int CountNonNewlineChar(const string &s);

bool LocationBeforeRangeStart(SourceLocation loc, SourceRange range);

bool LocationAfterRangeEnd(SourceLocation loc, SourceRange range);

bool LocationIsInRange(SourceLocation loc, SourceRange range);

bool Range1IsPartOfRange2(SourceRange range1, SourceRange range2);

int CountNonNullStmtInCompoundStmt(CompoundStmt *c);

SourceLocation GetLocationAfterSemicolon(SourceManager &src_mgr_, 
                                         SourceLocation loc);

bool ExprIsPointer(Expr *e);
bool ExprIsScalar(Expr *e);
bool ExprIsFloat(Expr *e);
bool ExprIsIntegral(CompilerInstance *comp_inst, Expr *e);
bool ExprIsArray(Expr *e);
bool ExprIsStruct(Expr *e);
bool ExprIsDeclRefExpr(Expr *e);
bool ExprIsPointerDereferenceExpr(Expr *e);
bool ExprIsScalarReference(Expr *e);
bool ExprIsArrayReference(Expr *e);
bool ExprIsStructReference(Expr *e);
bool ExprIsPointerReference(Expr *e);

bool OperatorIsArithmeticAssignment(BinaryOperator *b);
bool OperatorIsShiftAssignment(BinaryOperator *b);
bool OperatorIsBitwiseAssignment(BinaryOperator *b);

SourceLocation GetEndLocOfStringLiteral(
    SourceManager &src_mgr, SourceLocation start_loc);

/**
  @param  start_loc: start_loc location of the targeted literal
  @return end_loc location of number, char literal
*/
SourceLocation GetEndLocOfConstantLiteral(
    SourceManager &src_mgr, SourceLocation start_loc);

/**
  @param  uo: pointer to expression with unary operator
  @return end_loc location of given expression
*/
SourceLocation GetEndLocOfUnaryOpExpr(
    UnaryOperator *uo, CompilerInstance *comp_inst);

SourceLocation GetEndLocOfExpr(Expr *e, CompilerInstance *comp_inst);

string GetVarDeclName(const VarDecl *vd);
bool IsVarDeclConst(const VarDecl *vd);
bool IsVarDeclPointer(const VarDecl *vd);
bool IsVarDeclArray(const VarDecl *vd);
bool IsVarDeclScalar(const VarDecl *vd);
bool IsVarDeclFloating(const VarDecl *vd);
bool IsVarDeclStruct(const VarDecl *vd);

/**
  Return the type of array element
  Example: int[] -> int

  @param: type  type of the array
*/
string getArrayElementType(QualType type);

string getStructureType(QualType type);

// Return the type of the entity the pointer is pointing to.
string getPointerType(QualType type);

// Return True if the 2 types are same
bool sameArrayElementType(QualType type1, QualType type2);

bool isCompatibleType(QualType type1, QualType type2);

/** 
  Check if this directory exists

  @param  directory the directory to be checked
  @return True if the directory exists
      False otherwise
*/
bool DirectoryExists(const std::string &directory);

void PrintUsageErrorMsg();

void PrintLineColNumberErrorMsg();

const Stmt* GetParentOfStmt(Stmt *s, CompilerInstance *comp_inst);
const Stmt *GetSecondLevelParent(Stmt *s, CompilerInstance *comp_inst);

Expr* GetLeftOperandAfterMutationToMultiplicativeOp(Expr *lhs);
Expr* GetRightOperandAfterMutationToMultiplicativeOp(Expr *rhs);

BinaryOperator::Opcode TranslateToOpcode(const string &binary_operator);

int GetPrecedenceOfBinaryOperator(BinaryOperator::Opcode opcode);

Expr* GetLeftOperandAfterMutation(
    Expr *lhs, const BinaryOperator::Opcode mutated_opcode);
Expr* GetRightOperandAfterMutation(
    Expr *rhs, const BinaryOperator::Opcode mutated_opcode);

ostream& operator<<(ostream &stream, const MutantEntry &entry);
ostream& operator<<(ostream &stream, const MutantDatabase &database);

Expr* IgnoreParenExpr(Expr *e);

void ConvertConstIntExprToIntString(Expr *e, CompilerInstance *comp_inst,
                                    string &str);
void ConvertConstFloatExprToFloatString(Expr *e, CompilerInstance *comp_inst,
                                        string &str);

bool SortFloatAscending (long double i,long double j);
bool SortIntAscending (long long i,long long j);
bool SortStringAscending (string i,string j);

string GetMaxValue(QualType qualtype);
string GetMinValue(QualType qualtype);

#endif  // MUSIC_UTILITY_H_
