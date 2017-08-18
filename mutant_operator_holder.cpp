#include "mutant_operator_holder.h"
#include "comut_utility.h"

#include <iostream>

MutantOperatorHolder::MutantOperatorHolder()
{
  apply_SSDL_ = false;
  apply_VLSR_ = false;
  apply_VGSR_ = false;
  apply_VGAR_ = false;
  apply_VLAR_ = false;
  apply_VGTR_ = false;
  apply_VLTR_ = false;
  apply_VGPR_ = false;
  apply_VLPR_ = false;
  apply_VSCR_ = false;
  apply_CGCR_ = false;
  apply_CLCR_ = false;
  apply_CGSR_ = false;
  apply_CLSR_ = false;
  apply_OPPO_ = false;
  apply_OMMO_ = false;
  apply_OLNG_ = false;
  apply_OBNG_ = false;
  apply_OCNG_ = false; 
  apply_VTWD_ = false;
  apply_OIPM_ = false;
  apply_OCOR_ = false;
  apply_SANL_ = false;
  apply_SRWS_ = false;
  apply_SCSR_ = false;

  apply_VLSF_ = false;
  apply_VGSF_ = false;
  apply_VGTF_ = false;
  apply_VLTF_ = false;
  apply_VGPF_ = false;
  apply_VLPF_ = false;
  apply_VTWF_ = false;
}

MutantOperatorHolder::~MutantOperatorHolder() { }

