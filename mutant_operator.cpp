#include "mutant_operator.h"

MutantOperator::MutantOperator(std::string name, 
                               std::set<std::string> domain, 
                               std::set<std::string> range)
  :mutant_name_(name), domain_(domain), range_(range)
{
  /*cout << "using mutant operator " << name << endl;
  cout << "Domain: "; PrintStringSet(domain_);
  cout << "Range:  "; PrintStringSet(range_);*/
} 

MutantOperator::~MutantOperator() { }

std::string& MutantOperator::getName() 
{
  return mutant_name_;
}
