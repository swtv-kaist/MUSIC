#ifndef MUSIC_RLCR_H_
#define MUSIC_RLCR_H_ 

#include "expr_mutant_operator.h"

// Right hand side to Global Constant Replacement
class RLCR : public ExprMutantOperator
{
public:
  RLCR(const std::string name = "RLCR")
    : ExprMutantOperator(name), num_partitions(10)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  virtual void setRange(std::set<std::string> &range);

  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

  virtual void Mutate(clang::Expr *e, MusicContext *context); 

  bool IsInitMutationTarget(clang::Expr *e, MusicContext *context);

private:
  std::set<int> partitions;
  int num_partitions;

  bool IsDuplicateCaseLabel(string new_label, 
                            SwitchStmtInfoList *switchstmt_list);
  void GetRange(clang::Expr *e, MusicContext *context, 
                std::vector<std::string> *range);

  bool HandleRangePartition(std::string option);

  void MergeListsToStringList(std::vector<long long> &range_values_int,
                              std::vector<long double> &range_values_float,
                              std::vector<std::string> &range_values);

  // void HandleRangeOptionsWithFloat(std::string token_value, 
  //                                  std::vector<std::string> &range_int,
  //                                  std::vector<std::string> &range_float, 
  //                                  std::vector<std::string> *range);
};

#endif  // MUSIC_RLCR_H_