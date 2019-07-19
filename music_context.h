#ifndef MUSIC_CONTEXT_H_
#define MUSIC_CONTEXT_H_ 

#include <vector>
#include <string>
#include <utility>
#include <map>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"

#include "configuration.h"
#include "symbol_table.h"
#include "stmt_context.h"
#include "mutant_database.h"

typedef std::vector<std::string> ScalarReferenceNameList;

// Block scope are bounded by curly braces {}.
// The closer the scope is to the end_loc of vector, the smaller it is.
// ScopeRangeList = {scope1, scope2, scope3, ...}
// {...scope1
//   {...scope2
//     {...scope3
//     }
//   }
// }  

class MusicContext
{
public:
  clang::CompilerInstance *comp_inst_;
  
  LabelStmtToGotoStmtListMap *label_to_gotolist_map_;
  SwitchStmtInfoList *switchstmt_info_list_;

  ScalarReferenceNameList *non_VTWD_mutatable_scalarref_list_;

  ScopeRangeList *scope_list_;

  MutantDatabase &mutant_database_;

  MusicContext(
      clang::CompilerInstance *CI, Configuration *config,
      LabelStmtToGotoStmtListMap *label_map, 
      SymbolTable* symbol_table, MutantDatabase &mutant_database);

  bool IsRangeInMutationRange(clang::SourceRange range);

  int getFunctionId();
  SymbolTable* getSymbolTable();
  StmtContext& getStmtContext();
  Configuration* getConfiguration() const;

  void IncrementFunctionId();

private:
  int function_id_;

  SymbolTable *symbol_table_;
  StmtContext stmt_context_;
  Configuration *config_;
};

#endif	// MUSIC_CONTEXT_H_