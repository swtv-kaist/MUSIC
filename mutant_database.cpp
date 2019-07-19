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

  size_t last_dot_pos = input_filename_.find_last_of(".");
  database_filename_ += input_filename_.substr(0, last_dot_pos);

  // database_filename_.append(input_filename_, 0, input_filename_.length()-2);
  database_filename_ += "_mut_db.csv";

  rewriter_.setSourceMgr(src_mgr_, comp_inst->getLangOpts());
}

void MutantDatabase::AddMutantEntry(StmtContext& stmt_context, 
                    MutantName name, clang::SourceLocation start_loc,
                    clang::SourceLocation end_loc, std::string token,
                    std::string mutated_token, int proteum_style_line_num,
                    std::string additional_info, 
                    std::vector<std::string> extra_tokens,
                    std::vector<std::string> extra_mutated_tokens,
                    std::vector<clang::SourceLocation> extra_start_locs,
                    std::vector<clang::SourceLocation> extra_end_locs,
                    std::vector<std::string> additional_infos)
{
  // cout << "AddMutantEntry\n";
  int line_num = GetLineNumber(src_mgr_, start_loc);
  int col_num = GetColumnNumber(src_mgr_, start_loc);

  extra_tokens.insert(extra_tokens.begin(), token);
  extra_mutated_tokens.insert(extra_mutated_tokens.begin(), mutated_token);
  extra_start_locs.insert(extra_start_locs.begin(), start_loc);
  extra_end_locs.insert(extra_end_locs.begin(), end_loc);
  additional_infos.insert(additional_infos.begin(), additional_info);

  bool show_function_name = true;

  for (int i = 0; i < extra_start_locs.size(); ++i)
  {
    if (!ValidateSourceRange(extra_start_locs[i], extra_end_locs[i]))
      return;
  }

  // cout << "cp1\n";
  if (show_function_name)
    additional_infos.push_back(stmt_context.getContainingFunction(start_loc, src_mgr_));
  // cout << "cp2\n";

  MutantEntry new_entry(extra_tokens, extra_mutated_tokens, additional_infos, extra_start_locs, 
                        extra_end_locs, src_mgr_, proteum_style_line_num);

  // cout << "cp3\n";
  string temp = "MUSIC_global";
  /*if (line_num == 315 and temp.compare(stmt_context.getContainingFunction(start_loc, src_mgr_)) == 0)
  {
    cout << name << endl;
    cout << new_entry << endl;

    cout << start_loc.printToString(src_mgr_) << endl;
    cout << stmt_context.currently_parsed_function_range_->getBegin().printToString(src_mgr_) << endl;
    cout << stmt_context.currently_parsed_function_range_->getEnd().printToString(src_mgr_) << endl;

    BeforeThanCompare<SourceLocation> isBefore(src_mgr_);

    cout << isBefore(start_loc, stmt_context.currently_parsed_function_range_->getBegin()) << endl;
    cout << isBefore(start_loc, stmt_context.currently_parsed_function_range_->getEnd()) << endl;

    int i;
    cin >> i;
  }*/    

  for (int i = 0; i < extra_start_locs.size(); i++)
    if (extra_start_locs[i].isInvalid() || extra_end_locs[i].isInvalid())
    {
      cout << "MutantDatabase::AddMutantEntry Warning: fail to add mutant ";
      cout << "due to invalid SourceLocation\n";
      cout << extra_start_locs[i].printToString(src_mgr_) << endl;
      cout << extra_end_locs[i].printToString(src_mgr_) << endl;
      cout << name << endl;
      cout << new_entry << endl;
      return;
    }

  // cout << "cp4\n";

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
      // SOLVING ISSUE: is it possible for duplicate case between different 
      // mutation operators?
      // for (auto entry1: mutant_entry_table_[line_num][col_num]) 
      //   for (auto entry2: entry1.second)
      //     if (new_entry == entry2)
      //       return; 

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
          {
            return;
          }
        }

        mutant_entry_table_[line_num][col_num][name].push_back(new_entry);
      }
    }
  }

  // cout << "cp5\n";
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

  // TODO1: Implement an option which controls whether or not additional info
  // should be included in the mutant database.
  // TODO2: Some option does not have additional info yet (i.e. constant and 
  // variable mutation operators)

  // write information about token BEFORE mutation
  // mutant_db_file << entry.getProteumStyleLineNum() << ",";
  mutant_db_file << GetLineNumber(src_mgr_, entry.getStartLocation()[0]) << ",";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getStartLocation()[0]) << ",";
  mutant_db_file << GetLineNumber(src_mgr_, entry.getTokenEndLocation()[0]) << ",";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getTokenEndLocation()[0]) << ",";

  string token = entry.getToken()[0];
  size_t pos_comma = token.find(",");
  size_t pos_newline = token.find("\n");
  size_t len = 0;

  if (pos_comma == string::npos && pos_newline != string::npos)
    len = pos_newline;
  else if (pos_comma != string::npos && pos_newline == string::npos)
    len = pos_comma;
  else if (pos_comma != string::npos && pos_newline != string::npos)
    len = min(pos_newline, pos_comma);
  
  if (len != 0)
  {
    token = token.substr(0, len);
    if (token[0] == '\"')
      token.append("\"");
  }

  mutant_db_file << token << ",";

  // write information about token AFTER mutation
  mutant_db_file << GetLineNumber(src_mgr_, entry.getStartLocation()[0]) << ",";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getStartLocation()[0]) << ",";
  mutant_db_file << GetLineNumber(src_mgr_, entry.getMutatedTokenEndLocation()[0]) << ",";
  mutant_db_file << GetColumnNumber(src_mgr_, entry.getMutatedTokenEndLocation()[0]) << ",";

  token = entry.getMutatedToken()[0];
  pos_comma = token.find(",");
  pos_newline = token.find("\n");
  len = 0;

  if (pos_comma == string::npos && pos_newline != string::npos)
    len = pos_newline;
  else if (pos_comma != string::npos && pos_newline == string::npos)
    len = pos_comma;
  else if (pos_comma != string::npos && pos_newline != string::npos)
    len = min(pos_newline, pos_comma);
  
  if (len != 0)
  {
    token = token.substr(0, len);
    if (token[0] == '\"')
      token.append("\"");
  }

  mutant_db_file << token << ',';

  vector<string> additional_infos = entry.getAdditionalInfo();
  // write additional info if user wants
  for (int i = 0; i < additional_infos.size()-1; i++)
    mutant_db_file << additional_infos[i] << ",";
  mutant_db_file << additional_infos[additional_infos.size()-1] << endl;

  // close database file
  mutant_db_file.close(); 
}

