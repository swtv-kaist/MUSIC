#ifndef COMUT_MUTANT_OPERATOR_HOLDER_H_
#define COMUT_MUTANT_OPERATOR_HOLDER_H_

#include "mutant_operator.h"

#include <vector>
#include <set>
#include <string>

/**
  Hold all the mutant operators that will be applied to the input file.

  @param  vectors containing Operator MutantOperator mutating arithmetic mutant_name,
      arithmetic assignment, bitwise mutant_name, bitwise assignment,
      shift mutant_name, shift assignment, logical mutant_name, relational mutant_name,
      plain assignment.  
*/
class MutantOperatorHolder
{
public:
  std::vector<MutantOperator> arithmetic_operator_mutators_;
  std::vector<MutantOperator> arith_assign_operator_mutators_;
  std::vector<MutantOperator> bitwise_operator_mutators_;
  std::vector<MutantOperator> bitwise_assign_operator_mutators_;
  std::vector<MutantOperator> shift_operator_mutators_;
  std::vector<MutantOperator> shift_assign_operator_mutators_;
  std::vector<MutantOperator> logical_operator_mutators_;
  std::vector<MutantOperator> relational_operator_mutators_;
  std::vector<MutantOperator> plain_assign_operator_mutators_;

  std::set<std::string> CRCR_integral_range_;
  std::set<std::string> CRCR_floating_range_;

  bool apply_SSDL_;
  bool apply_VLSR_;
  bool apply_VGSR_;
  bool apply_VGAR_;
  bool apply_VLAR_;
  bool apply_VGTR_;
  bool apply_VLTR_;
  bool apply_VGPR_;
  bool apply_VLPR_;
  bool apply_VSCR_;
  bool apply_CGCR_;
  bool apply_CLCR_;
  bool apply_CGSR_;
  bool apply_CLSR_;
  bool apply_OPPO_;
  bool apply_OMMO_;
  bool apply_OLNG_;
  bool apply_OBNG_;
  bool apply_OCNG_;
  bool apply_VTWD_;
  bool apply_OIPM_;
  bool apply_OCOR_;
  bool apply_SRWS_;
  bool apply_SANL_;
  bool apply_SCSR_;

  bool apply_VLSF_;
  bool apply_VGSF_;
  bool apply_VGTF_;
  bool apply_VLTF_;
  bool apply_VGPF_;
  bool apply_VLPF_;
  bool apply_VTWF_;

  MutantOperatorHolder();
  ~MutantOperatorHolder();

  /**
    Check if a given string is a float number. (existence of dot(.))

    @param  num targeted string
    @return true if it is floating type
            false otherwise
  */
  bool NumIsFloat(std::string num);

  /**
    Put a new mutant operator into the correct MutantOperator vector
    based on the token it can mutate or modify appropriate boolean variable.

    @param  mutant_name new mutant operator to be added.
    @return True if the operator is added successfully.
            False otherwise
  */
  bool AddMutantOperator(MutantOperator &mutant_name);

  void AddAllObomMutantOperators();

  void AddBinaryOperatorMutator(std::string mutant_name);

  /**
    Add to each Mutant Operator vector all the MutantOperator that it
    can receive, with tokens before and after mutation of each token
    same as in Agrawal's Design of Mutation Operators for C.    
    Called when user does not specify any specific operator(s) to use.
  */
  void useAll();
};

#endif  //COMUT_MUTANT_OPERATOR_HOLDER_H_