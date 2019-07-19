#ifndef MUSIC_MUTANT_DATABASE_H_
#define MUSIC_MUTANT_DATABASE_H_

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/LangOptions.h"

#include "mutant_entry.h"
#include "stmt_context.h"

typedef int LineNumber;
typedef int ColumnNumber;
typedef std::string MutantName;

typedef std::vector<MutantEntry> MutantEntryList;
typedef std::map<MutantName, MutantEntryList> MutantNameToEntryMap;
typedef std::map<ColumnNumber, MutantNameToEntryMap> ColumnNumToEntryMap;
typedef std::map<LineNumber, ColumnNumToEntryMap> MutantEntryTable;

class MutantDatabase
{
public:
  MutantDatabase(clang::CompilerInstance *comp_inst, 
                 std::string input_filename, std::string output_dir, int limit);

  void AddMutantEntry(StmtContext& stmt_context, 
                      MutantName name, clang::SourceLocation start_loc,
                      clang::SourceLocation end_loc, std::string token,
                      std::string mutated_token, int proteum_style_line_num,
                      std::string additional_info="", 
                      std::vector<std::string> extra_tokens = std::vector<std::string>(),
                      std::vector<std::string> extra_mutated_tokens = std::vector<std::string>(),
                      std::vector<clang::SourceLocation> extra_start_locs = std::vector<clang::SourceLocation>(),
                      std::vector<clang::SourceLocation> extra_end_locs = std::vector<clang::SourceLocation>(), 
                      std::vector<std::string> additional_infos = std::vector<std::string>());
  void WriteEntryToDatabaseFile(std::string mutant_name, const MutantEntry &entry);
  void WriteAllEntriesToDatabaseFile();
  void WriteEntryToMutantFile(const MutantEntry &entry);
  void WriteAllEntriesToMutantFile();
  void ExportAllEntries();

  const MutantEntryTable& getEntryTable() const;

private:
  clang::CompilerInstance *comp_inst_;
  clang::SourceManager &src_mgr_;
  clang::LangOptions &lang_opts_;
  clang::Rewriter rewriter_;

  MutantEntryTable mutant_entry_table_;
  std::string input_filename_;
  std::string database_filename_;
  std::string output_dir_;
  int next_mutantfile_id_;

  // maxi number of mutants generated per mutation point per mutation operator
  int num_mutant_limit_;

  std::string GetNextMutantFilename();
  void IncrementNextMutantfileId();
  bool ValidateSourceRange(clang::SourceLocation &start_loc, clang::SourceLocation &end_loc);
};

#endif  // MUSIC_MUTANT_DATABASE_H_