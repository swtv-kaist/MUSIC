#include "../music_utility.h"
#include "vscr.h"

bool VSCR::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool VSCR::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

void VSCR::setRange(std::set<std::string> &range)
{
  for (auto it = range.begin(); it != range.end(); )
  {
    if (HandleRangePartition(*it))
      it = range.erase(it);
    else
      ++it;
  }

  range_ = range;

  // for (auto it: partitions)
  //   cout << "part: " << it << endl;

  // for (auto it: range_)
  //   cout << "range: " << it << endl;
}

// Return True if the mutant operator can mutate this expression
bool VSCR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (MemberExpr *me = dyn_cast<MemberExpr>(e))
	{
		SourceLocation start_loc = e->getBeginLoc();
		SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

    SourceManager &src_mgr = context->comp_inst_->getSourceManager();
    Rewriter rewriter;
    rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

    string token{
        ConvertToString(me->getBase(), context->comp_inst_->getLangOpts())};
    bool is_in_domain = domain_.empty() ? true : 
                        IsStringElementOfSet(token, domain_);

		return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
           !context->getStmtContext().IsInEnumDecl() &&
				   !context->getStmtContext().IsInArrayDeclSize() && is_in_domain;
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
    end_loc = Lexer::getLocForEndOfToken(start_loc, 0, src_mgr, 
                                         context->comp_inst_->getLangOpts());
    string temp_member = string(
        src_mgr.getCharacterData(start_loc),
        src_mgr.getCharacterData(end_loc)-src_mgr.getCharacterData(start_loc));

    if (temp_member.compare(token) != 0)
      return;
  }

  if (end_loc.isInvalid() || end_loc < start_loc ||
      end_loc == start_loc)
  {
    end_loc = Lexer::getLocForEndOfToken(start_loc, 0, src_mgr, 
                                         context->comp_inst_->getLangOpts());
  }

  StmtContext &stmt_context = context->getStmtContext();
  bool skip_float_literal = stmt_context.IsInNonFloatingExprRange(e) ||
                            stmt_context.IsInSwitchStmtConditionRange(e);

  if (auto rt = dyn_cast<RecordType>(base_type.getTypePtr()))
  {
  	RecordDecl *rd = rt->getDecl()->getDefinition();

    vector<string> range;

  	for (auto field = rd->field_begin(); field != rd->field_end(); ++field)
  	{
      if (skip_float_literal &&
          field->getType().getCanonicalType().getTypePtr()->isFloatingType())
        continue;

  		string mutated_token{field->getNameAsString()};

      if (!range_.empty() && range_.find(mutated_token) == range_.end())
        continue;

  		if (token.compare(mutated_token) != 0 &&
  				IsSameType(me/*->getMemberDecl()*/->getType().getCanonicalType(),
  									 field->getType().getCanonicalType()))
  		{
        range.push_back(mutated_token);
  		}
  	}

    // for (auto it: range)
    //   cout << "before range: " << it << endl;

    if (partitions.size() > 0)
      ApplyRangePartition(&range);

    for (auto it: range)
    {
      // cout << "after range: " << it << endl;
      context->mutant_database_.AddMutantEntry(context->getStmtContext(),
          name_, start_loc, end_loc, token, it, 
          context->getStmtContext().getProteumStyleLineNum());
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

    // cout << struct_type1 << " " << struct_type2 << endl;
		return struct_type1.compare(struct_type2) == 0;
	}

	return false;
}

bool VSCR::HandleRangePartition(string option) 
{
  vector<string> words;
  SplitStringIntoVector(option, words, string(" "));

  // Return false if this option does not contain enough words to specify 
  // partition or first word is not 'part'
  if (words.size() < 2 || words[0].compare("part") != 0)
    return false;

  for (int i = 1; i < words.size(); i++)
  {
    int num;
    if (ConvertStringToInt(words[i], num))
    {
      if (num > 0 && num <= 10)
        partitions.insert(num);
      else
      {
        cout << "No partition number " << num << ". Skip.\n";
        cout << "There are only 10 partitions for now.\n";
        continue;
      }
    }
    else
    {
      cout << "Cannot convert " << words[i] << " to an integer. Skip.\n";
      continue;
    }
  }

  return true;
}

void VSCR::ApplyRangePartition(vector<string> *range)
{
  vector<string> range2;
  range2 = *range;

  range->clear();
  sort(range2.begin(), range2.end(), SortStringAscending);

  for (auto part_num: partitions) 
  {
    // Number of possible tokens to mutate to might be smaller than 10.
    // So we do not have 10 partitions.
    if (part_num > range2.size())
    {
      cout << "There are only " << range2.size() << " to mutate to.\n";
      cout << "No partition number " << part_num << endl;
      continue;
    }

    if (range2.size() < num_partitions)
    {
      range->push_back(range2[part_num-1]);
      continue;
    }

    int start_idx = (range2.size() / 10) * (part_num - 1);
    int end_idx = (range2.size() / 10) * part_num;

    if (part_num == 10)
      end_idx = range2.size();

    for (int idx = start_idx; idx < end_idx; idx++)
      range->push_back(range2[idx]);
  }
}
