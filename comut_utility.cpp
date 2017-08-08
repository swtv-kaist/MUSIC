#include "comut_utility.h"
#include <iostream>

// Print out each element of a string set in a single line.
void PrintStringSet(std::set<std::string> &string_set)
{
  for (auto element: string_set)
  {
    std::cout << "//" << element;
  }
  std::cout << "//\n";
}

/**
  Check whether a string is a number (int or real)

  @param  target targeted string
  @return true if it is a number
      		false otherwise
*/
bool StringIsANumber(std::string target)
{
  bool no_dot = true;

  for (int i = 0; i < target.length(); ++i)
  {
    //-------add test of string length to limit number

    if (isdigit(target[i]))
      continue;

    // the dash cannot only appear at the first position
    // and there must be numbers behind it
    if (target[i] == '-' && i == 0 && target.length() > 1)
      continue;

    // prevent cases of 2 dot in a number
    if (target[i] == '.' && no_dot)
    {
      no_dot = false;
      continue;
    }

    return false;
  }
  return true;
}

/** 
  Check whether the domain of an operator specified by user is correct

  @param  mutant_name name of the operator
          domain string set containing targeted tokens
*/
void ValidateDomainOfMutantOperator(std::string &mutant_name, 
																		std::set<std::string> &domain) 
{  
  // set of operators mutating logical operator
  std::set<std::string> logical{"OLLN", "OLAN", "OLBN", "OLRN", "OLSN"};

  // set of operators mutating relational operator
  std::set<std::string> relational{"ORLN", "ORAN", "ORBN", "ORRN", "ORSN"};

  // set of operators mutating arithmetic operator
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

  // set of operators whose domain cannot be altered.
  std::set<std::string> empty_domain{"CRCR", "SSDL", "VLSR", "VGSR", "VGAR", 
  																	 "VLAR", "VGTR", "VLTR", "VGPR", "VLPR", 
  																	 "VSCR", "CGCR", "CLCR", "CGSR", "CLSR", 
  																	 "OMMO", "OPPO", "OLNG", "OCNG", "OBNG", 
  																	 "OBOM", "VTWD", "OIPM", "OCOR", "SANL", 
  																	 "SRWS", "SCSR", "VLSF", "VGSF", "VGTF", 
  																	 "VLTF", "VGPF", "VLPF", "VTWF"};

  // Determine the set of valid tokens based on the type of operator
  std::set<std::string> valid_domain;

  if (logical.find(mutant_name) != logical.end())
  {
    valid_domain = {"&&", "||"};
  }
  else if (relational.find(mutant_name) != relational.end())
  {
    valid_domain = {">", "<", "<=", ">=", "==", "!="};
  } 
  else if (arithmetic.find(mutant_name) != arithmetic.end())
  {
    valid_domain = {"+", "-", "*", "/", "%"};
  }
  else if (bitwise.find(mutant_name) != bitwise.end())
  {
    valid_domain = {"&", "|", "^"};
  }
  else if (shift.find(mutant_name) != shift.end())
  {
    valid_domain = {"<<", ">>"};
  }
  else if (arithmetic_assignment.find(mutant_name) != arithmetic_assignment.end())
  {
    valid_domain = {"+=", "-=", "*=", "/=", "%="};
  }
  else if (bitwise_assignment.find(mutant_name) != bitwise_assignment.end())
  {
    valid_domain = {"&=", "|=", "^="};
  }
  else if (shift_assignment.find(mutant_name) != shift_assignment.end())
  {
    valid_domain = {"<<=", ">>="};
  }
  else if (plain_assignment.find(mutant_name) != plain_assignment.end())
  {
    valid_domain = {"="};
  }
  else if (empty_domain.find(mutant_name) != empty_domain.end())
  {
    if (!domain.empty())
    {
      domain.clear();
      std::cout << "-A options are not available for " << mutant_name << std::endl;
      exit(1);
    }
    return;
  }
  else
  {
    std::cout << "Cannot identify mutant operator " << mutant_name << std::endl;
    exit(1);
  }

  // if domain is not empty, then check if each element in the set is valid
  if (domain.size() > 0)
  {
    for (auto it = domain.begin(); it != domain.end();)
    {
      if ((*it).empty())
        it = domain.erase(it);
      else if (valid_domain.find(*it) == valid_domain.end())
      {
        std::cout << "Mutant operator " << mutant_name << " cannot mutate " << *it << std::endl;
        exit(1);
        // it = domain.erase(it);
      }
      else
        ++it;
    }
  }
  else
  {
    // user did not specify domain for this operator
    // So mutate every valid token for this operator.
    domain = valid_domain;
  }
}

