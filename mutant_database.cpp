#include <time.h>
#include <algorithm>

#include "clang/Rewrite/Core/Rewriter.h"

#include "music_utility.h"
#include "mutant_database.h"

void GenerateRandomNumbers(set<int> &s, int desired_size, int cap)
{
  while (s.size() != desired_size)
    s.insert(rand() % cap);
}

MutantDatabase::MutantDatabase(clang::CompilerInstance *comp_inst, 
               std::string input_filename, std::string output_dir, int limit)
: comp_inst_(comp_inst), input_filename_(input_filename),
output_dir_(output_dir), next_mutantfile_id_(1), num_mutant_limit_(limit),
src_mgr_(comp_inst->getSourceManager()), lang_opts_(comp_inst->getLangOpts())
{
  // set database filename with output directory prepended
  database_filename_ = output_dir;
  database_filename_.append(input_filename_, 0, input_filename_.length()-2);
  database_filename_ += "_mut_db.csv";
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

    // Similar for column
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

      // Similar for mutant name
      if (mutantname_map_iter == mutant_entry_table_[line_num][col_num].end())
      {
        mutant_entry_table_[line_num][col_num][name] = MutantEntryList();
        mutant_entry_table_[line_num][col_num][name].push_back(new_entry);
      }
      else
      {
        // Check if there is another mutant with same replacement inside.
        // If yes, then skip. Otherwise push new entry in.

        // All these entries will have the same line and column number and
        // mutant name, so if they make the same replacement then they are
        // duplicated.

        // ISSUE: is it possible for duplicate case between different mutation 
        // operator?
        for (auto entry: mutant_entry_table_[line_num][col_num][name])
        {
          if (new_entry == entry)
            return;
        }

        mutant_entry_table_[line_num][col_num][name].push_back(new_entry);
      }
    }
  }
}

void MutantDatabase::WriteEntryToDatabaseFile(
    string mutant_name, const MutantEntry &entry)
{
  // cout << "making " << mutant_name << " mutant\n" << entry << endl;

  // Open mutattion database file in APPEND mode
  ofstream mutant_db_file(database_filename_.data(), ios::app);

  // write input file name
  // mutant_db_file << input_filename_ << ",";

  // write mutant file name
  mutant_db_file << GetNextMutantFilename() << ","; 

  // write name of operator  
  mutant_db_file << mutant_name << ",";

  // write information about token BEFORE mutation
  mutant_db_file << entry.getProteumStyleLineNum() << ",";
  mutant_db_file << GetLineNumber(src_mgr_, entry.getStartLocation()) << ",";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getStartLocation()) << ",";
  mutant_db_file << GetLineNumber(src_mgr_, entry.getTokenEndLocation()) << ",";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getTokenEndLocation()) << ",";
  mutant_db_file << entry.getToken() << ",";

  // write information about token AFTER mutation
  mutant_db_file << GetLineNumber(src_mgr_, entry.getStartLocation()) << ",";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getStartLocation()) << ",";
  mutant_db_file << GetLineNumber(src_mgr_, entry.getMutatedTokenEndLocation()) << ",";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getMutatedTokenEndLocation()) << ",";
  mutant_db_file << entry.getMutatedToken() << endl;

  // close database file
  mutant_db_file.close(); 
}

void MutantDatabase::WriteAllEntriesToDatabaseFile()
{
  long count = 0;

  for (auto line_map_iter: mutant_entry_table_)
    for (auto column_map_iter: line_map_iter.second)
      for (auto mutantname_map_iter: column_map_iter.second)
        for (auto entry: mutantname_map_iter.second)
        {
          count++;
          WriteEntryToDatabaseFile(mutantname_map_iter.first, entry);
          IncrementNextMutantfileId();
        }

  cout << "wrote " << count << " mutants to db file\n";
}

