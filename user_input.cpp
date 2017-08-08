#include "user_input.h"

UserInput::UserInput(std::string inputfile_name, std::string mutation_db_filename, 
										 clang::SourceLocation start_loc, clang::SourceLocation end_loc, 
										 std::string directory, int limit)
  :inputfile_name_(inputfile_name), mutant_database_filename_(mutation_db_filename), 
  mutation_range_start_loc_(start_loc), mutation_range_end_loc_(end_loc), 
  output_directory_(directory), limit_num_of_mutant_(limit) 
{ 
} 

// Accessors for each member variable.
std::string UserInput::getInputFilename() 
{
	return inputfile_name_;
}

std::string UserInput::getMutationDbFilename() 
{
	return mutant_database_filename_;
}
clang::SourceLocation* UserInput::getStartOfMutationRange() 
{
	return &mutation_range_start_loc_;
}

clang::SourceLocation* UserInput::getEndOfMutationRange() 
{
	return &mutation_range_end_loc_;
}

std::string UserInput::getOutputDir() 
{
	return output_directory_;
}

int UserInput::getLimit() 
{
	return limit_num_of_mutant_;
}
