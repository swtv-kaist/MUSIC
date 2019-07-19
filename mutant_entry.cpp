#include "mutant_entry.h"
#include "music_utility.h"

MutantEntry::MutantEntry(
    std::vector<string> tokens, std::vector<string> mutated_tokens, 
    std::vector<std::string> additional_info, 
    std::vector<clang::SourceLocation> start_locs,     
    std::vector<clang::SourceLocation> end_locs, clang::SourceManager &src_mgr,
    int proteum_style_line_num)
: /*token_(token), mutated_token_(mutated_token),*/
/*start_location_(start_loc), end_location_before_mutation_(end_loc),*/
proteum_style_line_num_(proteum_style_line_num), src_mgr_(src_mgr)
{
  for (int i = 0; i < tokens.size(); i++)
  {
    token_.push_back(tokens[i]);
    mutated_token_.push_back(mutated_tokens[i]);
    start_location_.push_back(start_locs[i]);
    end_location_before_mutation_.push_back(end_locs[i]);

    // POTENTIAL ISSUE: no translateLineCol check.
    end_location_after_mutation_.push_back(
        src_mgr.translateLineCol(
            src_mgr.getMainFileID(), GetLineNumber(src_mgr, start_locs[i]),
            GetColumnNumber(src_mgr, start_locs[i]) + \
            CountNonNewlineChar(mutated_tokens[i])));
  }

  for (int i = 0; i < additional_info.size(); i++)
    additional_info_.push_back(additional_info[i]);
}

// MutantEntry::MutantEntry(
//     string token, string mutated_token, 
//     string additional_info, clang::SourceLocation start_loc, 
//     clang::SourceLocation end_loc, clang::SourceManager &src_mgr,
//     int proteum_style_line_num)
// : token_(token), mutated_token_(mutated_token), additional_info_(additional_info),
// start_location_(start_loc), end_location_before_mutation_(end_loc),
// proteum_style_line_num_(proteum_style_line_num), src_mgr_(src_mgr)
// {
//   // cout << "cp MutantEntry\n";
//   // POTENTIAL ISSUE: no translateLineCol check.
//   end_location_after_mutation_ = src_mgr.translateLineCol(
//       src_mgr.getMainFileID(), GetLineNumber(src_mgr, start_loc),
//       GetColumnNumber(src_mgr, start_loc) + \
//       CountNonNewlineChar(mutated_token));
// }

int MutantEntry::getProteumStyleLineNum() const
{
  return proteum_style_line_num_;
}

std::vector<std::string> MutantEntry::getToken() const
{
  return token_;
}

std::vector<std::string> MutantEntry::getMutatedToken() const
{
  return mutated_token_;
}

std::vector<std::string> MutantEntry::getAdditionalInfo() const
{
  return additional_info_;
  // if (additional_info_.size() > 0)
  //   return additional_info_;
  // else
  //   return "empty";
}

std::vector<clang::SourceLocation> MutantEntry::getStartLocation() const
{
  return start_location_;
}

std::vector<clang::SourceLocation> MutantEntry::getTokenEndLocation() const
{
  return end_location_before_mutation_;
}

// clang::SourceRange MutantEntry::getTokenRange() const
// {
//   return SourceRange(start_location_, end_location_before_mutation_);
// }

std::vector<clang::SourceLocation> MutantEntry::getMutatedTokenEndLocation() const
{
  return end_location_after_mutation_;
}

// clang::SourceRange MutantEntry::getMutatedTokenRange() const
// {
//   return SourceRange(start_location_, end_location_after_mutation_);
// }

bool MutantEntry::operator==(const MutantEntry &rhs) const
{
  bool is_same = true;
  vector<string> rhs_token = rhs.getToken();
  vector<string> rhs_mutated_token = rhs.getMutatedToken();

  if (token_.size() != rhs_token.size())
    return false;

  for (int i = 0; i < token_.size(); i++)
  {
    if (token_[i].compare(rhs_token[i]) != 0 ||
        mutated_token_[i].compare(rhs_mutated_token[i]) != 0)
      return false;
  }

  return true;

  // return (token_.compare(rhs.getToken()) == 0 &&
  //         mutated_token_.compare(rhs.getMutatedToken()) == 0);
}

