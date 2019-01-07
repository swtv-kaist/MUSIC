#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <string>
#include <vector>
#include "clang/Basic/SourceLocation.h"

/**
  Contain the interpreted user input based on command option(s).

  @param  inputfile_name_ name of input file
		      mutant_database_filename_ name of mutation database file
		      mutation_range_start_loc_ start_loc location of mutation range
		      mutation_range_end_loc_ end_loc location of mutation range
		      output_directory_ output directory of generated files
		      limit_num_of_mutant_ max number of mutants per mutation point 
		      										 per mutant operator
*/
class Configuration
{
private:
	std::string inputfile_name_;
  std::string mutant_database_filename_;
  clang::SourceLocation mutation_range_start_loc_;
  clang::SourceLocation mutation_range_end_loc_;
  std::string output_directory_;
  int limit_num_of_mutant_;
  std::vector<int> excluded_lines_;

public:
  Configuration(std::string inputfile_name, std::string mutation_db_filename, 
  					    clang::SourceLocation start_loc, clang::SourceLocation end_loc, 
  					    std::vector<int>& excluded_lines, std::string directory = "./", 
                int limit = 0);

  // Getters
  std::string getInputFilename();
  std::string getMutationDbFilename();
	clang::SourceLocation* getStartOfMutationRange();
	clang::SourceLocation* getEndOfMutationRange();
	std::string getOutputDir();
	int getLimitNumOfMutants();
  std::vector<int>& getExcludedLines();
};

#endif	// CONFIGURATION_H_