/** 
  Check whether the range of an operator specified by user is correct

  @param  mutant_name name of the operator
          range string set containing tokens to mutate with
*/
void ValidateRangeOfMutantOperator(std::string &mutant_name, 
																	 std::set<std::string> &range) 
{
  // set of operators mutating to a logical operator
  std::set<std::string> logical{"OALN", "OBLN", "OLLN", "ORLN", "OSLN"};

  // set of operators mutating to a relational operator
  std::set<std::string> relational{"OLRN", "OARN", "OBRN", "ORRN", "OSRN"};

  // set of operators mutating to a arithmetic operator
  std::set<std::string> arithmetic{"OLAN", "OAAN", "OBAN", "ORAN", "OSAN"};

  // set of operators mutating to a arithmetic operator
  std::set<std::string> bitwise{"OLBN", "OABN", "OBBN", "ORBN", "OSBN"};

  // set of operators mutating to a arithmetic operator
  std::set<std::string> shift{"OLSN", "OASN", "OBSN", "ORSN", "OSSN"};

  // set of operators mutating to arithmetic assignment operator
  std::set<std::string> arithmetic_assignment{"OAAA", "OBAA", "OEAA", "OSAA"};

  // set of operators mutating to bitwise assignment operator
  std::set<std::string> bitwise_assignment{"OABA", "OBBA", "OEBA", "OSBA"};

  // set of operators mutating to shift assignment operator
  std::set<std::string> shift_assignment{"OASA", "OBSA", "OESA", "OSSA"};

  // set of operators mutating to plain assignment operator
  std::set<std::string> plain_assignment{"OAEA", "OBEA", "OSEA"};

  // set of operators whose range cannot be altered.
  std::set<std::string> empty_range{"SSDL", "VLSR", "VGSR", "VGAR", "VLAR", 
  																	"VGTR", "VLTR", "VGPR", "VLPR", "VSCR", 
  																	"CGCR", "CLCR", "CGSR", "CLSR", "OMMO", 
  																	"OPPO", "OLNG", "OCNG", "OBNG", "OBOM", 
  																	"VTWD", "OIPM", "OCOR", "SANL",  "SRWS", 
  																	"SCSR", "VLSF", "VGSF", "VGTF", "VLTF",  
  																	"VGPF", "VLPF", "VTWF"};

  // Determine the set of valid tokens based on the type of operator
  std::set<std::string> valid_range;  

  if (logical.find(mutant_name) != logical.end())
  {
    valid_range = {"&&", "||"};
  }
  else if (relational.find(mutant_name) != relational.end())
  {
    valid_range = {">", "<", "<=", ">=", "==", "!="};
  } 
  else if (arithmetic.find(mutant_name) != arithmetic.end())
  {
    valid_range = {"+", "-", "*", "/", "%"};
  }
  else if (bitwise.find(mutant_name) != bitwise.end())
  {
    valid_range = {"&", "|", "^"};
  }
  else if (shift.find(mutant_name) != shift.end())
  {
    valid_range = {"<<", ">>"};
  }
  else if (arithmetic_assignment.find(mutant_name) != arithmetic_assignment.end())
  {
    valid_range = {"+=", "-=", "*=", "/=", "%="};
  }
  else if (bitwise_assignment.find(mutant_name) != bitwise_assignment.end())
  {
    valid_range = {"&=", "|=", "^="};
  }
  else if (shift_assignment.find(mutant_name) != shift_assignment.end())
  {
    valid_range = {"<<=", ">>="};
  }
  else if (plain_assignment.find(mutant_name) != plain_assignment.end())
  {
    valid_range = {"="};
  }
  else if (mutant_name.compare("CRCR") == 0)
  {
    if (range.size() > 0)
    {
      for (auto num = range.begin(); num != range.end(); )
      {
        // if user input something not a number, it is ignored. (delete from the set)
        if (!StringIsANumber(*num))
        {
          std::cout << "Warning: " << *num << " is not a number\n";
          exit(1);
          // num = range.erase(num);
        }  
        else
          ++num;
      }
    }
    return;
  }
  else if (empty_range.find(mutant_name) != empty_range.end())
  {
    if (!range.empty())
    {
      range.clear();
      std::cout << "-B options are not available for " << mutant_name << std::endl;
      exit(1);
    }
    return;
  }
  else
  {
    std::cout << "Cannot identify mutant operator " << mutant_name << std::endl;
    exit(1);
  }

  // if range is not empty, then check if each element in the set is valid
  if (range.size() > 0) {
    for (auto it = range.begin(); it != range.end();)
    {
      if ((*it).empty())
        it = range.erase(it);
      else if (valid_range.find(*it) == valid_range.end())
      {
        std::cout << "Mutant operator " << mutant_name << " cannot mutate to " << *it << std::endl;
        exit(1);
        // it = range.erase(it);
      }
      else
        ++it;
    }
  }
  else {
    // user did not specify range for this operator
    // So mutate to every valid token for this operator.
    range = valid_range;
  }
}