#include "../music_utility.h"
#include "mutant_operator_template.h"

void MutantOperatorTemplate::setDomain(set<string> &domain)
{
  domain_ = domain;
}

void MutantOperatorTemplate::setRange(set<string> &range)
{
  range_ = range;
}

string MutantOperatorTemplate::getName()
{
  return name_;
}

bool MutantOperatorTemplate::IsInitMutationTarget(clang::Expr *e, MusicContext *context)
{
  return false;
}