void MutantDatabase::WriteAllEntriesToDatabaseFile()
{
  long count = 0;
  map<string, int> mutant_count;
  set<string> all_mutant_operators{
      "SSDL", "OCNG", "ORRN", /*"VTWF",*/ "CRCR", /*"SANL", "SRWS", "SCSR", 
      "VLSF", "VGSF", "VLTF", "VGTF", "VLPF", "VGPF",*/ "VGSR", "VLSR", 
      "VGAR", "VLAR", "VGTR", "VLTR", "VGPR", "VLPR", "VTWD", "VSCR", 
      "CGCR", "CLCR", "CGSR", "CLSR", "OPPO", "OMMO", "OLNG", "OBNG", 
      "OIPM", "OCOR", "OLLN", "OSSN", "OBBN", "OLRN", "ORLN", "OBLN", 
      "OBRN", "OSLN", "OSRN", "OBAN", "OBSN", "OSAN", "OSBN", "OAEA", 
      "OBAA", "OBBA", "OBEA", "OBSA", "OSAA", "OSBA", "OSEA", "OSSA", 
      "OEAA", "OEBA", "OESA", "OAAA", "OABA", "OASA", "OALN", "OAAN", 
      "OARN", "OABN", "OASN", "OLAN", "ORAN", "OLBN", "OLSN", "ORSN", 
      "ORBN"/*, "RGCR", "RLCR", "RGSR", "RLSR", "RGPR", "RLPR", "RGTR",
      "RLTR", "RGAR", "RLAR", "RRCR"*/, "DirVarAriNeg", "DirVarBitNeg", 
      "DirVarLogNeg", "DirVarIncDec", "DirVarRepReq", "DirVarRepCon", 
      "DirVarRepPar", "DirVarRepGlo", "DirVarRepExt", "DirVarRepLoc",
      "IndVarAriNeg", "IndVarBitNeg", 
      "IndVarLogNeg", "IndVarIncDec", "IndVarRepReq", "IndVarRepCon", 
      "IndVarRepPar", "IndVarRepGlo", "IndVarRepExt", "IndVarRepLoc",
      "RetStaDel", "SCRB", "SBRC", "ArgAriNeg", "ArgBitNeg", "ArgDel",
      "ArgIncDec", "ArgLogNeg", "ArgRepReq", "ArgStcAli", "ArgStcDif",
      "FunCalDel", "SWDD", "SDWD", "VASM", "SGLR", "SSOM", "SMVB", "SRSR",
      "STRP", "STRI", "SMTT", "SMTC", "SSWM", "VDTR", "SBRN", "SCRN",
      "CovAllNod", "CovAllEdg"};
      
  for (auto e: all_mutant_operators)
    mutant_count[e] = 0;

  for (auto line_map_iter: mutant_entry_table_)
    for (auto column_map_iter: line_map_iter.second)
      for (auto mutantname_map_iter: column_map_iter.second)
        for (auto entry: mutantname_map_iter.second)
        {
          count++;
          WriteEntryToDatabaseFile(mutantname_map_iter.first, entry);
          IncrementNextMutantfileId();
          mutant_count[mutantname_map_iter.first] += 1;
        }

  for (auto it: mutant_count)
    cout << it.first << " " << it.second << endl;
  cout << "wrote " << count << " mutants to db file\n";
}

