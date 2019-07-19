#ifndef MUSIC_MUTANT_ENTRY_H_
#define MUSIC_MUTANT_ENTRY_H_ 

#include <string>
#include <iostream>
#include <vector>

#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"

class MutantEntry
{
public:
  clang::SourceManager &src_mgr_;

  MutantEntry(std::vector<std::string> tokens, std::vector<std::string> mutated_token, 
              std::vector<std::string> additional_info, 
              std::vector<clang::SourceLocation> start_locs, 
              std::vector<clang::SourceLocation> end_locs,
              clang::SourceManager &src_mgr, int proteum_style_line_num);

  // getters
  int getProteumStyleLineNum() const;
  std::vector<std::string> getToken() const;
  std::vector<std::string> getMutatedToken() const;
  std::vector<std::string> getAdditionalInfo() const;
  // only end location changes after mutation
  std::vector<clang::SourceLocation> getStartLocation() const;
  std::vector<clang::SourceLocation> getTokenEndLocation() const;
  // clang::SourceRange getTokenRange() const;
  std::vector<clang::SourceLocation> getMutatedTokenEndLocation() const;
  // clang::SourceRange getMutatedTokenRange() const;

  bool operator==(const MutantEntry &rhs) const;

private:
  int proteum_style_line_num_;
  std::vector<std::string> token_;
  std::vector<std::string> mutated_token_;
  std::vector<std::string> additional_info_;
  std::vector<clang::SourceLocation> start_location_;
  std::vector<clang::SourceLocation> end_location_before_mutation_;
  std::vector<clang::SourceLocation> end_location_after_mutation_;
};

#endif  // MUSIC_MUTANT_ENTRY_H_