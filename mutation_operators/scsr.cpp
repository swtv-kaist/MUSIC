#include "../music_utility.h"
#include "scsr.h"

bool SCSR::ValidateDomain(const std::set<std::string> &domain)
{
	return true;
}

bool SCSR::ValidateRange(const std::set<std::string> &range)
{
	return true;
}

void SCSR::setRange(std::set<std::string> &range)
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
bool SCSR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (StringLiteral *sl = dyn_cast<StringLiteral>(e))
	{
		SourceLocation start_loc = sl->getBeginLoc();
    SourceLocation end_loc = GetEndLocOfStringLiteral(
    		context->comp_inst_->getSourceManager(), start_loc);
    StmtContext &stmt_context = context->getStmtContext();

    // Mutation is applicable if this expression is in mutation range,
    // not inside an enum declaration and not inside field decl range.
    // FieldDecl is a member of a struct or union.
    return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
    			 !stmt_context.IsInEnumDecl() &&
    			 !stmt_context.IsInFieldDeclRange(e);
	}

	return false;
}

void SCSR::Mutate(clang::Expr *e, MusicContext *context)
{
	// use to prevent duplicate mutants from local/global string literals
	set<string> stringCache;
	GenerateGlobalMutants(e, context, &stringCache);
	GenerateLocalMutants(e, context, &stringCache);
}

void SCSR::GenerateGlobalMutants(Expr *e, MusicContext *context,
																 set<string> *stringCache)
{
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfStringLiteral(src_mgr, start_loc);

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  vector<string> range;

	// All string literals from global list are distinct 
  // (filtered from InformationGatherer).
  for (auto it: *(context->getSymbolTable()->getGlobalStringLiteralList()))
  {
  	string mutated_token{ConvertToString(it, context->comp_inst_->getLangOpts())};

    if (mutated_token.compare(token) != 0)
    {
      range.push_back(mutated_token);
      stringCache->insert(mutated_token);
    }
  }

  // for (auto e: range)
  //   cout << "before global range: " << e << endl;

  if (partitions.size() > 0)
    ApplyRangePartition(&range);

  // for (auto e: range)
  //   cout << "after global range: " << e << endl;

  for (auto e: range)
    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, e, 
        context->getStmtContext().getProteumStyleLineNum());
}

void SCSR::GenerateLocalMutants(Expr *e, MusicContext *context,
															  set<string> *stringCache)
{
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	SourceLocation start_loc = e->getBeginLoc();
  SourceLocation end_loc = GetEndLocOfStringLiteral(src_mgr, start_loc);

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};

  vector<string> range;

	if (!context->getStmtContext().IsInCurrentlyParsedFunctionRange(e))
		return;

	for (auto it: (*(context->getSymbolTable()->getLocalStringLiteralList()))[context->getFunctionId()])
	{
		string mutated_token = ConvertToString(it, context->comp_inst_->getLangOpts());

    // mutate if the literal is not the same as the token
    // and prevent duplicate if the literal is already in the cache
    if (mutated_token.compare(token) != 0 &&
        stringCache->find(mutated_token) == stringCache->end())
    {
      stringCache->insert(mutated_token);
      range.push_back(mutated_token);
    }
	}

  // for (auto e: range)
  //   cout << "before local range: " << e << endl;

  if (partitions.size() > 0)
    ApplyRangePartition(&range);

  // for (auto e: range)
  //   cout << "after local range: " << e << endl;

  int count = 0;

  for (auto e: range)
  {
    int temp = count % 10 + 1;
    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, e, 
        context->getStmtContext().getProteumStyleLineNum());

    count++;
  }
}

bool SCSR::HandleRangePartition(string option) 
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

void SCSR::ApplyRangePartition(vector<string> *range)
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
