#include "comut_utility.h"
#include "comut_context.h"

ComutContext::ComutContext(
    clang::CompilerInstance *CI, Configuration *config,
    LabelStmtToGotoStmtListMap *label_map, 
    SymbolTable *symbol_table)
  : comp_inst(CI), userinput(config),
    label_to_gotolist_map(label_map), function_id_(-1),
    next_mutantfile_id(1), 
    symbol_table_(symbol_table), stmt_context_(CI)
{
	std::string input_filename{config->getInputFilename()};
	mutant_filename.assign(input_filename, 0, input_filename.length()-2);
	mutant_filename += ".MUT";
}

bool ComutContext::IsRangeInMutationRange(clang::SourceRange range)
{
	SourceRange mutation_range(*(userinput->getStartOfMutationRange()),
														 *(userinput->getEndOfMutationRange()));

	return Range1IsPartOfRange2(range, mutation_range);
}

int ComutContext::getFunctionId()
{
  return function_id_;
}

SymbolTable* ComutContext::getSymbolTable()
{
	return symbol_table_;
}

StmtContext& ComutContext::getStmtContext()
{
	return stmt_context_;
}

Configuration* ComutContext::getConfiguration() const
{
  return userinput;
}

void ComutContext::IncrementFunctionId()
{
  function_id_++;
}