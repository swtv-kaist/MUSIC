#include "../music_utility.h"
#include "strp.h"

/* The domain for STRP must be names of functions whose
   function calls will be mutated. */
bool STRP::ValidateDomain(const std::set<std::string> &domain)
{
  return true;
}

bool STRP::ValidateRange(const std::set<std::string> &range)
{
  // for (auto e: range)
 //    if (!IsValidVariableName(e))
 //      return false;

  return true;
}

void STRP::setRange(std::set<std::string> &range) {}

// Return True if the mutant operator can mutate this expression
bool STRP::IsMutationTarget(clang::Stmt *s, MusicContext *context)
{
  if (isa<DeclStmt>(s) || isa<NullStmt>(s))
    return false;

  const Stmt* parent = GetParentOfStmt(s, context->comp_inst_);

  if (!parent)
    return false;

  if (!isa<CompoundStmt>(parent))
    return false;

  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = GetLocationAfterSemicolon(
      src_mgr, 
      TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

  return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
         NoUnremovableLabelInsideRange(src_mgr,
                                       SourceRange(start_loc, end_loc),
                                       context->label_to_gotolist_map_);
}

void STRP::Mutate(clang::Stmt *s, MusicContext *context)
{ 
  if (LabelStmt *ls = dyn_cast<LabelStmt>(s))
  {
    Mutate(ls->getSubStmt(), context);
    return;
  }

  if (DefaultStmt *ds = dyn_cast<DefaultStmt>(s))
  {
    Mutate(ds->getSubStmt(), context);
    return;
  }

  if (SwitchCase *sc = dyn_cast<SwitchCase>(s))
  {
    Mutate(sc->getSubStmt(), context); 
    return;
  }

  if (ForStmt *fs = dyn_cast<ForStmt>(s))
    if (fs->getBody() && !isa<CompoundStmt>(fs->getBody()))
      Mutate(fs->getBody(), context);
  
  if (WhileStmt *ws = dyn_cast<WhileStmt>(s))
    if (ws->getBody() && !isa<CompoundStmt>(ws->getBody()))
      Mutate(ws->getBody(), context);
  
  if (DoStmt *ds = dyn_cast<DoStmt>(s))
    if (ds->getBody() && !isa<CompoundStmt>(ds->getBody()))
      Mutate(ds->getBody(), context);
  
  if (IfStmt *is = dyn_cast<IfStmt>(s))
  {
    if (is->getThen() && !isa<CompoundStmt>(is->getThen()))
      Mutate(is->getThen(), context);

    if (Stmt *else_stmt = is ->getElse())
      Mutate(else_stmt, context);
  }

  // cout << "STRP:\n" << ConvertToString(s, context->comp_inst_->getLangOpts()) << endl;
  SourceManager &src_mgr = context->comp_inst_->getSourceManager();
  SourceLocation start_loc = s->getBeginLoc();
  SourceLocation end_loc = GetLocationAfterSemicolon(
      src_mgr, 
      TryGetEndLocAfterBracketOrSemicolon(s->getEndLoc(), context->comp_inst_));

  if (!context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) ||
      !NoUnremovableLabelInsideRange(src_mgr,
                                     SourceRange(start_loc, end_loc),
                                     context->label_to_gotolist_map_))
    return;


  string token{ConvertToString(s, context->comp_inst_->getLangOpts())};
  string mutated_token{"kill(getpid(), 9);"};

  SourceLocation start_of_file = src_mgr.getLocForStartOfFile(src_mgr.getMainFileID());
  vector<string> extra_tokens{""};
  vector<string> extra_mutated_tokens{"#include <sys/types.h>\n#include <signal.h>\n#include <unistd.h>\n"};
  vector<SourceLocation> extra_start_locs{start_of_file};
  vector<SourceLocation> extra_end_locs{start_of_file};

  context->mutant_database_.AddMutantEntry(context->getStmtContext(),
      name_, start_loc, end_loc, token, mutated_token, 
      context->getStmtContext().getProteumStyleLineNum(), "",
      extra_tokens, extra_mutated_tokens, extra_start_locs, extra_end_locs);
}

bool STRP::NoUnremovableLabelInsideRange(
  SourceManager &src_mgr, SourceRange range, 
  LabelStmtToGotoStmtListMap *label_map)
{
  auto it = label_map->begin();

  for (; it != label_map->end(); ++it)
  {
    // cout << "cp ssdl\n";

    SourceLocation loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(),
      it->first.first,
      it->first.second);

    // check only those labels inside range
    if (LocationIsInRange(loc, range))
    {
      // check if this label is goto-ed from outside of the statement
      // if yes, then the label is unremovable, return False
      for (auto e: it->second)  
      {   
        // the goto is outside of the statement
        if (!LocationIsInRange(e, range))  
          return false;
      }
    }

    // the LabelStmtToGotoStmtListMap is traversed in the order of 
    // increasing location. If the location is after the range then 
    // all the rest is outside statement range
    if (LocationAfterRangeEnd(loc, range))
      return true;
  }

  return true;
}
