#ifndef COMUT_MUTANT_OPERATOR_H_
#define COMUT_MUTANT_OPERATOR_H_

#include <string>
#include <set>

/**
  Represent a mutant operator

  @param  name of the operator.
          set of tokens that the operator can mutate.
          set of tokens that the operator can mutate to.
*/
class MutantOperator
{
public:
  std::string mutant_name_;
  std::set<std::string> domain_;
  std::set<std::string> range_;

  MutantOperator(std::string name, std::set<std::string> domain, 
                 std::set<std::string> range);

  ~MutantOperator();

  std::string& getName();
};

#endif  // COMUT_MUTANT_OPERATOR_H_