bool MutantOperatorHolder::AddMutantOperator(MutantOperator &mutant_name)
{
  // set of operators mutating logical operator
  std::set<std::string> logical{"OLLN", "OLAN", "OLBN", "OLRN", "OLSN"};

  // set of operators mutating relational operator.
  std::set<std::string> relational{"ORLN", "ORAN", "ORBN", /*"ORRN",*/ "ORSN"};

  // set of operators mutating arithmetic operators.
  std::set<std::string> arithmetic{"OALN", "OAAN", "OABN", "OARN", "OASN"};

  // set of operators mutating bitwise operator
  std::set<std::string> bitwise{"OBLN", "OBAN", "OBBN", "OBRN", "OBSN"};

  // set of operators mutating shift operator
  std::set<std::string> shift{"OSLN", "OSAN", "OSBN", "OSRN", "OSSN"};

  // set of operators mutating arithmetic assignment operator
  std::set<std::string> arithmetic_assignment{"OAAA", "OABA", "OAEA", "OASA"};

  // set of operators mutating bitwise assignment operator
  std::set<std::string> bitwise_assignment{"OBAA", "OBBA", "OBEA", "OBSA"};

  // set of operators mutating shift assignment operator
  std::set<std::string> shift_assignment{"OSAA", "OSBA", "OSEA", "OSSA"};

  // set of operators mutating plain assignment operator
  std::set<std::string> plain_assignment{"OEAA", "OEBA", "OESA"};

  // Check which set does the operator belongs to.
  // Add the operator to the corresponding vector.
  if (logical.find(mutant_name.getName()) != logical.end())
  {
    logical_operator_mutators_.push_back(mutant_name);
  }
  else if (relational.find(mutant_name.getName()) != relational.end())
  {
    relational_operator_mutators_.push_back(mutant_name);
  }
  else if (arithmetic.find(mutant_name.getName()) != arithmetic.end())
  {
    arithmetic_operator_mutators_.push_back(mutant_name);
  }
  else if (bitwise.find(mutant_name.getName()) != bitwise.end())
  {
    bitwise_operator_mutators_.push_back(mutant_name);
  }
  else if (shift.find(mutant_name.getName()) != shift.end())
  {
    shift_operator_mutators_.push_back(mutant_name);
  }
  else if (arithmetic_assignment.find(mutant_name.getName()) != \
           arithmetic_assignment.end())
  {
    arith_assign_operator_mutators_.push_back(mutant_name);
  }
  else if (bitwise_assignment.find(mutant_name.getName()) != \
           bitwise_assignment.end())
  {
    bitwise_assign_operator_mutators_.push_back(mutant_name);
  }
  else if (shift_assignment.find(mutant_name.getName()) != \
           shift_assignment.end())
  {
    shift_assign_operator_mutators_.push_back(mutant_name);
  }
  else if (plain_assignment.find(mutant_name.getName()) != \
           plain_assignment.end())
  {
    plain_assign_operator_mutators_.push_back(mutant_name);
  }
  else if (mutant_name.getName().compare("CRCR") == 0) 
  {
    /*CRCR_integral_range_ = {"0", "1", "-1"};
    CRCR_floating_range_ = {"0.0", "1.0", "-1.0"};

    for (auto it: mutant_name.range_)
    {
      if (NumIsFloat(it))
        CRCR_floating_range_.insert(it);
      else
        CRCR_integral_range_.insert(it);
    }

    std::cout << "crcr float\n";
    PrintStringSet(CRCR_floating_range_);
    std::cout << "crcr integral\n";
    PrintStringSet(CRCR_integral_range_);*/
  }
  else if (mutant_name.getName().compare("SSDL") == 0)
  {
    apply_SSDL_ = false;
  }
  else if (mutant_name.getName().compare("VLSR") == 0)
  {
    apply_VLSR_ = false;
  }
  else if (mutant_name.getName().compare("VGSR") == 0)
  {
    apply_VGSR_ = false;
  }
  else if (mutant_name.getName().compare("VGAR") == 0)
  {
    apply_VGAR_ = false;
  }
  else if (mutant_name.getName().compare("VLAR") == 0)
  {
    apply_VLAR_ = false;
  }
  else if (mutant_name.getName().compare("VGTR") == 0)
  {
    apply_VGTR_ = false;
  }
  else if (mutant_name.getName().compare("VLTR") == 0)
  {
    apply_VLTR_ = false;
  }
  else if (mutant_name.getName().compare("VGPR") == 0)
  {
    apply_VGPR_ = false;
  }
  else if (mutant_name.getName().compare("VLPR") == 0)
  {
    apply_VLPR_ = false;
  }
  else if (mutant_name.getName().compare("VSCR") == 0)
  {
    apply_VSCR_ = false;
  }
  else if (mutant_name.getName().compare("CGCR") == 0)
  {
    apply_CGCR_ = false;
  }
  else if (mutant_name.getName().compare("CLCR") == 0)
  {
    apply_CLCR_ = false;
  }
  else if (mutant_name.getName().compare("CGSR") == 0)
  {
    apply_CGSR_ = false;
  }
  else if (mutant_name.getName().compare("CLSR") == 0)
  {
    apply_CLSR_ = false;
  }
  else if (mutant_name.getName().compare("OMMO") == 0)
  {
    apply_OMMO_ = false;
  }
  else if (mutant_name.getName().compare("OPPO") == 0)
  {
    apply_OPPO_ = false;
  }
  else if (mutant_name.getName().compare("OLNG") == 0)
  {
    apply_OLNG_ = false;
  }
  else if (mutant_name.getName().compare("OCNG") == 0)
  {
    apply_OCNG_ = false;
  }
  else if (mutant_name.getName().compare("OBNG") == 0)
  {
    apply_OBNG_ = false;
  }
  else if (mutant_name.getName().compare("OBOM") == 0)
  {
    AddAllObomMutantOperators();
  }
  else if (mutant_name.getName().compare("VTWD") == 0)
  {
    apply_VTWD_ = false;
  }
  else if (mutant_name.getName().compare("OIPM") == 0)
  {
    apply_OIPM_ = false;
  }
  else if (mutant_name.getName().compare("OCOR") == 0)
  {
    apply_OCOR_ = true;
  }
  else if (mutant_name.getName().compare("SANL") == 0)
  {
    apply_SANL_ = false;
  }
  else if (mutant_name.getName().compare("SRWS") == 0)
  {
    apply_SRWS_ = false;
  }
  else if (mutant_name.getName().compare("SCSR") == 0)
  {
    apply_SCSR_ = false;
  }

  else if (mutant_name.getName().compare("VLSF") == 0)
  {
    apply_VLSF_ = false;
  }
  else if (mutant_name.getName().compare("VGSF") == 0)
  {
    apply_VGSF_ = false;
  }
  else if (mutant_name.getName().compare("VGTF") == 0)
  {
    apply_VGTF_ = false;
  }
  else if (mutant_name.getName().compare("VLTF") == 0)
  {
    apply_VLTF_ = false;
  }
  else if (mutant_name.getName().compare("VGPF") == 0)
  {
    apply_VGPF_ = false;
  }
  else if (mutant_name.getName().compare("VLPF") == 0)
  {
    apply_VLPF_ = false;
  }
  else if (mutant_name.getName().compare("VTWF") == 0)
  {
    apply_VTWF_ = false;
  }
  else if (mutant_name.getName().compare("ORRN") == 0)
  {
    cout << "do not add orrn\n";
  }

  else {
    // The operator does not belong to any of the supported categories
    return false;
  }

  return true;
}

