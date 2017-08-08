#ifndef COMUT_UTILITY_H_
#define COMUT_UTILITY_H_

#include <set>
#include <string>

// Print out each element of a string set in a single line.
void PrintStringSet(std::set<std::string> &string_set);

/**
  Check whether a string is a number (int or real)

  @param  target targeted string
  @return true if it is a number
      		false otherwise
*/
bool StringIsANumber(std::string target);

/** 
  Check whether the domain of an operator specified by user is correct

  @param  mutant_name name of the operator
          domain string set containing targeted tokens
*/
void ValidateDomainOfMutantOperator(std::string &mutant_name, 
																		std::set<std::string> &domain);

/** 
  Check whether the range of an operator specified by user is correct

  @param  mutant_name name of the operator
          range string set containing tokens to mutate with
*/
void ValidateRangeOfMutantOperator(std::string &mutant_name, 
																	 std::set<std::string> &range);

#endif