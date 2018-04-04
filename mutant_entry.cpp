#include "mutant_entry.h"
#include "music_utility.h"

MutantEntry::MutantEntry(
    string token, string mutated_token, clang::SourceLocation start_loc, 
    clang::SourceLocation end_loc, clang::SourceManager &src_mgr,
    int proteum_style_line_num)
: token_(token), mutated_token_(mutated_token),
start_location_(start_loc), end_location_before_mutation_(end_loc),
proteum_style_line_num_(proteum_style_line_num), src_mgr_(src_mgr)
{
  // cout << "cp MutantEntry\n";
  // POTENTIAL ISSUE: no translateLineCol check.
  end_location_after_mutation_ = src_mgr.translateLineCol(
      src_mgr.getMainFileID(), GetLineNumber(src_mgr, start_loc),
      GetColumnNumber(src_mgr, start_loc) + \
      CountNonNewlineChar(mutated_token));
}

int MutantEntry::getProteumStyleLineNum() const
{
  return proteum_style_line_num_;
}

std::string MutantEntry::getToken() const
{
  return token_;
}
std::string MutantEntry::getMutatedToken() const
{
  return mutated_token_;
}
clang::SourceLocation MutantEntry::getStartLocation() const
{
  return start_location_;
}
clang::SourceLocation MutantEntry::getTokenEndLocation() const
{
  return end_location_before_mutation_;
}
clang::SourceRange MutantEntry::getTokenRange() const
{
  return SourceRange(start_location_, end_location_before_mutation_);
}
clang::SourceLocation MutantEntry::getMutatedTokenEndLocation() const
{
  return end_location_after_mutation_;
}
clang::SourceRange MutantEntry::getMutatedTokenRange() const
{
  return SourceRange(start_location_, end_location_after_mutation_);
}

bool MutantEntry::operator==(const MutantEntry &rhs) const
{
  return (token_.compare(rhs.getToken()) == 0 &&
          mutated_token_.compare(rhs.getMutatedToken()) == 0);
}