void MutantDatabase::WriteEntryToMutantFile(const MutantEntry &entry)
{
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr_, lang_opts_);

  vector<SourceLocation> start_locs = entry.getStartLocation();
  vector<SourceLocation> end_locs = entry.getTokenEndLocation();
  vector<string> mutated_tokens = entry.getMutatedToken();

  for (int i = 0; i < start_locs.size(); i++)
  {
    SourceLocation start_loc = src_mgr_.getExpansionLoc(start_locs[i]);
    SourceLocation end_loc = src_mgr_.getExpansionLoc(end_locs[i]);

    int length = src_mgr_.getFileOffset(end_loc) - \
                 src_mgr_.getFileOffset(start_loc);

    // cout << "length = " << length << endl;
    
    rewriter.ReplaceText(start_loc, length, mutated_tokens[i]);
  }

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
      "SSDL", "OCNG", "ORRN", /*"VTWF",*/ "CRCR", /*"SANL", "SRWS", "SCSR", 
      "VLSF", "VGSF", "VLTF", "VGTF", "VLPF", "VGPF",*/ "VGSR", "VLSR", 
      "VGAR", "VLAR", "VGTR", "VLTR", "VGPR", "VLPR", "VTWD", "VSCR", 
      "CGCR", "CLCR", "CGSR", "CLSR", "OPPO", "OMMO", "OLNG", "OBNG", 
      "OIPM", "OCOR", "OLLN", "OSSN", "OBBN", "OLRN", "ORLN", "OBLN", 
      "OBRN", "OSLN", "OSRN", "OBAN", "OBSN", "OSAN", "OSBN", "OAEA", 
      "OBAA", "OBBA", "OBEA", "OBSA", "OSAA", "OSBA", "OSEA", "OSSA", 
      "OEAA", "OEBA", "OESA", "OAAA", "OABA", "OASA", "OALN", "OAAN", 
      "OARN", "OABN", "OASN", "OLAN", "ORAN", "OLBN", "OLSN", "ORSN", 
      "ORBN"/*, "RGCR", "RLCR", "RGSR", "RLSR", "RGPR", "RLPR", "RGTR",
      "RLTR", "RGAR", "RLAR", "RRCR"*/, "DirVarAriNeg", "DirVarBitNeg", 
      "DirVarLogNeg", "DirVarIncDec", "DirVarRepReq", "DirVarRepCon", 
      "DirVarRepPar", "DirVarRepGlo", "DirVarRepExt", "DirVarRepLoc",
      "IndVarAriNeg", "IndVarBitNeg", 
      "IndVarLogNeg", "IndVarIncDec", "IndVarRepReq", "IndVarRepCon", 
      "IndVarRepPar", "IndVarRepGlo", "IndVarRepExt", "IndVarRepLoc",
      "RetStaDel", "SCRB", "SBRC", "ArgAriNeg", "ArgBitNeg", "ArgDel",
      "ArgIncDec", "ArgLogNeg", "ArgRepReq", "ArgStcAli", "ArgStcDif",
      "FunCalDel", "SWDD", "SDWD", "VASM", "SGLR", "SSOM", "SMVB", "SRSR",
      "STRP", "STRI", "SMTT", "SMTC", "SSWM", "VDTR", "SBRN", "SCRN",
      "CovAllNod", "CovAllEdg"};
      
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

  size_t last_dot_pos = input_filename_.find_last_of(".");
  // database_filename_ += input_filename_.substr(0, last_dot_pos);

  mutant_filename = input_filename_.substr(0, last_dot_pos);
  // mutant_filename.assign(input_filename_, 0, input_filename_.length()-2);

  mutant_filename += ".MUT";
  mutant_filename += to_string(next_mutantfile_id_);
  mutant_filename += input_filename_.substr(last_dot_pos);
  // mutant_filename += ".c";
  return mutant_filename;
}

