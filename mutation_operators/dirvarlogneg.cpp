#include "../music_utility.h"
#include "dirvarlogneg.h"

bool DirVarLogNeg::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool DirVarLogNeg::ValidateRange(const std::set<std::string> &range)
{
  return true;
}

// Return True if the mutant operator can mutate this expression
bool DirVarLogNeg::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
  // cout << "checking " << ConvertToString(e, context->comp_inst_->getLangOpts()) << endl;
  if (!IsDirVar(e, context))
    return false;
  // cout << "cp1" << endl;
  SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
  StmtContext &stmt_context = context->getStmtContext();

  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
  bool is_in_domain = domain_.empty() ? true : 
                      IsStringElementOfSet(token, domain_);

  // DirVarLogNeg can mutate expr that are
  //    - inside mutation range
  //    - not inside enum decl
  //    - not on lhs of assignment (a+1=a -> uncompilable)
  //    - not inside unary increment/decrement/addressop
  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         !stmt_context.IsInEnumDecl() &&
         !stmt_context.IsInLhsOfAssignmentRange(e) &&
         !stmt_context.IsInAddressOpRange(e) && is_in_domain &&
         !stmt_context.IsInUnaryIncrementDecrementRange(e)/* && 
         (ExprIsScalar(e))*/;
}

void DirVarLogNeg::Mutate(clang::Expr *e, MusicContext *context)
{
  // cout << "found an interface variable " << ConvertToString(e, context->comp_inst_->getLangOpts()) << endl;
  // PrintLocation(context->comp_inst_->getSourceManager(), e->getLocStart());
  // return;

  SourceLocation start_loc = e->getLocStart();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

  Rewriter rewriter;
  rewriter.setSourceMgr(
      context->comp_inst_->getSourceManager(),
      context->comp_inst_->getLangOpts());
  string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  string mutated_token = "(!" + token + ")";

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum(), "");
}

bool DirVarLogNeg::IsDirVar(clang::Expr *e, MusicContext *context)
{
  if (!context->getStmtContext().IsInCurrentlyParsedFunctionRange(e))
    return false;
  
  // cout << "in IsDirVar" << endl;
  string token{""};
  // PrintLocation(context->comp_inst_->getSourceManager(), e->getLocStart());
  // cout << e->getStmtClassName() << endl;

  if (DeclRefExpr *dre = dyn_cast_or_null<DeclRefExpr>(e))
    if (!isa<FunctionDecl>(dre->getFoundDecl()) && !isa<EnumConstantDecl>(dre->getDecl()))
    {
      // cout << "cp 4" << endl;
      token = dre->getFoundDecl()->getNameAsString();
    }
    else
      return false;
  else if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
  {
    // cout << "cp 5" << endl;
    if (uo->getOpcode() != UO_Deref)
      return false;

    token = ConvertToString(e, context->comp_inst_->getLangOpts());
  }
  else
    return false;  // not variable reference or unary operators expressions
  
  // cout << "cp2\n";
  // cout << "token is " << token << endl;
  // either in global used set
  for (auto var: (*(context->getSymbolTable()->getFuncUsedGlobalList()))[context->getFunctionId()])
  {
    // cout << "candidate: " << var->getNameAsString() << endl;
    if (var->getNameAsString().compare(token) == 0)
      return true;
  }
  // cout << "cp3\n";
  // or param set
  for (auto var: (*(context->getSymbolTable()->getFuncParamList()))[context->getFunctionId()])
  {
    if (token.find(var->getNameAsString()) != string::npos)
      return true;
  }

  return false;
}
