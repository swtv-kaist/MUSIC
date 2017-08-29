#include "../comut_utility.h"
#include "ocor.h"

OCOR::OCOR(const string name)
	: MutantOperatorTemplate(name)
{
	integral_type_list_ = {"int", "unsigned", "short", "long", 
                         "unsigned long", "char", "unsigned char", 
                         "signed char"};

  floating_type_list_ = {"float", "double", "long double"};
}

bool OCOR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool OCOR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool OCOR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (CStyleCastExpr *csce = dyn_cast<CStyleCastExpr>(e))
	{
		SourceLocation start_loc = csce->getLocStart();
    SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);
    const Type *type{csce->getTypeAsWritten().getCanonicalType().getTypePtr()};

    // OCOR mutates expression with C-style cast (Ex. (int) x)
    // These expr should be in mutation range, outside field decl
    // and the type of cast should be integral (int, char, float)
    return Range1IsPartOfRange2(
				SourceRange(start_loc, end_loc), 
				SourceRange(*(context->userinput->getStartOfMutationRange()),
										*(context->userinput->getEndOfMutationRange()))) &&
					 !LocationIsInRange(start_loc, *(context->fielddecl_range)) &&
					 (type->isIntegerType() || type->isCharType() || 
            type->isFloatingType());
	}

	return false;
}

// Return True if the mutant operator can mutate this statement
bool OCOR::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void OCOR::Mutate(clang::Expr *e, ComutContext *context)
{
	CStyleCastExpr *csce;
	if (!(csce = dyn_cast<CStyleCastExpr>(e)))
		return;

	// Get start and end location of parenthesis surrounding the cast type
	SourceLocation start_loc = csce->getLParenLoc();
  SourceLocation end_loc = csce->getRParenLoc();
  end_loc = end_loc.getLocWithOffset(1);

  string type_str{csce->getTypeAsWritten().getCanonicalType().getAsString()};

  if (type_str.compare("unsigned int") == 0)
    type_str = "unsigned";

  // retrieve exact type written in inputfile for database record
  string token;
  SourceLocation walk = start_loc;

  while (walk != end_loc)
  {
    token += *(
    		context->comp_inst->getSourceManager().getCharacterData(walk));
    walk = walk.getLocWithOffset(1);           
  }

  MutateToIntegralType(type_str, token, start_loc, end_loc, context);

  bool is_subexpr_ptr{
  		ExprIsPointer(csce->getSubExpr()->IgnoreImpCasts())};

  // Prevent mutating expr in array subscript, switch condition and case,
  // binary modulo, shift, bitwise epxr to floating-type.
  // all of pre-mentioned expr demands integral subexpr
  if (is_subexpr_ptr &&
  		LocationIsInRange(start_loc, *(context->arraysubscript_range)) &&
      LocationIsInRange(start_loc, *(context->switchcase_range)) &&
      LocationIsInRange(start_loc, *(context->switchstmt_condition_range)) &&
      LocationIsInRange(start_loc, *(context->non_OCOR_mutatable_expr_range)))
  	return;

  MutateToFloatingType(type_str, token, start_loc, end_loc, context);
}

void OCOR::Mutate(clang::Stmt *s, ComutContext *context)
{}

void OCOR::MutateToIntegralType(
		const string &type_str, const string &token,
		const SourceLocation &start_loc, const SourceLocation &end_loc, 
		ComutContext *context)
{
	for (auto it: integral_type_list_)
  {
    if (it.compare(type_str) != 0)
    {
      string mutated_token = "(" + it + ")";
      
      GenerateMutantFile(context, start_loc, end_loc, mutated_token);
			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																		token, mutated_token);
    }
  }
}

void OCOR::MutateToFloatingType(
		const string &type_str, const string &token,
		const SourceLocation &start_loc, const SourceLocation &end_loc, 
		ComutContext *context)
{
	for (auto it: floating_type_list_)
  {
    if (it.compare(type_str) != 0)
    {
      string mutated_token = "(" + it + ")";
      
      GenerateMutantFile(context, start_loc, end_loc, mutated_token);
			WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																		token, mutated_token);
    }
  }
}