void MutantDatabase::IncrementNextMutantfileId()
{
  next_mutantfile_id_++;
}

bool MutantDatabase::ValidateSourceRange(SourceLocation &start_loc, SourceLocation &end_loc)
{
  // I forget the reason why ...
  // if (start_loc.isMacroID() && end_loc.isMacroID())
  //   return false;
  // cout << "checking\n";
  // PrintLocation(src_mgr_, start_loc);
  // PrintLocation(src_mgr_, end_loc);

  if (start_loc.isInvalid() || end_loc.isInvalid())
    return false;

  // if (!src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) &&
  //     !src_mgr_.isInFileID(end_loc, src_mgr_.getMainFileID()))
  //   return false;

  // cout << "MusicASTVisitor cp1\n";
  // cout << start_loc.printToString(src_mgr_) << endl;
  // cout << end_loc.printToString(src_mgr_) << endl;
  // cout << src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) << endl;
  // cout << src_mgr_.isInFileID(end_loc, src_mgr_.getMainFileID()) << endl;

  // Skip nodes that are not in the to-be-mutated file.
  if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(start_loc).getHashValue() &&
      src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(end_loc).getHashValue())
    return false;

  // cout << "cp2\n";

  if (start_loc.isMacroID())
  {
    /* THIS PART WORKS FOR CLANG 6.0.1 */
    // pair<SourceLocation, SourceLocation> expansion_range = 
    //     rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
    // start_loc = expansion_range.first;
    /*=================================*/

    /* THIS PART WORKS FOR CLANG 7.0.1 */
    CharSourceRange expansion_range = 
        rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
    start_loc = expansion_range.getBegin();
    /*=================================*/
  }

  // cout << "cp2\n";

  if (end_loc.isMacroID())
  {
    /* THIS PART WORKS FOR CLANG 6.0.1 */
    // pair<SourceLocation, SourceLocation> expansion_range = 
    //     rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
    // end_loc = expansion_range.second;
    /*=================================*/

    /* THIS PART WORKS FOR CLANG 7.0.1 */
    CharSourceRange expansion_range = 
        rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
    end_loc = expansion_range.getEnd();
    /*=================================*/

    end_loc = Lexer::getLocForEndOfToken(
        src_mgr_.getExpansionLoc(end_loc), 0, src_mgr_, comp_inst_->getLangOpts());

    // Skipping whitespaces (if any)
    while (*(src_mgr_.getCharacterData(end_loc)) == ' ')
      end_loc = end_loc.getLocWithOffset(1);

    // Return if macro is variable-typed. 
    if (*(src_mgr_.getCharacterData(end_loc)) != '(')
      return true;

    // Find the closing bracket of this function-typed macro
    int parenthesis_counter = 1;
    end_loc = end_loc.getLocWithOffset(1);

    while (parenthesis_counter != 0)
    {
      if (*(src_mgr_.getCharacterData(end_loc)) == '(')
        parenthesis_counter++;

      if (*(src_mgr_.getCharacterData(end_loc)) == ')')
        parenthesis_counter--;

      end_loc = end_loc.getLocWithOffset(1);
    }
  }
  // cout << "cp3\n";

  return true;
}
