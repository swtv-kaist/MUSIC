#include "../comut_utility.h"
#include "vscr.h"

bool VSCR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool VSCR::ValidateRange(const std::set<std::string> &range)
{
	return range.empty();
}

// Return True if the mutant operator can mutate this expression
bool VSCR::CanMutate(clang::Expr *e, ComutContext *context)
{
	if (MemberExpr *me = dyn_cast<MemberExpr>(e))
	{
		SourceLocation start_loc = e->getLocStart();
		SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

		return Range1IsPartOfRange2(
				SourceRange(start_loc, end_loc), 
				SourceRange(*(context->userinput->getStartOfMutationRange()),
										*(context->userinput->getEndOfMutationRange()))) &&
				 !context->is_inside_enumdecl &&
				 !context->is_inside_array_decl_size;
	}

	return false;
}

// Return True if the mutant operator can mutate this statement
bool VSCR::CanMutate(clang::Stmt *s, ComutContext *context)
{
	return false;
}

void VSCR::Mutate(clang::Expr *e, ComutContext *context)
{
	MemberExpr *me;
	if (!(me = dyn_cast<MemberExpr>(e)))
		return;

	auto base_type = me->getBase()->getType().getCanonicalType();

  // structPointer->structMember
  // base_type now is pointer type. what we want is pointee type
  if (me->isArrow())  
  {
    auto pointer_type = cast<PointerType>(base_type.getTypePtr());
    base_type = pointer_type->getPointeeType().getCanonicalType();
  }

  string token{me->getMemberDecl()->getNameAsString()};
  SourceLocation start_loc = me->getMemberLoc();
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst);

  if (auto rt = dyn_cast<RecordType>(base_type.getTypePtr()))
  {
  	RecordDecl *rd = rt->getDecl()->getDefinition();

  	for (auto field = rd->field_begin(); field != rd->field_end(); ++field)
  	{
  		string mutated_token{field->getNameAsString()};

  		if (token.compare(mutated_token) != 0 &&
  				IsSameType(me/*->getMemberDecl()*/->getType().getCanonicalType(),
  									 field->getType().getCanonicalType()))
  		{
  			GenerateMutantFile(context, start_loc, end_loc, mutated_token);
				WriteMutantInfoToMutantDbFile(context, start_loc, end_loc, 
																			token, mutated_token);
  		}
  	}
  }
  else
  {
  	cout << "GenerateVscrMutant: cannot convert to record type at "; 
    PrintLocation(context->comp_inst->getSourceManager(), start_loc);
  }
}

void VSCR::Mutate(clang::Stmt *s, ComutContext *context)
{}

// assuming both parameters are Canonical types
bool VSCR::IsSameType(const QualType type1, const QualType type2)
{
	// int, float, char type are replacible for each other
	if (type1.getTypePtr()->isScalarType() &&
			!type1.getTypePtr()->isPointerType() &&
			type2.getTypePtr()->isScalarType() &&
			!type2.getTypePtr()->isPointerType())
		return true;

	if (type1.getTypePtr()->isPointerType() &&
			type2.getTypePtr()->isPointerType())
	{
		string pointee_type1 = getPointerType(type1);
		string pointee_type2 = getPointerType(type2);
		return pointee_type1.compare(pointee_type2) == 0;
	}

	if (type1.getTypePtr()->isArrayType() &&
			type2.getTypePtr()->isArrayType())
	{
		string member_type1 = cast<ArrayType>(
				type1.getTypePtr())->getElementType().getCanonicalType().getAsString();
		string member_type2 = cast<ArrayType>(
				type2.getTypePtr())->getElementType().getCanonicalType().getAsString();
		return member_type1.compare(member_type2) == 0;
	}

	if (type1.getTypePtr()->isStructureType() &&
			type2.getTypePtr()->isStructureType())
	{
		string struct_type1 = type1.getAsString();
		string struct_type2 = type2.getAsString();
		return struct_type1.compare(struct_type2) == 0;
	}

	return false;
}