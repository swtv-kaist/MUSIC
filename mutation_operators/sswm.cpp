#include "../music_utility.h"
#include "sswm.h"

/* The domain for SSWM must be names of functions whose
   function calls will be mutated. */
bool SSWM::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool SSWM::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void SSWM::setRange(std::set<std::string> &range) {}

// Return True if the mutant operator can mutate this expression
bool SSWM::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  if (!isa<SwitchStmt>(s))
    return false;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = GetLocationAfterSemicolon(
      src_mgr, 
      TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc));
}

void SSWM::Mutate(clang::Stmt *s, MusicContext *context)
{ 
  SwitchStmt *ss = nullptr;
  if (!(ss = dyn_cast<SwitchStmt>(s)))
    return;

  SwitchCase *sc = ss->getSwitchCaseList();
  if (sc == nullptr)
    return;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();

  SourceLocation start_of_file = src_mgr.getLocForStartOfFile(src_mgr.getMainFileID());
  vector<string> extra_tokens{""};
  vector<string> extra_mutated_tokens{"#include <sys/types.h>\n#include <signal.h>\n#include <unistd.h>\n"};
  vector<SourceLocation> extra_start_locs{start_of_file};
  vector<SourceLocation> extra_end_locs{start_of_file};

  while (sc != nullptr)
  {
    SourceLocation start_loc = sc->getBeginLoc();
    SourceLocation end_loc = sc->getColonLoc().getLocWithOffset(1);
    SourceLocation temp_loc = start_loc;
    string token{""};

    // Getting token this way because ConvertToString may include the sub-stmt
    while (temp_loc != end_loc)
    {
      token += (*(src_mgr.getCharacterData(temp_loc)));
      temp_loc = temp_loc.getLocWithOffset(1);
    }

    string mutated_token{token + " kill(getpid(), 9); MUSIC_SSWM:;\n"};

    SwitchCase *next_sc = sc->getNextSwitchCase();
    if (next_sc != nullptr)
      mutated_token = "goto MUSIC_SSWM;\n" + mutated_token;

    context->mutant_database_.AddMutantEntry(context->getStmtContext(),
        name_, start_loc, end_loc, token, mutated_token, 
        context->getStmtContext().getProteumStyleLineNum(), "",
        extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);

    sc = sc->getNextSwitchCase();
  }
}
