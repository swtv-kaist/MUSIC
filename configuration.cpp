#include "configuration.h"

Configuration::Configuration(std::string inputfile_name, std::string mutation_db_filename, 
										 clang::SourceLocation start_loc, clang::SourceLocation end_loc, 
										 std::string directory, int limit)
  :inputfile_name_(inputfile_name), mutant_database_filename_(mutation_db_filename), 
  mutation_range_start_loc_(start_loc), mutation_range_end_loc_(end_loc), 
  output_directory_(directory), limit_num_of_mutant_(limit) 
{ 
} 

// Accessors for each member variable.
std::string Configuration::getInputFilename() 
{
	return inputfile_name_;
}

std::string Configuration::getMutationDbFilename() 
{
	return mutant_database_filename_;
}
clang::SourceLocation* Configuration::getStartOfMutationRange() 
{
	return &mutation_range_start_loc_;
}

clang::SourceLocation* Configuration::getEndOfMutationRange() 
{
	return &mutation_range_end_loc_;
}

std::string Configuration::getOutputDir() 
{
	return output_directory_;
}

int Configuration::getLimitNumOfMutants() 
{
	return limit_num_of_mutant_;
}

// std::vector<StmtMutantOperator*>& Configuration::getStmtOperatorList()
// {
//   return stmt_mutant_operator_list_;
// }

// std::vector<ExprMutantOperator*>& Configuration::getExprOperatorList()
// {
//   return expr_mutant_operator_list_;
// }
