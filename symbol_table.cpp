#include "symbol_table.h"

SymbolTable::SymbolTable(
		GlobalScalarConstantList *g_scalarconstant_list,
    LocalScalarConstantList *l_scalarconstant_list,
    GlobalStringLiteralList *g_stringliteral_list,
    LocalStringLiteralList *l_stringliteral_list,
    VarDeclList *g_scalar_vardecl_list,
	  std::vector<VarDeclList> *l_scalar_vardecl_list,
		VarDeclList *g_array_vardecl_list,
	  std::vector<VarDeclList> *l_array_vardecl_list,
		VarDeclList *g_struct_vardecl_list,
	  std::vector<VarDeclList> *l_struct_vardecl_list,
		VarDeclList *g_pointer_vardecl_list,
	  std::vector<VarDeclList> *l_pointer_vardecl_list,
	  FunctionParamList *func_param_list,
    FunctionUsedGlobalList *func_used_global_list,
    FunctionNotUsedGlobalList *func_not_used_global_list,
    FunctionLocalList *func_local_list,
    LineToVarsMap *line_to_vars_map,
    LineToConstsMap *line_to_consts_map,
    std::vector<LabelList> *label_list,
    std::vector<ReturnStmtList> *return_stmt_list)
	:global_scalarconstant_list_(g_scalarconstant_list),
	local_scalarconstant_list_(l_scalarconstant_list),
	global_stringliteral_list_(g_stringliteral_list),
	local_stringliteral_list_(l_stringliteral_list),
	global_scalar_vardecl_list_(g_scalar_vardecl_list),
	local_scalar_vardecl_list_(l_scalar_vardecl_list),
	global_array_vardecl_list_(g_array_vardecl_list),
	local_array_vardecl_list_(l_array_vardecl_list),
	global_struct_vardecl_list_(g_struct_vardecl_list),
	local_struct_vardecl_list_(l_struct_vardecl_list),
	global_pointer_vardecl_list_(g_pointer_vardecl_list),
	local_pointer_vardecl_list_(l_pointer_vardecl_list),
	func_param_list_(func_param_list),
	func_used_global_list_(func_used_global_list),
	func_not_used_global_list_(func_not_used_global_list),
	func_local_list_(func_local_list), 
	line_to_vars_map_(line_to_vars_map),
	line_to_consts_map_(line_to_consts_map),
	label_list_(label_list),
	return_stmt_list_(return_stmt_list)
{}


GlobalScalarConstantList* SymbolTable::getGlobalScalarConstantList()
{
	return global_scalarconstant_list_;
}

LocalScalarConstantList* SymbolTable::getLocalScalarConstantList()
{
	return local_scalarconstant_list_;
}

GlobalStringLiteralList* SymbolTable::getGlobalStringLiteralList()
{
	return global_stringliteral_list_;
}

LocalStringLiteralList* SymbolTable::getLocalStringLiteralList()
{
	return local_stringliteral_list_;
}

VarDeclList* SymbolTable::getGlobalScalarVarDeclList()
{
	return global_scalar_vardecl_list_;
}

std::vector<VarDeclList>* SymbolTable::getLocalScalarVarDeclList()
{
	return local_scalar_vardecl_list_;
}

VarDeclList* SymbolTable::getGlobalArrayVarDeclList()
{
	return global_array_vardecl_list_;
}

std::vector<VarDeclList>* SymbolTable::getLocalArrayVarDeclList()
{
	return local_array_vardecl_list_;
}

VarDeclList* SymbolTable::getGlobalStructVarDeclList()
{
	return global_struct_vardecl_list_;
}

std::vector<VarDeclList>* SymbolTable::getLocalStructVarDeclList()
{
	return local_struct_vardecl_list_;
}

VarDeclList* SymbolTable::getGlobalPointerVarDeclList()
{
	return global_pointer_vardecl_list_;
}

std::vector<VarDeclList>* SymbolTable::getLocalPointerVarDeclList()
{
	return local_pointer_vardecl_list_;
}

FunctionParamList* SymbolTable::getFuncParamList()
{
	return func_param_list_;
}

FunctionUsedGlobalList* SymbolTable::getFuncUsedGlobalList()
{
	return func_used_global_list_;
}

FunctionNotUsedGlobalList* SymbolTable::getFuncNotUsedGlobalList()
{
	return func_not_used_global_list_;
}

FunctionLocalList* SymbolTable::getFuncLocalList()
{
	return func_local_list_;
}

LineToVarsMap* SymbolTable::getLineToVarMap()
{
	return line_to_vars_map_;
}

LineToConstsMap* SymbolTable::getLineToConstsMap()
{
	return line_to_consts_map_;
}

std::vector<LabelList>* SymbolTable::getLabelList()
{
	return label_list_;
}

std::vector<ReturnStmtList>* SymbolTable::getReturnStmtList()
{
	return return_stmt_list_;
}
