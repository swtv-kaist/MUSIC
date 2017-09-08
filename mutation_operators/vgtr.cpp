#include "../comut_utility.h"
#include "vgtr.h"

bool VGTR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VGTR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VGTR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsStructReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);
	StmtContext &stmt_context = context->getStmtContext();

	// VGTR can mutate this expression only if it is struct type
	// inside mutation range and NOT inside array decl size or enum declaration
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInEnumDecl();
}



void VGTR::Mutate(clang::Expr *e, ComutContext *context)
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

  string struct_type = getStructureType(e->getType());

  for (auto vardecl: *(context->getSymbolTable()->getGlobalStructVarDeclList()))
  {
  	if (!(vardecl->getLocStart() < start_loc))
  		break; 
  	
  	if (skip_const_vardecl && IsVarDeclConst(vardecl)) 
      continue;   

    if (skip_float_vardecl && IsVarDeclFloating(vardecl))
      continue;

    string mutated_token{GetVarDeclName(vardecl)};

    if (token.compare(mutated_token) != 0 &&
        struct_type.compare(getStructureType(vardecl->getType())) == 0)
    {
    	context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
    }
  }
}


