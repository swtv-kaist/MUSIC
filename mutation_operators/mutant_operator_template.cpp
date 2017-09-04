#include "../comut_utility.h"
#include "mutant_operator_template.h"

#include <fstream>

void MutantOperatorTemplate::setDomain(set<string> &domain)
{
  domain_ = domain;
}

void MutantOperatorTemplate::setRange(set<string> &range)
{
  range_ = range;
}

void MutantOperatorTemplate::GenerateMutantFile(
	ComutContext *context, const SourceLocation &start_loc,
	const SourceLocation &end_loc, const string &mutated_token)
{
	Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	int length = src_mgr.getFileOffset(end_loc) - \
							 src_mgr.getFileOffset(start_loc);

	rewriter.ReplaceText(start_loc, length, mutated_token);

	string mutant_filename = GetNextMutantFilename(context);

	// Make and write mutated code to output file.
  const RewriteBuffer *RewriteBuf = rewriter.getRewriteBufferFor(
  	src_mgr.getMainFileID());
  ofstream output(mutant_filename.data());
  output << string(RewriteBuf->begin(), RewriteBuf->end());
  output.close();	

  // ===================================================
  // in original version, write to db file and increment next mutant file id
}

// Return the name of the next mutated code file
string MutantOperatorTemplate::GetNextMutantFilename(
    const ComutContext *context)
{
  string ret{context->userinput->getOutputDir()};
  ret += context->mutant_filename;
  ret += to_string(context->next_mutantfile_id);
  ret += ".c";
  return ret;
}

void MutantOperatorTemplate::WriteMutantInfoToMutantDbFile(
    ComutContext *context, const clang::SourceLocation &start_loc, 
    const clang::SourceLocation &end_loc, const std::string &token, 
    const std::string &mutated_token)
{
	// Open mutattion database file in APPEND mode (write to the end_loc of file)
  ofstream mutant_db_file(
  	(context->userinput->getMutationDbFilename()).data(), 
    ios::app);

  // write input file name
 	mutant_db_file << context->userinput->getInputFilename() << "\t";

 	// write output file name
  mutant_db_file << context->mutant_filename << context->next_mutantfile_id << "\t"; 

  // write name of operator  
  mutant_db_file << name_ << "\t"; 

  mutant_db_file << context->proteumstyle_stmt_start_line_num << "\t";

  SourceManager &src_mgr = context->comp_inst->getSourceManager();
  SourceLocation new_end_loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(),
      GetLineNumber(src_mgr, start_loc),
      GetColumnNumber(src_mgr, start_loc) + \
      CountNonNewlineChar(mutated_token));

  // write information about token BEFORE mutation
  mutant_db_file << GetLineNumber(src_mgr, start_loc) << "\t";
  mutant_db_file << GetColumnNumber(src_mgr, start_loc) << "\t";
  mutant_db_file << GetLineNumber(src_mgr, end_loc) << "\t";
  mutant_db_file << GetColumnNumber(src_mgr, end_loc) << "\t";
  mutant_db_file << token << "\t";

  // write information about token AFTER mutation
  mutant_db_file << GetLineNumber(src_mgr, start_loc) << "\t";
  mutant_db_file << GetColumnNumber(src_mgr, start_loc) << "\t";
  mutant_db_file << GetLineNumber(src_mgr, new_end_loc) << "\t";
  mutant_db_file << GetColumnNumber(src_mgr, new_end_loc) << "\t";
  mutant_db_file << mutated_token << endl;

  mutant_db_file.close(); 

  ++context->next_mutantfile_id;
}