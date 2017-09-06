#include "../comut_utility.h"
#include "vltr.h"

bool VLTR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VLTR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VLTR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (!ExprIsStructReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);
  StmtContext &stmt_context = context->getStmtContext();

	// VLTR can mutate this expression only if it is struct type,
	// inside mutation range and NOT inside array decl size or enum declaration
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         !stmt_context.IsInArrayDeclSize() &&
         !stmt_context.IsInEnumDecl() &&
         stmt_context.IsInCurrentlyParsedFunctionRange(e);
}



void VLTR::Mutate(clang::Expr *e, ComutContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst->getLangOpts());

	string token{rewriter.ConvertToString(e)};

	// get all variable declaration that VLSR can mutate this expr to.
  VarDeclList range(
      (*(context->getSymbolTable()->getLocalStructVarDeclList()))[context->getFunctionId()]);

  GetRange(e, context, &range);

  string struct_type = getStructureType(e->getType());

  for (auto vardecl: range)
  {
    string mutated_token{GetVarDeclName(vardecl)};

    if (struct_type.compare(getStructureType(vardecl->getType())) == 0)
    {
      GenerateMutantFile(context, start_loc, end_loc, mutated_token);
      WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
                                    token, mutated_token);
    }
  }
}



void VLTR::GetRange(Expr *e, ComutContext *context, VarDeclList *range)
{
  SourceLocation start_loc = e->getLocStart();
  Rewriter rewriter;
  rewriter.setSourceMgr(context->comp_inst->getSourceManager(), 
                        context->comp_inst->getLangOpts());

  string token{rewriter.ConvertToString(e)};
  StmtContext &stmt_context = context->getStmtContext();

  // cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = stmt_context.IsInSwitchStmtConditionRange(e);

  // cannot mutate a variable in lhs of assignment to a const variable
  bool skip_const_vardecl = stmt_context.IsInLhsOfAssignmentRange(e);

  bool skip_register_vardecl = stmt_context.IsInAddressOpRange(e);

  // remove all vardecl appear after expr
  for (auto it = range->begin(); it != range->end(); )
  {
    if (!((*it)->getLocStart() < start_loc))
    {
      range->erase(it, range->end());
      break;
    }

    if ((skip_const_vardecl && IsVarDeclConst((*it))) ||
        (skip_float_vardecl && IsVarDeclFloating((*it))) ||
        (skip_register_vardecl && 
          (*it)->getStorageClass() == SC_Register) ||
        (token.compare(GetVarDeclName(*it)) == 0))
    {
      it = range->erase(it);
      continue;
    }

    ++it;
  }

  for (auto scope: *(context->scope_list_))
  {
    // all vardecl after expr are removed.
    // Hence no need to consider scopes after expr as well.
    if (LocationBeforeRangeStart(start_loc, scope))
      break;

    if (!LocationIsInRange(start_loc, scope))
      for (auto it = range->begin(); it != range->end();)
      {
        if (LocationAfterRangeEnd((*it)->getLocStart(), scope))
          break;

        if (LocationIsInRange((*it)->getLocStart(), scope))
        {
          it = range->erase(it);
          continue;
        }

        ++it;
      }
  }
}