void MutantOperatorHolder::AddAllObomMutantOperators()
{
  AddBinaryOperatorMutator(std::string("OLLN"));
  AddBinaryOperatorMutator(std::string("OLAN"));
  AddBinaryOperatorMutator(std::string("OLBN"));
  AddBinaryOperatorMutator(std::string("OLRN"));
  AddBinaryOperatorMutator(std::string("OLSN"));

  AddBinaryOperatorMutator(std::string("ORLN"));
  AddBinaryOperatorMutator(std::string("ORAN"));
  AddBinaryOperatorMutator(std::string("ORBN"));
  // AddBinaryOperatorMutator(std::string("ORRN"));
  AddBinaryOperatorMutator(std::string("ORSN"));

  AddBinaryOperatorMutator(std::string("OALN"));
  AddBinaryOperatorMutator(std::string("OAAN"));
  AddBinaryOperatorMutator(std::string("OABN"));
  AddBinaryOperatorMutator(std::string("OARN"));
  AddBinaryOperatorMutator(std::string("OASN"));

  AddBinaryOperatorMutator(std::string("OBLN"));
  AddBinaryOperatorMutator(std::string("OBAN"));
  AddBinaryOperatorMutator(std::string("OBBN"));
  AddBinaryOperatorMutator(std::string("OBRN"));
  AddBinaryOperatorMutator(std::string("OBSN"));

  AddBinaryOperatorMutator(std::string("OSLN"));
  AddBinaryOperatorMutator(std::string("OSAN"));
  AddBinaryOperatorMutator(std::string("OSBN"));
  AddBinaryOperatorMutator(std::string("OSRN"));
  AddBinaryOperatorMutator(std::string("OSSN"));

  AddBinaryOperatorMutator(std::string("OAAA"));
  AddBinaryOperatorMutator(std::string("OABA"));
  AddBinaryOperatorMutator(std::string("OAEA"));
  AddBinaryOperatorMutator(std::string("OASA"));

  AddBinaryOperatorMutator(std::string("OBAA"));
  AddBinaryOperatorMutator(std::string("OBBA"));
  AddBinaryOperatorMutator(std::string("OBEA"));
  AddBinaryOperatorMutator(std::string("OBSA"));

  AddBinaryOperatorMutator(std::string("OSAA"));
  AddBinaryOperatorMutator(std::string("OSBA"));
  AddBinaryOperatorMutator(std::string("OSEA"));
  AddBinaryOperatorMutator(std::string("OSSA"));

  AddBinaryOperatorMutator(std::string("OEAA"));
  AddBinaryOperatorMutator(std::string("OEBA"));
  AddBinaryOperatorMutator(std::string("OESA"));
}

void MutantOperatorHolder::AddBinaryOperatorMutator(std::string mutant_name)
{
  std::set<std::string> domain;
  std::set<std::string> range;

  ValidateRangeOfMutantOperator(mutant_name, range);
  ValidateDomainOfMutantOperator(mutant_name, domain);
  
  MutantOperator new_operator{mutant_name, domain, range};
  AddMutantOperator(new_operator);
}

void MutantOperatorHolder::ApplyAllMutantOperators()
{
  AddAllObomMutantOperators();

  // CRCR_integral_range_ = {"0", "1", "-1"};
  // CRCR_floating_range_ = {"0.0", "1.0", "-1.0"};

  apply_SSDL_ = false;
  apply_VGSR_ = false;
  apply_VLSR_ = false;
  apply_VGAR_ = false;
  apply_VLAR_ = false;
  apply_VGTR_ = false;
  apply_VLTR_ = false;
  apply_VGPR_ = false;
  apply_VLPR_ = false;
  apply_VSCR_ = false;
  apply_CGCR_ = false;
  apply_CLCR_ = false;
  apply_CGSR_ = false;
  apply_CLSR_ = false;
  apply_OPPO_ = false;
  apply_OMMO_ = false;
  apply_OLNG_ = false;
  apply_OBNG_ = false;
  apply_OCNG_ = false;  
  apply_VTWD_ = false;  
  apply_OIPM_ = false;
  apply_OCOR_ = true;
  apply_SANL_ = false;
  apply_SRWS_ = false;
  apply_SCSR_ = false;

  apply_VLSF_ = false;
  apply_VGSF_ = false;
  apply_VGTF_ = false;
  apply_VLTF_ = false;
  apply_VGPF_ = false;
  apply_VLPF_ = false;
  apply_VTWF_ = false;
}