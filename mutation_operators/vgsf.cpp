#include "../music_utility.h"
#include "vgsf.h"

/* The domain for VGSF must be names of functions whose
   function calls will be mutated. */
bool VGSF::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool VGSF::ValidateRange(const std::set<std::string> &range)
{
	for (auto e: range)
    if (!IsValidVariableName(e))
      return false;

  return true;
}

// Return True if the mutant operator can mutate this expression
bool VGSF::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (CallExpr *ce = dyn_cast<CallExpr>(e))
	{
		SourceLocation start_loc = ce->getLocStart();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    FunctionDecl *fd = ce->getDirectCallee();
    if (!fd)
      return false;

    if (!domain_.empty() && 
        !IsStringElementOfSet(fd->getNameAsString(), domain_))
      return false;

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is scalar type.
		return (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
						!context->getStmtContext().IsInEnumDecl() &&
						ExprIsScalar(e));
	}

	return false;
}

void VGSF::Mutate(clang::Expr *e, MusicContext *context)
{
	CallExpr *ce;
	if (!(ce = dyn_cast<CallExpr>(e)))
		return;

	SourceLocation start_loc = ce->getLocStart();

  // getRParenLoc returns the location before the right parenthesis
  SourceLocation end_loc = ce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

	// cannot mutate variable in switch condition to a floating-type variable
  bool skip_float_vardecl = \
      context->getStmtContext().IsInSwitchStmtConditionRange(e);

  for (auto vardecl: *(context->getSymbolTable()->getGlobalScalarVarDeclList()))
  {
  	if (!(vardecl->getLocStart() < start_loc))
  		break;

    string mutated_token{GetVarDeclName(vardecl)};

    if (!range_.empty() && !IsStringElementOfSet(mutated_token, range_))
      continue;
  	
  	if (skip_float_vardecl && IsVarDeclFloating(vardecl))
      continue;

    if (token.compare(mutated_token) != 0)
    {
    	context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
    }
  }
}


