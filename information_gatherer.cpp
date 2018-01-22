#include "information_gatherer.h"
#include <stdio.h>

void InformationGatherer::HandleTranslationUnit(clang::ASTContext &Context)
{
  /* we can use ASTContext to get the TranslationUnitDecl, which is
  a single Decl that collectively represents the entire source file */
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}

LabelStmtToGotoStmtListMap* InformationGatherer::getLabelToGotoListMap()
{
  return Visitor.getLabelToGotoListMap();
}

SymbolTable* InformationGatherer::getSymbolTable()
{
	return Visitor.getSymbolTable();
}