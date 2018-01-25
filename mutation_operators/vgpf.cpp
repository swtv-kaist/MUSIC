#include "../music_utility.h"
#include "vgpf.h"

bool VGPF::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VGPF::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VGPF::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (CallExpr *ce = dyn_cast<CallExpr>(e))
	{
		SourceLocation start_loc = ce->getLocStart();

    // getRParenLoc returns the location before the right parenthesis
    SourceLocation end_loc = ce->getRParenLoc();
    end_loc = end_loc.getLocWithOffset(1);

    // Return True if expr is in mutation range, NOT inside enum decl
    // and is pointer type.
		return (context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
						!context->getStmtContext().IsInEnumDecl() &&
						ExprIsPointer(e));
	}

	return false;
}

void VGPF::Mutate(clang::Expr *e, MusicContext *context)
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

  // type of the entity pointed to by pointer variable
  string pointee_type{
  		getPointerType(e->getType().getCanonicalType())};

  for (auto vardecl: *(context->getSymbolTable()->getGlobalPointerVarDeclList()))
  {
    if (!(vardecl->getLocStart() < start_loc))
      break;
    
    if (skip_float_vardecl && IsVarDeclFloating(vardecl))
      continue;

    string mutated_token{GetVarDeclName(vardecl)};

    if (token.compare(mutated_token) != 0 &&
        pointee_type.compare(getPointerType(vardecl->getType())) == 0)
    {
      context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, context->getStmtContext().getProteumStyleLineNum());
    }
  }
}


