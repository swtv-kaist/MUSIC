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
                 std::string input_filename, std::string output_dir);

  void AddMutantEntry(MutantName name, clang::SourceLocation start_loc,
                      clang::SourceLocation end_loc, std::string token,
                      std::string mutated_token, int proteum_style_line_num);
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

  MutantEntryTable mutant_entry_table_;
  std::string input_filename_;
  std::string database_filename_;
  std::string output_dir_;
  int next_mutantfile_id_;

  std::string GetNextMutantFilename();
  void IncrementNextMutantfileId();
};

#endif  // MUSIC_MUTANT_DATABASE_H_