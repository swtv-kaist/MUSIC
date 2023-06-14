#include "../music_utility.h"
#include "ocor.h"

OCOR::OCOR(const string name)
	: ExprMutantOperator(name)
{
	integral_type_list_ = {"int", "unsigned", "short", "long", 
                         "unsigned long", "char", "unsigned char", 
                         "signed char"};

  floating_type_list_ = {"float", "double", "long double"};
}

bool OCOR::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool OCOR::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

// Return True if the mutant operator can mutate this expression
bool OCOR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (CStyleCastExpr *csce = dyn_cast<CStyleCastExpr>(e))
	{
		SourceLocation start_loc = csce->getBeginLoc();
    SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);
    const Type *type{csce->getTypeAsWritten().getCanonicalType().getTypePtr()};
    StmtContext &stmt_context = context->getStmtContext();

    string type_str{csce->getTypeAsWritten().getCanonicalType().getAsString()};
    bool is_in_domain = domain_.empty() ? true : 
                        IsStringElementOfSet(type_str, domain_);

    // OCOR mutates expression with C-style cast (Ex. (int) x)
    // These expr should be in mutation range, outside field decl
    // and the type of cast should be integral (int, char, float)
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
					 !stmt_context.IsInFieldDeclRange(e) &&
					 (type->isIntegerType() || type->isCharType() || 
            type->isFloatingType()) && is_in_domain;
	}

	return false;
}

void OCOR::Mutate(clang::Expr *e, MusicContext *context)
{
	CStyleCastExpr *csce;
	if (!(csce = dyn_cast<CStyleCastExpr>(e)))
		return;

	// Get start and end location of parenthesis surrounding the cast type
	SourceLocation start_loc = csce->getLParenLoc();
  SourceLocation end_loc = csce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  string type_str{csce->getTypeAsWritten().getCanonicalType().getAsString()};  

  // retrieve exact type written in inputfile for database record
  string token;
  SourceLocation walk = start_loc;

  while (walk != end_loc)
  {
    token += *(
    		context->comp_inst_->getSourceManager().getCharacterData(walk));
    walk = walk.getLocWithOffset(1);           
  }

  if (!range_.empty())
  {
    MutateToSpecifiedRange(type_str, token, start_loc, end_loc, context);
    return;
  }

  if (type_str.compare("unsigned int") == 0)
    type_str = "unsigned";

  MutateToIntegralType(type_str, token, start_loc, end_loc, context);

  bool is_subexpr_ptr{
  		ExprIsPointer(csce->getSubExpr()->IgnoreImpCasts())};
  StmtContext &stmt_context = context->getStmtContext();

  // Prevent mutating expr in array subscript, switch condition and case,
  // binary modulo, shift, bitwise epxr to floating-type.
  // all of pre-mentioned expr demands integral subexpr
  if (is_subexpr_ptr ||
  		stmt_context.IsInArraySubscriptRange(e) ||
      stmt_context.IsInSwitchCaseRange(e) ||
      stmt_context.IsInSwitchStmtConditionRange(e) ||
      stmt_context.IsInNonFloatingExprRange(e))
  	return;

  MutateToFloatingType(type_str, token, start_loc, end_loc, context);
}

void OCOR::MutateToIntegralType(
		const string &type_str, const string &token,
		const SourceLocation &start_loc, const SourceLocation &end_loc, 
		MusicContext *context)
{
	for (auto it: integral_type_list_)
  {
    if (it.compare(type_str) != 0)
    {
      string mutated_token = "(" + it + ")";
      
      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum());
    }
  }
}

void OCOR::MutateToFloatingType(
		const string &type_str, const string &token,
		const SourceLocation &start_loc, const SourceLocation &end_loc, 
		MusicContext *context)
{
	for (auto it: floating_type_list_)
  {
    if (it.compare(type_str) != 0)
    {
      string mutated_token = "(" + it + ")";
      
      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum());
    }
  }
}

void OCOR::MutateToSpecifiedRange(
    const string &type_str, const string &token,
    const SourceLocation &start_loc, const SourceLocation &end_loc, 
    MusicContext *context)
{
  for (auto e: range_)
  {
    if (e.compare(type_str) != 0)
    {
      string mutated_token = "(" + e + ")";
      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, mutated_token, 
          context->getStmtContext().getProteumStyleLineNum());
    }
  }
}
