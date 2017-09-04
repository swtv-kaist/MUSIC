#include "information_gatherer.h"

void InformationGatherer::HandleTranslationUnit(clang::ASTContext &Context)
{
  /* we can use ASTContext to get the TranslationUnitDecl, which is
  a single Decl that collectively represents the entire source file */
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}

/*std::vector<clang::SourceLocation>* InformationGatherer::getLabels()
{
  return Visitor.label_srclocation_list_;
}*/

LabelStmtToGotoStmtListMap* InformationGatherer::getLabelToGotoListMap()
{
  return &(Visitor.label_to_gotolist_map_);
}

GlobalScalarConstantList* InformationGatherer::getAllGlobalScalarConstants()
{
  return &(Visitor.global_scalarconstant_list_);
}

LocalScalarConstantList* InformationGatherer::getAllLocalScalarConstants()
{
  return &(Visitor.local_scalarconstant_list_);
}

GlobalStringLiteralList* InformationGatherer::getAllGlobalStringLiterals()
{
  return &(Visitor.global_stringliteral_list_);
}

LocalStringLiteralList* InformationGatherer::getAllLocalStringLiterals()
{
  return &(Visitor.local_stringliteral_list_);
}

VarDeclList* InformationGatherer::getGlobalScalarVardeclList()
{
	return Visitor.getGlobalScalarVardeclList();
}

std::vector<VarDeclList>* InformationGatherer::getLocalScalarVardeclList()
{
	return Visitor.getLocalScalarVardeclList();
}

SymbolTable* InformationGatherer::getSymbolTable()
{
	return Visitor.getSymbolTable();
}