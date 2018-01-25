#include "../music_utility.h"
#include "vgar.h"

bool VGAR::ValidateDomain(const std::set<std::string> &domain)
{
  for (auto e: domain)
    if (!IsValidVariableName(e))
      return false;

  return true;
	// return domain.empty();
}

bool VGAR::ValidateRange(const std::set<std::string> &range)
{
  for (auto e: range)
    if (!IsValidVariableName(e))
      return false;

  return true;

	// return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VGAR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (!ExprIsArrayReference(e))
		return false;

	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
	StmtContext &stmt_context = context->getStmtContext();

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

	// VGAR can mutate this expression only if it is array type
	// inside mutation range and NOT inside array decl size or enum declaration
	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInEnumDecl() && is_in_domain;
}

string newGetArrayTypeFunction(QualType type)
{
	return cast<ArrayType>(
  		type.getCanonicalType().getTypePtr())->getElementType().getCanonicalType().getAsString();
}

void VGAR::Mutate(clang::Expr *e, MusicContext *context)
{
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	Rewriter rewriter;
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
	StmtContext &stmt_context = context->getStmtContext();

	// cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = stmt_context.IsInSwitchStmtConditionRange(e);

  // cannot mutate a variable in lhs of assignment to a const variable
  bool skip_const_vardecl = stmt_context.IsInLhsOfAssignmentRange(e);

  for (auto vardecl: *(context->getSymbolTable()->getGlobalArrayVarDeclList()))
  {
  	if (!(vardecl->getLocStart() < start_loc))
  		break; 

    string mutated_token{GetVarDeclName(vardecl)};

    if (!range_.empty() && !IsStringElementOfSet(mutated_token, range_))
      continue;

  	if (skip_const_vardecl && IsVarDeclConst(vardecl)) 
      continue;   

    if (skip_float_vardecl && IsVarDeclFloating(vardecl))
      continue;

    if (token.compare(mutated_token) != 0 && 
        sameArrayElementType(e->getType(), vardecl->getType()))
    {
    	context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
    }
  }
}


