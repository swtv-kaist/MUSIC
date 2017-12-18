#include <time.h>

#include "clang/Rewrite/Core/Rewriter.h"

#include "comut_utility.h"
#include "mutant_database.h"

void GenerateRandomNumbers(set<int> &s, int desired_size, int cap)
{
  while (s.size() != desired_size)
    s.insert(rand() % cap);
}

MutantDatabase::MutantDatabase(clang::CompilerInstance *comp_inst, 
               std::string input_filename, std::string output_dir)
: comp_inst_(comp_inst), input_filename_(input_filename),
output_dir_(output_dir), next_mutantfile_id_(1), 
src_mgr_(comp_inst->getSourceManager()), lang_opts_(comp_inst->getLangOpts())
{
  // set database filename with output directory prepended
  database_filename_ = output_dir;
  database_filename_.append(input_filename_, 0, input_filename_.length()-2);
  database_filename_ += "_mut_db.out";
}

void MutantDatabase::AddMutantEntry(MutantName name, clang::SourceLocation start_loc,
                    clang::SourceLocation end_loc, std::string token,
                    std::string mutated_token, int proteum_style_line_num)
{
  int line_num = GetLineNumber(src_mgr_, start_loc);
  int col_num = GetColumnNumber(src_mgr_, start_loc);
  MutantEntry new_entry(token, mutated_token, start_loc, 
                        end_loc, src_mgr_, proteum_style_line_num);

  auto line_map_iter = mutant_entry_table_.find(line_num);

  // if this is the first mutant on this line to be recorded,
  // add new entries all the way down 3 levels.
  if (line_map_iter == mutant_entry_table_.end())
  {
    mutant_entry_table_[line_num] = ColumnNumToEntryMap();
    mutant_entry_table_[line_num][col_num] = MutantNameToEntryMap();
    mutant_entry_table_[line_num][col_num][name] = MutantEntryList();
    mutant_entry_table_[line_num][col_num][name].push_back(new_entry);
  }
  // if this is not the first mutant on this line, check if it is the first
  // on this column
  else
  {
    auto column_map_iter = mutant_entry_table_[line_num].find(col_num);

    if (column_map_iter == mutant_entry_table_[line_num].end())
    {
      mutant_entry_table_[line_num][col_num] = MutantNameToEntryMap();
      mutant_entry_table_[line_num][col_num][name] = MutantEntryList();
      mutant_entry_table_[line_num][col_num][name].push_back(new_entry);
    }
    else
    {
      auto mutantname_map_iter = \
          mutant_entry_table_[line_num][col_num].find(name);

      if (mutantname_map_iter == mutant_entry_table_[line_num][col_num].end())
      {
        mutant_entry_table_[line_num][col_num][name] = MutantEntryList();
        mutant_entry_table_[line_num][col_num][name].push_back(new_entry);
      }
      else
        mutant_entry_table_[line_num][col_num][name].push_back(new_entry);
    }
  }
}

void MutantDatabase::WriteEntryToDatabaseFile(
    string mutant_name, const MutantEntry &entry)
{
  // Open mutattion database file in APPEND mode
  ofstream mutant_db_file(database_filename_.data(), ios::app);

  // write input file name
  mutant_db_file << input_filename_ << "\t";

  // write mutant file name
  mutant_db_file << GetNextMutantFilename() << "\t"; 

  // write name of operator  
  mutant_db_file << mutant_name << "\t";

  // write information about token BEFORE mutation
  mutant_db_file << entry.getProteumStyleLineNum() << "\t";
  mutant_db_file << GetLineNumber(src_mgr_, entry.getStartLocation()) << "\t";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getStartLocation()) << "\t";
  mutant_db_file << GetLineNumber(src_mgr_, entry.getTokenEndLocation()) << "\t";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getTokenEndLocation()) << "\t";
  mutant_db_file << entry.getToken() << "\t";

  // write information about token AFTER mutation
  mutant_db_file << GetLineNumber(src_mgr_, entry.getStartLocation()) << "\t";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getStartLocation()) << "\t";
  mutant_db_file << GetLineNumber(src_mgr_, entry.getMutatedTokenEndLocation()) << "\t";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getMutatedTokenEndLocation()) << "\t";
  mutant_db_file << entry.getMutatedToken() << endl;

  // close database file
  mutant_db_file.close(); 
}

void MutantDatabase::WriteAllEntriesToDatabaseFile()
{
  for (auto line_map_iter: mutant_entry_table_)
    for (auto column_map_iter: line_map_iter.second)
      for (auto mutantname_map_iter: column_map_iter.second)
        for (auto entry: mutantname_map_iter.second)
        {
          WriteEntryToDatabaseFile(mutantname_map_iter.first, entry);
          IncrementNextMutantfileId();
        }
}

void MutantDatabase::WriteEntryToMutantFile(const MutantEntry &entry)
{
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr_, lang_opts_);

  int length = src_mgr_.getFileOffset(entry.getTokenEndLocation()) - \
               src_mgr_.getFileOffset(entry.getStartLocation());

  rewriter.ReplaceText(entry.getStartLocation(), length, 
                       entry.getMutatedToken());

  string mutant_filename{output_dir_};
  mutant_filename += GetNextMutantFilename();
  mutant_filename += ".c";

  // Make and write mutated code to output file.
  const RewriteBuffer *RewriteBuf = rewriter.getRewriteBufferFor(
      src_mgr_.getMainFileID());
  ofstream output(mutant_filename.data());
  output << string(RewriteBuf->begin(), RewriteBuf->end());
  output.close(); 
}

void MutantDatabase::WriteAllEntriesToMutantFile()
{
  for (auto line_map_iter: mutant_entry_table_)
    for (auto column_map_iter: line_map_iter.second)
      for (auto mutantname_map_iter: column_map_iter.second)
        for (auto entry: mutantname_map_iter.second)
        {
          WriteEntryToMutantFile(entry);
          IncrementNextMutantfileId();
        }
}

// generate mutant file and write to database file
void MutantDatabase::ExportAllEntries()
{
  for (auto line_map_iter: mutant_entry_table_)
    for (auto column_map_iter: line_map_iter.second)
      for (auto mutantname_map_iter: column_map_iter.second)
        for (auto entry: mutantname_map_iter.second)
        {
          WriteEntryToDatabaseFile(mutantname_map_iter.first, entry);
          WriteEntryToMutantFile(entry);
          IncrementNextMutantfileId();
        }
}

const MutantEntryTable& MutantDatabase::getEntryTable() const
{
  return mutant_entry_table_;
}

string MutantDatabase::GetNextMutantFilename()
{
  // if input filename is "test.c" and next_mutantfile_id_ is 1,
  // then the next mutant filename is "test.MUT1.c"
  // this function will, however, return "test.MUT1" 
  // for use in both database record and mutant file generation
  string mutant_filename;
  mutant_filename.assign(input_filename_, 0, input_filename_.length()-2);
  mutant_filename += ".MUT";
  mutant_filename += to_string(next_mutantfile_id_);
  return mutant_filename;
}

void MutantDatabase::IncrementNextMutantfileId()
{
  next_mutantfile_id_++;
}