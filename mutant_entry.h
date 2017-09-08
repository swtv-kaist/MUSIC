#ifndef COMUT_MUTANT_ENTRY_H_
#define COMUT_MUTANT_ENTRY_H_ 

#include <string>
#include <iostream>

#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"

class MutantEntry
{
public:
  clang::SourceManager &src_mgr_;

  MutantEntry(std::string token, std::string mutated_token,
              clang::SourceLocation start_loc, clang::SourceLocation end_loc,
              clang::SourceManager &src_mgr, int proteum_style_line_num);

  // getters
  int getProteumStyleLineNum() const;
  std::string getToken() const;
  std::string getMutatedToken() const;
  // only end location changes after mutation
  clang::SourceLocation getStartLocation() const;
  clang::SourceLocation getTokenEndLocation() const;
  clang::SourceRange getTokenRange() const;
  clang::SourceLocation getMutatedTokenEndLocation() const;
  clang::SourceRange getMutatedTokenRange() const;

private:
  int proteum_style_line_num_;
  std::string token_;
  std::string mutated_token_;
  clang::SourceLocation start_location_;
  clang::SourceLocation end_location_;
  clang::SourceLocation end_location_after_mutation_;
};

#endif  // COMUT_MUTANT_ENTRY_H_