void MutantDatabase::WriteEntryToMutantFile(const MutantEntry &entry)
{
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr_, lang_opts_);

  SourceLocation start_loc = src_mgr_.getExpansionLoc(entry.getStartLocation());
  SourceLocation end_loc = src_mgr_.getExpansionLoc(entry.getTokenEndLocation());

  int length = src_mgr_.getFileOffset(end_loc) - \
               src_mgr_.getFileOffset(start_loc);

  // cout << "length = " << length << endl;
  
  rewriter.ReplaceText(start_loc, length, entry.getMutatedToken());

  // cout << rewriter.getRewrittenText(SourceRange(entry.getStartLocation(), entry.getTokenEndLocation())) << endl;
  // cout << entry.getStartLocation().printToString(src_mgr_) << endl;


  string mutant_filename{output_dir_};
  mutant_filename += GetNextMutantFilename();

  // Make and write mutated code to output file.
  // cout << "cp 1\n";
  const RewriteBuffer *RewriteBuf = rewriter.getRewriteBufferFor(
      src_mgr_.getMainFileID());
  // cout << "cp 2\n";
  ofstream output(mutant_filename.data());
  // cout << "cp 3\n";
  output << string(RewriteBuf->begin(), RewriteBuf->end());
  // cout << "cp 4\n";
  output.close(); 

  // cout << "done\n";
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

bool CompareEntry(long i, long j)
{
  return i < j;
}

// generate mutant file and write to database file
void MutantDatabase::ExportAllEntries()
{
  map<string, int> mutant_count;
  set<string> all_mutant_operators{
      "SSDL", "OCNG", "ORRN", "VTWF", "CRCR", "SANL", "SRWS", "SCSR", 
      "VLSF", "VGSF", "VLTF", "VGTF", "VLPF", "VGPF", "VGSR", "VLSR", 
      "VGAR", "VLAR", "VGTR", "VLTR", "VGPR", "VLPR", "VTWD", "VSCR", 
      "CGCR", "CLCR", "CGSR", "CLSR", "OPPO", "OMMO", "OLNG", "OBNG", 
      "OIPM", "OCOR", "OLLN", "OSSN", "OBBN", "OLRN", "ORLN", "OBLN", 
      "OBRN", "OSLN", "OSRN", "OBAN", "OBSN", "OSAN", "OSBN", "OAEA", 
      "OBAA", "OBBA", "OBEA", "OBSA", "OSAA", "OSBA", "OSEA", "OSSA", 
      "OEAA", "OEBA", "OESA", "OAAA", "OABA", "OASA", "OALN", "OAAN", 
      "OARN", "OABN", "OASN", "OLAN", "ORAN", "OLBN", "OLSN", "ORSN", 
      "ORBN"};
      
  for (auto e: all_mutant_operators)
    mutant_count[e] = 0;


  for (auto line_map_iter: mutant_entry_table_)
    for (auto column_map_iter: line_map_iter.second)
      for (auto mutantname_map_iter: column_map_iter.second)
      {
        // Generate all mutants of this mutation operator at this mutation 
        // point if number of to-be-generated mutants is <= given limit.
        if (mutantname_map_iter.second.size() <= num_mutant_limit_)
        {
          for (auto entry: mutantname_map_iter.second)
          {
            WriteEntryToDatabaseFile(mutantname_map_iter.first, entry);
            WriteEntryToMutantFile(entry);
            IncrementNextMutantfileId();

            mutant_count[mutantname_map_iter.first] += 1;
          }
        }
        // Otherwise, randomly generate LIMIT number of mutants.
        else
        {
          // Generate a list of LIMIT distinct numbers/indexes, each less than  
          // number of supposed-to-be-generated mutants.
          // Generate mutants from mutant entries at those indexes.

          set<int> random_nums;
          GenerateRandomNumbers(random_nums, num_mutant_limit_, 
                                mutantname_map_iter.second.size());

          for (auto idx: random_nums)
          {
            WriteEntryToDatabaseFile(mutantname_map_iter.first, 
                                     mutantname_map_iter.second[idx]);
            WriteEntryToMutantFile(mutantname_map_iter.second[idx]);
            IncrementNextMutantfileId();

            mutant_count[mutantname_map_iter.first] += 1;
          }
        } 
      }

  for (auto it: mutant_count)
    cout << it.first << " " << it.second << endl;
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
  mutant_filename += ".c";
  return mutant_filename;
}

void MutantDatabase::IncrementNextMutantfileId()
{
  next_mutantfile_id_++;
}