#ifndef MUSIC_INFORMATION_GATHERER_H_
#define MUSIC_INFORMATION_GATHERER_H_ 

#include "information_visitor.h"

class InformationGatherer : public clang::ASTConsumer
{
public:
  InformationGatherer(clang::CompilerInstance *CI)
    :Visitor(CI)
  {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context);

  LabelStmtToGotoStmtListMap* getLabelToGotoListMap();

  SymbolTable* getSymbolTable();
  
private:
  InformationVisitor Visitor;
};

#endif	// MUSIC_INFORMATION_GATHERER_H_
