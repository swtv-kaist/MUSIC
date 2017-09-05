#include "../comut_utility.h"
#include "oipm.h"

bool OIPM::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool OIPM::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool OIPM::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
	{
		if (uo->getOpcode() != UO_Deref)
			return false;

		// OIPM can mutate expressions which have the following forms:
		// 		*...*array[index]
		// 		*...*pointer++
		// 		*...*pointer--
		Expr *first_non_deref_subexpr = cast<Expr>(uo);

		// Retrieve the sub-expression (array[index] or pointer++ or pointer--)
    while (true)
    {
      UnaryOperator *subexpr;

      if (subexpr = dyn_cast<UnaryOperator>(first_non_deref_subexpr))
        if (subexpr->getOpcode() == UO_Deref)
        {
          first_non_deref_subexpr = subexpr->getSubExpr()->IgnoreImpCasts();
          continue;
        }

      break;
    }

    SourceLocation start_loc = uo->getLocStart();
    SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst);

    if (isa<ArraySubscriptExpr>(first_non_deref_subexpr))
    {
    	is_subexpr_arraysubscript = true;

    	return Range1IsPartOfRange2(
					SourceRange(start_loc, end_loc), 
					SourceRange(*(context->userinput->getStartOfMutationRange()),
											*(context->userinput->getEndOfMutationRange()))) &&
    				!context->is_inside_enumdecl;
    }

    // if subexpr has pointer form (pointer++ or pointer--)
    // then OIPM can be applied only when 
    // 		- (*...*pointer) is not constant because you cannot ++ or -- a const
    //    - (*...*pointer) is not left hand side of assignment because
    // 			mutation would make it a rvalue -> syntactically incorrect
    if (UnaryOperator *uo_subexpr = \
    		dyn_cast<UnaryOperator>(first_non_deref_subexpr))
    {
    	is_subexpr_pointer = true;

    	return Range1IsPartOfRange2(
					SourceRange(start_loc, end_loc), 
					SourceRange(*(context->userinput->getStartOfMutationRange()),
											*(context->userinput->getEndOfMutationRange()))) &&
    				 !context->is_inside_enumdecl &&
    				 (uo_subexpr->getOpcode() == UO_PostDec || 
  					 uo_subexpr->getOpcode() == UO_PostInc) &&
  					 !uo->getType().getCanonicalType().isConstQualified() &&
  					 start_loc != context->lhs_of_assignment_range->getBegin();
  	}
	}

	return false;
}



void OIPM::Mutate(clang::Expr *e, ComutContext *context)
{
	UnaryOperator *uo;
	if (!(uo = dyn_cast<UnaryOperator>(e)))
		return;

	Rewriter rewriter;
	rewriter.setSourceMgr(context->comp_inst->getSourceManager(), 
														context->comp_inst->getLangOpts());
	string token{rewriter.ConvertToString(uo)};

	Expr *first_non_deref_subexpr = cast<Expr>(uo);

	// Retrieve the sub-expression (array[index] or pointer++ or pointer--)
  while (true)
  {
    UnaryOperator *subexpr;

    if (subexpr = dyn_cast<UnaryOperator>(first_non_deref_subexpr))
      if (subexpr->getOpcode() == UO_Deref)
      {
        first_non_deref_subexpr = subexpr->getSubExpr()->IgnoreImpCasts();
        continue;
      }

    break;
  }

  SourceLocation start_loc = uo->getLocStart();
  SourceLocation end_loc = GetEndLocOfUnaryOpExpr(uo, context->comp_inst);

	if (is_subexpr_pointer)
	{
		is_subexpr_pointer = false;
		MutatePointerSubExpr(first_non_deref_subexpr, start_loc, 
												 end_loc, token, context);
	}

	if (is_subexpr_arraysubscript)
	{
		is_subexpr_arraysubscript = false;
		MutateArraySubscriptSubExpr(first_non_deref_subexpr, start_loc, 
															  end_loc, token, context);
	}
}



SourceLocation OIPM::GetLeftBracketOfArraySubscript(
		const ArraySubscriptExpr *ase, SourceManager &src_mgr)
{
  SourceLocation ret = ase->getLocStart();

  while (*(src_mgr.getCharacterData(ret)) != '[')
    ret = ret.getLocWithOffset(1);

  return ret;
}

void OIPM::MutateArraySubscriptSubExpr(
		Expr *subexpr, const SourceLocation start_loc,
		const SourceLocation end_loc, const string token, 
		ComutContext *context)
{
	ArraySubscriptExpr *ase;
	if (!(ase = dyn_cast<ArraySubscriptExpr>(subexpr)))
		return;

	SourceManager &src_mgr = context->comp_inst->getSourceManager();
	SourceLocation left_bracket_loc = GetLeftBracketOfArraySubscript(
			ase, src_mgr);

	string array_index;

  while (left_bracket_loc != end_loc)
  {
    array_index += *(src_mgr.getCharacterData(left_bracket_loc));
    left_bracket_loc = left_bracket_loc.getLocWithOffset(1);
  }

  // token has the form *...*arr[idx]
  // mutated_token will have the form (*...*arr)[idx]
  string mutated_token = "(" + token.substr(0, token.length() - \
                          array_index.length()) + ")" + array_index;

  GenerateMutantFile(context, start_loc, end_loc, mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																token, mutated_token);
}

void OIPM::MutatePointerSubExpr(
		Expr *subexpr, const SourceLocation start_loc,
		const SourceLocation end_loc, const string token, 
		ComutContext *context)
{
	UnaryOperator *uo;
	if (!(uo = dyn_cast<UnaryOperator>(subexpr)))
		return;

	// retrieve the string form of given expression without ++/--
  string mutated_token{token};
  mutated_token.pop_back();
  mutated_token.pop_back();

  string first_mutated_token;
  if (uo->getOpcode() == UO_PostDec)
    first_mutated_token = "--(" + mutated_token + ")";
  else
    first_mutated_token = "++(" + mutated_token + ")";

  GenerateMutantFile(context, start_loc, end_loc, first_mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																token, first_mutated_token);

	string second_mutated_token;
  if (uo->getOpcode() == UO_PostDec)
    second_mutated_token = "(" + mutated_token + ")--";
  else
    second_mutated_token = "(" + mutated_token + ")++";

  GenerateMutantFile(context, start_loc, end_loc, second_mutated_token);
	WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																token, second_mutated_token);
}
