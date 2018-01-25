#include "../music_utility.h"
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
bool VSCR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (MemberExpr *me = dyn_cast<MemberExpr>(e))
	{
		SourceLocation start_loc = e->getLocStart();
		SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

		return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
           !context->getStmtContext().IsInEnumDecl() &&
				   !context->getStmtContext().IsInArrayDeclSize();
	}

	return false;
}

void VSCR::Mutate(clang::Expr *e, MusicContext *context)
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
  SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();

  int line = GetLineNumber(src_mgr, start_loc);
  int col = GetColumnNumber(src_mgr, start_loc);

  /* Handling Macro. */
  if (start_loc.isMacroID())
  {
    start_loc = src_mgr.getExpansionLoc(start_loc);
    end_loc = Lexer::getLocForEndOfToken(start_loc, 0, src_mgr, context->comp_inst_->getLangOpts());
    string temp_member = string(
        src_mgr.getCharacterData(start_loc),
        src_mgr.getCharacterData(end_loc)-src_mgr.getCharacterData(start_loc));

    if (temp_member.compare(token) != 0)
      return;
  }

  if (end_loc.isInvalid() || end_loc < start_loc ||
      end_loc == start_loc)
  {
    end_loc = Lexer::getLocForEndOfToken(start_loc, 0, src_mgr, context->comp_inst_->getLangOpts());
  }

  StmtContext &stmt_context = context->getStmtContext();
  bool skip_float_literal = stmt_context.IsInNonFloatingExprRange(e) ||
                            stmt_context.IsInSwitchStmtConditionRange(e);

  if (auto rt = dyn_cast<RecordType>(base_type.getTypePtr()))
  {
  	RecordDecl *rd = rt->getDecl()->getDefinition();

  	for (auto field = rd->field_begin(); field != rd->field_end(); ++field)
  	{
      if (skip_float_literal &&
          field->getType().getCanonicalType().getTypePtr()->isFloatingType())
        continue;

  		string mutated_token{field->getNameAsString()};

  		if (token.compare(mutated_token) != 0 &&
  				IsSameType(me/*->getMemberDecl()*/->getType().getCanonicalType(),
  									 field->getType().getCanonicalType()))
  		{
  			context->mutant_database_.AddMutantEntry(name_, start_loc, end_loc, token, mutated_token, stmt_context.getProteumStyleLineNum());
  		}
  	}
  }
  else
  {
  	cout << "GenerateVscrMutant: cannot convert to record type at "; 
    PrintLocation(context->comp_inst_->getSourceManager(), start_loc);
  }
}

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