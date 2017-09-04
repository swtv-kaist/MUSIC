#ifndef COMUT_INFORMATION_GATHERER_H_
#define COMUT_INFORMATION_GATHERER_H_ 

#include "mutant_operator_holder.h"
#include "information_visitor.h"

class InformationGatherer : public clang::ASTConsumer
{
public:
  InformationGatherer(clang::CompilerInstance *CI)
    :Visitor(CI)
  {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context);

  // std::vector<clang::SourceLocation>* getLabels();
  LabelStmtToGotoStmtListMap* getLabelToGotoListMap();
  GlobalScalarConstantList* getAllGlobalScalarConstants();
  LocalScalarConstantList* getAllLocalScalarConstants();
  GlobalStringLiteralList* getAllGlobalStringLiterals();
  LocalStringLiteralList* getAllLocalStringLiterals();

  VarDeclList* getGlobalScalarVardeclList();
  std::vector<VarDeclList>* getLocalScalarVardeclList();

  SymbolTable* getSymbolTable();
  
private:
  InformationVisitor Visitor;
};

#endif	// COMUT_INFORMATION_GATHERER_H_