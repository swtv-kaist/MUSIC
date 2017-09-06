#include "../comut_utility.h"
#include "vgar.h"

bool VGAR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VGAR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VGAR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsArrayReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);
	StmtContext &stmt_context = context->getStmtContext();

	// VGAR can mutate this expression only if it is array type
	// inside mutation range and NOT inside array decl size or enum declaration
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInEnumDecl();
}

string newGetArrayTypeFunction(QualType type)
{
	return cast<ArrayType>(
  		type.getCanonicalType().getTypePtr())->getElementType().getCanonicalType().getAsString();
}

void VGAR::Mutate(clang::Expr *e, ComutContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(e)};
	StmtContext &stmt_context = context->getStmtContext();

	// cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = stmt_context.IsInSwitchStmtConditionRange(e);

  // cannot mutate a variable in lhs of assignment to a const variable
  bool skip_const_vardecl = stmt_context.IsInLhsOfAssignmentRange(e);

  for (auto vardecl: *(context->getSymbolTable()->getGlobalArrayVarDeclList()))
  {
  	if (!(vardecl->getLocStart() < start_loc))
  		break; 

  	if (skip_const_vardecl && IsVarDeclConst(vardecl)) 
      continue;   

    if (skip_float_vardecl && IsVarDeclFloating(vardecl))
      continue;

    string mutated_token{GetVarDeclName(vardecl)};

    if (token.compare(mutated_token) != 0 && 
        sameArrayElementType(e->getType(), vardecl->getType()))
    {
    	GenerateMutantFile(context, start_loc, end_loc, mutated_token);
			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																		token, mutated_token);
    }
  }
}


