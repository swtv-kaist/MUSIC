#ifndef MUSIC_CLCR_H_
#define MUSIC_CLCR_H_ 

#include "expr_mutant_operator.h"

class CLCR : public ExprMutantOperator
{
public:
  CLCR(const std::string name = "CLCR")
    : ExprMutantOperator(name), choose_max_(false), 
    choose_min_(false), choose_median_(false), 
    close_less_(false), close_more_(false), num_partitions(10)
  {}

  virtual bool ValidateDomain(const std::set<std::string> &domain);
  virtual bool ValidateRange(const std::set<std::string> &range);

  virtual void setRange(std::set<std::string> &range);

  // Return True if the mutant operator can mutate this expression
  virtual bool IsMutationTarget(clang::Expr *e, MusicContext *context);

  virtual void Mutate(clang::Expr *e, MusicContext *context);

private:
  bool choose_max_;
  bool choose_min_;
  bool choose_median_;
  bool close_less_;
  bool close_more_;
  std::set<int> partitions;
  int num_partitions;

  bool IsDuplicateCaseLabel(string new_label, 
                            SwitchStmtInfoList *switchstmt_list);
  void GetRange(clang::Expr *e, MusicContext *context, std::vector<std::string> *range);

  bool HandleRangePartition(std::string option); 

  void MergeListsToStringList(std::vector<long long> &range_values_int,
                              std::vector<long double> &range_values_float,
                              std::vector<std::string> &range_values);

  void HandleRangeOptionsWithFloat(std::string token_value, 
                                   std::vector<std::string> &range_int,
                                   std::vector<std::string> &range_float, 
                                   std::vector<std::string> *range);
};

#endif  // MUSIC_CLCR_H_