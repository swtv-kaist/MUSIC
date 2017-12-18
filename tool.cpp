#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <utility>
#include <iomanip>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <set>
#include <cctype>
#include <limits.h>
#include <time.h>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Frontend/Utils.h"
#include "clang/Sema/Sema.h"

#include "comut_utility.h"
#include "configuration.h"
#include "comut_context.h"
#include "information_visitor.h"
#include "information_gatherer.h"
#include "mutant_database.h"
#include "comut_ast_consumer.h"
#include "all_mutant_operators.h"

// #include <cstring>
// #include <cerrno>

enum class UserInputAnalyzingState
{
  // expecting any not-A-B option
  kNonAOrBOption,

  // expecting a mutant operator name
  kMutantName,

  // expecting a mutant operator name, or another not-A-B option
  kNonAOrBOptionAndMutantName,

  // expecting a directory
  kOutputDir,

  // expecting a number
  kRsLine, kRsColumn,
  kReLine, kReColumn,
  kLimitNumOfMutant,

  // expecting a mutant name or any option
  kAnyOptionAndMutantName,

  // expecting either mutant name or any not-A option
  kNonAOptionAndMutantName,

  // expecting domain for previous mutant operator
  kDomainOfMutantOperator,

  // expecting range for previous mutant operator
  kRangeOfMutantOperator,

  // A_SDL_LINE, A_SDL_COL,  // expecting a number
  // B_SDL_LINE, B_SDL_COL,  // expecting a number
};

CompilerInstance* MakeCompilerInstance(char *filename)
{
  // CompilerInstance will hold the instance of the Clang compiler for us,
  // managing the various objects needed to run the compiler.
  CompilerInstance *TheCompInst = new CompilerInstance();
  
  // Diagnostics manage problems and issues in compile 
  TheCompInst->createDiagnostics(NULL, false);

  // Set target platform options 
  // Initialize target info with the default triple for our platform.
  // TargetOptions *TO = new TargetOptions();
  shared_ptr<TargetOptions> TO(new TargetOptions());
  TO->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI = TargetInfo::CreateTargetInfo(TheCompInst->getDiagnostics(), 
                                                TO);
  TheCompInst->setTarget(TI);

  // FileManager supports for file system lookup, file system caching, 
  // and directory search management.
  TheCompInst->createFileManager();
  FileManager &FileMgr = TheCompInst->getFileManager();
  
  // SourceManager handles loading and caching of source files into memory.
  TheCompInst->createSourceManager(FileMgr);
  SourceManager &SourceMgr = TheCompInst->getSourceManager();
  
  // Prreprocessor runs within a single source file
  TheCompInst->createPreprocessor(TU_Complete);
  
  // ASTContext holds long-lived AST nodes (such as types and decls) .
  TheCompInst->createASTContext();

  // Enable HeaderSearch option
  // llvm::IntrusiveRefCntPtr<clang::HeaderSearchOptions> hso( 
      // new HeaderSearchOptions());

  shared_ptr<HeaderSearchOptions> hso(new HeaderSearchOptions());
  HeaderSearch headerSearch(hso, SourceMgr,
                            TheCompInst->getDiagnostics(),
                            TheCompInst->getLangOpts(), TI);

  // <Warning!!> -- Platform Specific Code lives here
  // This depends on A) that you're running linux and
  // B) that you have the same GCC LIBs installed that I do. 
  /*
  $ gcc -xc -E -v -
  ..
   /usr/local/include
   /usr/lib/gcc/x86_64-linux-gnu/4.4.5/include
   /usr/lib/gcc/x86_64-linux-gnu/4.4.5/include-fixed
   /usr/include
  End of search list.
  */
  const char *include_paths[] = {"/usr/local/include",
      "/usr/lib/gcc/x86_64-linux-gnu/4.4.6/include",
      "/usr/lib/gcc/x86_64-linux-gnu/4.4.6/include-fixed",
      "/usr/include"};

  for (int i=0; i<4; i++) 
    hso->AddPath(include_paths[i], clang::frontend::Angled, 
                 false, false);
  // </Warning!!> -- End of Platform Specific Code

  InitializePreprocessor(TheCompInst->getPreprocessor(), 
                         TheCompInst->getPreprocessorOpts(),
                         TheCompInst->getPCHContainerReader(),
                         TheCompInst->getFrontendOpts());

  // Set the main file Handled by the source manager to the input file.
  const FileEntry *FileIn = FileMgr.getFile(filename);
  FileID FileId = SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User);
  SourceMgr.setMainFileID(FileId);
  // SourceMgr.createMainFileID(FileIn);
  
  // Inform Diagnostics that processing of a source file is beginning. 
  TheCompInst->getDiagnosticClient().BeginSourceFile(
      TheCompInst->getLangOpts(),&TheCompInst->getPreprocessor());

  return TheCompInst;
}

InformationGatherer* GetNecessaryDataFromInputFile(char *filename)
{
  CompilerInstance *TheCompInst = MakeCompilerInstance(filename);

  // Parse the file to AST, gather labelstmts, goto stmts, 
  // scalar constants, string literals. 
  InformationGatherer *TheGatherer = new InformationGatherer(TheCompInst);

  ParseAST(TheCompInst->getPreprocessor(), TheGatherer, 
           TheCompInst->getASTContext());

  return TheGatherer;
}

void AddMutantOperator(string mutant_name, 
                       set<string> &domain, set<string> &range, 
                       vector<StmtMutantOperator*> &stmt_operator_list,
                       vector<ExprMutantOperator*> &expr_operator_list)
{
  // Make appropriate MutantOperator based on name
  // Verifiy and set domain and range accordingly

  StmtMutantOperator *new_stmt_operator = nullptr;

  if (mutant_name.compare("SSDL") == 0)
    new_stmt_operator = new SSDL();
  else if (mutant_name.compare("OCNG") == 0)
    new_stmt_operator = new OCNG();

  if (new_stmt_operator != nullptr)
  {
    // Set domain for mutant operator if domain is valid
    if (!new_stmt_operator->ValidateDomain(domain))
    {
      cout << "invalid domain\n";
      exit(1);
    }
    else
      new_stmt_operator->setDomain(domain);

    // Set range for mutant operator if range is valid
    if (!new_stmt_operator->ValidateRange(range))
    {
      cout << "invalid range\n";
      exit(1);
    }
    else
      new_stmt_operator->setRange(range);

    stmt_operator_list.push_back(new_stmt_operator);
    return;
  }

  ExprMutantOperator *new_expr_operator = nullptr;

  if (mutant_name.compare("ORRN") == 0)
    new_expr_operator = new ORRN();
  else if (mutant_name.compare("VTWF") == 0)
    new_expr_operator = new VTWF();
  else if (mutant_name.compare("CRCR") == 0)
    new_expr_operator = new CRCR();
  else if (mutant_name.compare("SANL") == 0)
    new_expr_operator = new SANL();
  else if (mutant_name.compare("SRWS") == 0)
    new_expr_operator = new SRWS();
  else if (mutant_name.compare("SCSR") == 0)
    new_expr_operator = new SCSR();
  else if (mutant_name.compare("VLSF") == 0)
    new_expr_operator = new VLSF();
  else if (mutant_name.compare("VGSF") == 0)
    new_expr_operator = new VGSF();
  else if (mutant_name.compare("VLTF") == 0)
    new_expr_operator = new VLTF();
  else if (mutant_name.compare("VGTF") == 0)
    new_expr_operator = new VGTF();
  else if (mutant_name.compare("VLPF") == 0)
    new_expr_operator = new VLPF();
  else if (mutant_name.compare("VGPF") == 0)
    new_expr_operator = new VGPF();
  else if (mutant_name.compare("VGSR") == 0)
    new_expr_operator = new VGSR();
  else if (mutant_name.compare("VLSR") == 0)
    new_expr_operator = new VLSR();
  else if (mutant_name.compare("VGAR") == 0)
    new_expr_operator = new VGAR();
  else if (mutant_name.compare("VLAR") == 0)
    new_expr_operator = new VLAR();
  else if (mutant_name.compare("VGTR") == 0)
    new_expr_operator = new VGTR();
  else if (mutant_name.compare("VLTR") == 0)
    new_expr_operator = new VLTR();
  else if (mutant_name.compare("VGPR") == 0)
    new_expr_operator = new VGPR();
  else if (mutant_name.compare("VLPR") == 0)
    new_expr_operator = new VLPR();
  else if (mutant_name.compare("VTWD") == 0)
    new_expr_operator = new VTWD();
  else if (mutant_name.compare("VSCR") == 0)
    new_expr_operator = new VSCR();
  else if (mutant_name.compare("CGCR") == 0)
    new_expr_operator = new CGCR();
  else if (mutant_name.compare("CLCR") == 0)
    new_expr_operator = new CLCR();
  else if (mutant_name.compare("CGSR") == 0)
    new_expr_operator = new CGSR();
  else if (mutant_name.compare("CLSR") == 0)
    new_expr_operator = new CLSR();
  else if (mutant_name.compare("OPPO") == 0)
    new_expr_operator = new OPPO();
  else if (mutant_name.compare("OMMO") == 0)
    new_expr_operator = new OMMO();
  else if (mutant_name.compare("OLNG") == 0)
    new_expr_operator = new OLNG();
  else if (mutant_name.compare("OBNG") == 0)
    new_expr_operator = new OBNG();
  else if (mutant_name.compare("OIPM") == 0)
    new_expr_operator = new OIPM();
  else if (mutant_name.compare("OCOR") == 0)
    new_expr_operator = new OCOR();
  else if (mutant_name.compare("OLLN") == 0)
    new_expr_operator = new OLLN();
  else if (mutant_name.compare("OSSN") == 0)
    new_expr_operator = new OSSN();
  else if (mutant_name.compare("OBBN") == 0)
    new_expr_operator = new OBBN();
  else if (mutant_name.compare("OLRN") == 0)
    new_expr_operator = new OLRN();
  else if (mutant_name.compare("ORLN") == 0)
    new_expr_operator = new ORLN();
  else if (mutant_name.compare("OBLN") == 0)
    new_expr_operator = new OBLN();
  else if (mutant_name.compare("OBRN") == 0)
    new_expr_operator = new OBRN();
  else if (mutant_name.compare("OSLN") == 0)
    new_expr_operator = new OSLN();
  else if (mutant_name.compare("OSRN") == 0)
    new_expr_operator = new OSRN();
  else if (mutant_name.compare("OBAN") == 0)
    new_expr_operator = new OBAN();
  else if (mutant_name.compare("OBSN") == 0)
    new_expr_operator = new OBSN();
  else if (mutant_name.compare("OSAN") == 0)
    new_expr_operator = new OSAN();
  else if (mutant_name.compare("OSBN") == 0)
    new_expr_operator = new OSBN();
  else if (mutant_name.compare("OAEA") == 0)
    new_expr_operator = new OAEA();
  else if (mutant_name.compare("OBAA") == 0)
    new_expr_operator = new OBAA();
  else if (mutant_name.compare("OBBA") == 0)
    new_expr_operator = new OBBA();
  else if (mutant_name.compare("OBEA") == 0)
    new_expr_operator = new OBEA();
  else if (mutant_name.compare("OBSA") == 0)
    new_expr_operator = new OBSA();
  else if (mutant_name.compare("OSAA") == 0)
    new_expr_operator = new OSAA();
  else if (mutant_name.compare("OSBA") == 0)
    new_expr_operator = new OSBA();
  else if (mutant_name.compare("OSEA") == 0)
    new_expr_operator = new OSEA();
  else if (mutant_name.compare("OSSA") == 0)
    new_expr_operator = new OSSA();
  else if (mutant_name.compare("OEAA") == 0)
    new_expr_operator = new OEAA();
  else if (mutant_name.compare("OEBA") == 0)
    new_expr_operator = new OEBA();
  else if (mutant_name.compare("OESA") == 0)
    new_expr_operator = new OESA();
  else if (mutant_name.compare("OAAA") == 0)
    new_expr_operator = new OAAA();
  else if (mutant_name.compare("OABA") == 0)
    new_expr_operator = new OABA();
  else if (mutant_name.compare("OASA") == 0)
    new_expr_operator = new OASA();
  else if (mutant_name.compare("OALN") == 0)
    new_expr_operator = new OALN();
  else if (mutant_name.compare("OAAN") == 0)
    new_expr_operator = new OAAN();
  else if (mutant_name.compare("OARN") == 0)
    new_expr_operator = new OARN();
  else if (mutant_name.compare("OABN") == 0)
    new_expr_operator = new OABN();
  else if (mutant_name.compare("OASN") == 0)
    new_expr_operator = new OASN();
  else if (mutant_name.compare("OLAN") == 0)
    new_expr_operator = new OLAN();
  else if (mutant_name.compare("ORAN") == 0)
    new_expr_operator = new ORAN();
  else if (mutant_name.compare("OLBN") == 0)
    new_expr_operator = new OLBN();
  else if (mutant_name.compare("OLSN") == 0)
    new_expr_operator = new OLSN();
  else if (mutant_name.compare("ORSN") == 0)
    new_expr_operator = new ORSN();
  else if (mutant_name.compare("ORBN") == 0)
    new_expr_operator = new ORBN();
  else
  {
    cout << "Unknown mutant operator: " << mutant_name << endl;
    return;
  }

  if (new_expr_operator != nullptr)
  {
    // Set domain for mutant operator if domain is valid
    if (!new_expr_operator->ValidateDomain(domain))
    {
      cout << "invalid domain\n";
      exit(1);
    }
    else
      new_expr_operator->setDomain(domain);

    // Set range for mutant operator if range is valid
    if (!new_expr_operator->ValidateRange(range))
    {
      cout << "invalid range\n";
      exit(1);
    }
    else
      new_expr_operator->setRange(range);

    expr_operator_list.push_back(new_expr_operator);
  }
}

void AddAllMutantOperator(vector<StmtMutantOperator*> &stmt_operator_list,
                          vector<ExprMutantOperator*> &expr_operator_list)
{
  set<string> domain;
  set<string> range;

  set<string> stmt_mutant_operators{"SSDL", "OCNG"};
  set<string> expr_mutant_operators{
      "ORRN", "VTWF", "CRCR", "SANL", "SRWS", "SCSR", "VLSF", "VGSF", 
      "VLTF", "VGTF", "VLPF", "VGPF", "VGSR", "VLSR", "VGAR", "VLAR", 
      "VGTR", "VLTR", "VGPR", "VLPR", "VTWD", "VSCR", "CGCR", "CLCR", 
      "CGSR", "CLSR", "OPPO", "OMMO", "OLNG", "OBNG", "OIPM", "OCOR", 
      "OLLN", "OSSN", "OBBN", "OLRN", "ORLN", "OBLN", "OBRN", "OSLN", 
      "OSRN", "OBAN", "OBSN", "OSAN", "OSBN", "OAEA", "OBAA", "OBBA", 
      "OBEA", "OBSA", "OSAA", "OSBA", "OSEA", "OSSA", "OEAA", "OEBA", 
      "OESA", "OAAA", "OABA", "OASA", "OALN", "OAAN", "OARN", "OABN", 
      "OASN", "OLAN", "ORAN", "OLBN", "OLSN", "ORSN", "ORBN"};

  for (auto mutant_name: stmt_mutant_operators)
    AddMutantOperator(mutant_name, domain, range, stmt_operator_list, 
                      expr_operator_list);

  for (auto mutant_name: expr_mutant_operators)
    AddMutantOperator(mutant_name, domain, range, stmt_operator_list, 
                      expr_operator_list);
}

// Wrap up currently-entering mutant operator (if can)
// before change the state to kNonAOrBOption.
void ClearState(UserInputAnalyzingState &state, string mutant_name, 
                set<string> &domain, set<string> &range,
                vector<StmtMutantOperator*> &stmt_operator_list,
                vector<ExprMutantOperator*> &expr_operator_list)
{
  switch (state)
  {
    case UserInputAnalyzingState::kAnyOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOrBOptionAndMutantName:
    {
      AddMutantOperator(mutant_name, domain, range, 
                        stmt_operator_list, expr_operator_list);

      domain.clear();
      range.clear();

      break;
    }
    case UserInputAnalyzingState::kNonAOrBOption:
      break;
    default:
      PrintUsageErrorMsg();
      exit(1);
  };

  state = UserInputAnalyzingState::kNonAOrBOption;
}

void HandleInputArgument(string input, UserInputAnalyzingState &state, 
                         string &mutant_name, set<string> &domain,
                         set<string> &range, string &output_dir, int &limit, 
                         SourceLocation *start_loc, SourceLocation *end_loc, 
                         int &line_num, int &col_num, SourceManager &src_mgr, 
                         vector<StmtMutantOperator*> &stmt_operator_list,
                         vector<ExprMutantOperator*> &expr_operator_list)
{
  switch (state)
  {
    case UserInputAnalyzingState::kNonAOrBOption:
      PrintUsageErrorMsg();
      exit(1);

    case UserInputAnalyzingState::kMutantName:
      for (int i = 0; i < input.length() ; ++i)
      {
        if (input[i] >= 'a' && input[i] <= 'z')
          input[i] -= 32;
      }

      mutant_name.clear();
      mutant_name = input;

      state = UserInputAnalyzingState::kAnyOptionAndMutantName;
      break;

    case UserInputAnalyzingState::kOutputDir:
      output_dir.clear();
      output_dir = input + "/";
      
      if (!DirectoryExists(output_dir)) 
      {
        cout << "Invalid directory for -o option: " << output_dir << endl;
        exit(1);
      }

      state = UserInputAnalyzingState::kNonAOrBOption;
      break;

    case UserInputAnalyzingState::kRsLine:
      if (NumIsFloat(input) || !ConvertStringToInt(input, line_num))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      if (line_num <= 0)
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      state = UserInputAnalyzingState::kRsColumn;
      break;

    case UserInputAnalyzingState::kRsColumn:
      if (NumIsFloat(input) || !ConvertStringToInt(input, col_num))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      *start_loc = src_mgr.translateLineCol(src_mgr.getMainFileID(), 
                                            line_num, col_num);

      if (col_num <= 0 || GetLineNumber(src_mgr, *start_loc) < line_num ||
          GetColumnNumber(src_mgr, *start_loc) < col_num)
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      state = UserInputAnalyzingState::kNonAOrBOption;
      break;

    case UserInputAnalyzingState::kReLine:
      if (NumIsFloat(input) || !ConvertStringToInt(input, line_num))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      if (line_num <= 0)
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      state = UserInputAnalyzingState::kReColumn;
      break;

    case UserInputAnalyzingState::kReColumn:
      if (NumIsFloat(input) || !ConvertStringToInt(input, col_num))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      *end_loc = src_mgr.translateLineCol(src_mgr.getMainFileID(), 
                                          line_num, col_num);

      if (col_num <= 0 || GetLineNumber(src_mgr, *end_loc) < line_num ||
          GetColumnNumber(src_mgr, *end_loc) < col_num)
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      state = UserInputAnalyzingState::kNonAOrBOption;
      break;

    case UserInputAnalyzingState::kLimitNumOfMutant:
      if (NumIsFloat(input) || !ConvertStringToInt(input, limit))
      {
        cout << "Invalid input for -l option, must be an positive integer smaller than 4294967295\n";
        cout << "Usage: -l <max>\n";
        exit(1);
      }

      if (limit <= 0)
      {
        cout << "Invalid input for -l option, must be an positive integer smaller than 4294967295\n";
        cout << "Usage: -l <max>\n";
        exit(1);
      }      

      state = UserInputAnalyzingState::kNonAOrBOption;
      break;

    case UserInputAnalyzingState::kAnyOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOptionAndMutantName:
    case UserInputAnalyzingState::kNonAOrBOptionAndMutantName:
    {
      AddMutantOperator(mutant_name, domain, range, 
                        stmt_operator_list, expr_operator_list);

      state = UserInputAnalyzingState::kMutantName;
      domain.clear();
      range.clear();

      HandleInputArgument(input, state, mutant_name, domain, range, 
                          output_dir, limit, start_loc, end_loc, line_num, 
                          col_num, src_mgr, stmt_operator_list,
                          expr_operator_list);
      break;
    }

    case UserInputAnalyzingState::kDomainOfMutantOperator:
      SplitStringIntoSet(input, domain, string(","));
      // ValidateDomainOfMutantOperator(mutant_name, domain);
      state = UserInputAnalyzingState::kNonAOptionAndMutantName;
      break;

    case UserInputAnalyzingState::kRangeOfMutantOperator:
      SplitStringIntoSet(input, range, string(","));
      // ValidateRangeOfMutantOperator(mutant_name, range);
      state = UserInputAnalyzingState::kNonAOrBOptionAndMutantName;
      break;

    default:
      // cout << "unknown state: " << state << endl;
      exit(1);
  };
}

void AnalyzeUserInput(int argc, char *argv[], UserInputAnalyzingState &state,
                      string &mutant_name, set<string> &domain,
                      set<string> &range, string &output_dir, int &limit, 
                      SourceLocation *start_loc, SourceLocation *end_loc, 
                      int &line_num, int &col_num, SourceManager &src_mgr, 
                      vector<StmtMutantOperator*> &stmt_operator_list,
                      vector<ExprMutantOperator*> &expr_operator_list)
{
  // Analyze each user command argument from left to right
  for (int i = 2; i < argc; ++i)
  {
    string option = argv[i];
    cout << "handling " << option << endl;

    /* Option -m signals that the next input should be 
       a mutant operator name. User might have used option -m
       before, so we should call ClearState to add previously stored 
       mutant operator (if exists) and clear temporary storing variable. */
    if (option.compare("-m") == 0)
    {
      ClearState(state, mutant_name, domain, range, 
                 stmt_operator_list, expr_operator_list);
      
      mutant_name.clear();
      domain.clear();
      range.clear();
      
      // Signals expectation of a mutant operator name
      state = UserInputAnalyzingState::kMutantName;
    }
    // 
    else if (option.compare("-l") == 0)
    {
      ClearState(state, mutant_name, domain, range, 
                 stmt_operator_list, expr_operator_list);
      state = UserInputAnalyzingState::kLimitNumOfMutant;
    }
    else if (option.compare("-rs") == 0)
    {
      ClearState(state, mutant_name, domain, range, 
                 stmt_operator_list, expr_operator_list);
      state = UserInputAnalyzingState::kRsLine;
    }
    else if (option.compare("-re") == 0)
    {
      ClearState(state, mutant_name, domain, range, 
                 stmt_operator_list, expr_operator_list);
      state = UserInputAnalyzingState::kReLine;
    }
    else if (option.compare("-o") == 0)
    {
      ClearState(state, mutant_name, domain, range, 
                 stmt_operator_list, expr_operator_list);
      state = UserInputAnalyzingState::kOutputDir;
    }
    else if (option.compare("-A") == 0)
    {
      if (state != UserInputAnalyzingState::kAnyOptionAndMutantName)
      {
        // not expecting -A at this position
        PrintUsageErrorMsg();
        exit(1);
      }
      else
        state = UserInputAnalyzingState::kDomainOfMutantOperator;
    }
    else if (option.compare("-B") == 0)
    {
      if (state != UserInputAnalyzingState::kNonAOptionAndMutantName && 
          state != UserInputAnalyzingState::kAnyOptionAndMutantName)
      {
        // not expecting -B at this position
        PrintUsageErrorMsg();
        exit(1);
      }
      else
        state = UserInputAnalyzingState::kRangeOfMutantOperator;
    }
    else
    {
      HandleInputArgument(option, state, mutant_name, domain, range,
                          output_dir, limit, start_loc, end_loc, line_num, 
                          col_num, src_mgr, stmt_operator_list, 
                          expr_operator_list);
    }
  }

  ClearState(state, mutant_name, domain, range, 
             stmt_operator_list, expr_operator_list);
}

static llvm::cl::OptionCategory ComutOptions("COMUT options");
static llvm::cl::extrahelp CommonHelp(tooling::CommonOptionsParser::HelpMessage);

// static llvm::cl::extrahelp MoreHelp("\nMore help text...");

static llvm::cl::list<string> OptionM(
    "m", llvm::cl::desc("Specify mutant operator name to use"), 
    llvm::cl::value_desc("mutantname"),
    llvm::cl::cat(ComutOptions));

static llvm::cl::opt<string> OptionO("o", llvm::cl::cat(ComutOptions));

static llvm::cl::opt<unsigned int> OptionL(
    "l", llvm::cl::init(UINT_MAX), llvm::cl::cat(ComutOptions));

static llvm::cl::list<unsigned int> OptionRS(
    "rs", llvm::cl::multi_val(2),
    llvm::cl::cat(ComutOptions));
static llvm::cl::list<unsigned int> OptionRE(
    "re", llvm::cl::multi_val(2),
    llvm::cl::cat(ComutOptions));

static llvm::cl::opt<string> OptionA("A", llvm::cl::cat(ComutOptions));
static llvm::cl::opt<string> OptionB("B", llvm::cl::cat(ComutOptions));

InformationGatherer *g_gatherer;
CompilerInstance *g_CI;
Configuration *g_config;
MutantDatabase *g_mutant_database;
ComutContext *g_comut_context;
vector<ExprMutantOperator*> g_expr_mutant_operator_list;
vector<StmtMutantOperator*> g_stmt_mutant_operator_list;
tooling::CommonOptionsParser *g_option_parser;

// default output directory is current directory.
string g_output_dir = "./";

/* By default, as many mutants will be generated 
     at a location per mutant operator as possible. */
int g_limit = UINT_MAX;

string g_inputfile_name, g_mutdbfile_name;
string g_current_inputfile_path;
SourceLocation g_mutation_range_start;
SourceLocation g_mutation_range_end;

inline bool exists_test3 (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

class GenerateMutantAction : public ASTFrontendAction
{
protected:
  void ExecuteAction() override
  {
    // CompilerInstance &CI = getCompilerInstance();
    // HeaderSearchOptions &hso = CI.getHeaderSearchOpts();
    
    // const char *include_paths[] = {
    //     // "/usr/local/include",
    //     "/usr/lib/gcc/x86_64-linux-gnu/6/include"
    //     // "/usr/lib/gcc/x86_64-linux-gnu/6/include-fixed",
    //     /*"/usr/include"*/};

    // for (int i=0; i<1; i++) 
    //   hso.AddPath(include_paths[i], clang::frontend::Angled, 
    //               false, false);

    cout << "executing action from GenerateMutantAction\n";
    ASTFrontendAction::ExecuteAction();
    cout << "done execute action\n";

    //=================================================
    //==================== OUTPUT =====================
    //=================================================
    /* Open the file with mode TRUNC to create the file if not existed
    or delete content if existed. */
    string filename{"/home/duyloc1503/comut-libtool/src/" + g_mutdbfile_name};

    ofstream out_mutDb(g_mutdbfile_name.data(), ios::trunc);

    if (!out_mutDb.is_open())
      std::cerr<<"Failed to open file : "<<strerror(errno)<<std::endl;
    else
      cout << "opened file name " << g_mutdbfile_name << endl;

    out_mutDb.close();

    g_mutant_database->ExportAllEntries();
    // g_mutant_database->WriteAllEntriesToDatabaseFile();
  }

  // bool shouldEraseOutputFiles() override
  // {
  //   cout << "shouldEraseOutputFiles 2 called\n";
  //   bool ret = getCompilerInstance().getDiagnostics().hasErrorOccurred();
  //   cout << "return value should have been " << ret << endl;
  //   return ret;
  // }

public:
  virtual unique_ptr<ASTConsumer> CreateASTConsumer(
      CompilerInstance &CI, llvm::StringRef InFile)
  {
    // Parse rs and re option.
    SourceManager &sm = CI.getSourceManager();
    g_mutation_range_start = sm.getLocForStartOfFile(sm.getMainFileID());
    g_mutation_range_end = sm.getLocForEndOfFile(sm.getMainFileID());

    if (!OptionRS.empty())
    {
      if (OptionRS[0] == 0 || OptionRE[0] == 0)
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      SourceLocation interpreted_loc = sm.translateLineCol(
          sm.getMainFileID(), OptionRS[0], OptionRS[1]);

      if (OptionRS[0] != GetLineNumber(sm, interpreted_loc) ||
          OptionRS[1] != GetColumnNumber(sm, interpreted_loc))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      g_mutation_range_start = sm.translateLineCol(
          sm.getMainFileID(), OptionRS[0], OptionRS[1]);
    }

    cout << "done parsing rs\n";

    if (!OptionRE.empty())
    {
      if (OptionRE[0] == 0 || OptionRE[1] == 0)
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      SourceLocation interpreted_loc = sm.translateLineCol(
          sm.getMainFileID(), OptionRE[0], OptionRE[1]);

      if (OptionRE[0] != GetLineNumber(sm, interpreted_loc) ||
          OptionRE[1] != GetColumnNumber(sm, interpreted_loc))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      g_mutation_range_end = sm.translateLineCol(
          sm.getMainFileID(), OptionRE[0], OptionRE[1]);
    }

    /* Create Configuration object pointer to pass as attribute 
       for ComutASTConsumer. */
    g_config = new Configuration(
        g_inputfile_name, g_mutdbfile_name, g_mutation_range_start, 
        g_mutation_range_end, g_output_dir, g_limit);

    g_mutant_database = new MutantDatabase(
        &CI, g_config->getInputFilename(),
        g_config->getOutputDir());

    g_comut_context = new ComutContext(
        &CI, g_config, g_gatherer->getLabelToGotoListMap(),
        g_gatherer->getSymbolTable(), *g_mutant_database);

    return unique_ptr<ASTConsumer>(new ComutASTConsumer(
        &CI, g_gatherer->getLabelToGotoListMap(),
        g_stmt_mutant_operator_list,
        g_expr_mutant_operator_list, *g_comut_context));
  }
};

class GatherDataAction : public ASTFrontendAction
{
protected:
  void ExecuteAction() override
  {
    CompilerInstance &CI = getCompilerInstance();
    HeaderSearchOptions &hso = CI.getHeaderSearchOpts();
    
    // const char *include_paths[] = {
    //     // "/usr/local/include",
    //     "/usr/lib/gcc/x86_64-linux-gnu/6/include"
    //     // "/usr/lib/gcc/x86_64-linux-gnu/6/include-fixed",
    //     "/usr/include"};

    // vector<string> include_paths{
    //     "/usr/lib/gcc/x86_64-linux-gnu/6/include"
    // };

    // for (int i=0; i<1; i++) 
    //   hso.AddPath(include_paths[i], clang::frontend::Angled, 
    //               false, false);

    // for (auto path: include_paths)
    //   hso.AddPath(path, frontend::System, false, false);

    cout << "executing action from GatherDataAction\n";
    ASTFrontendAction::ExecuteAction();

    cout << g_gatherer->getLabelToGotoListMap()->size() << endl;

    vector<string> source{g_current_inputfile_path};

    tooling::ClangTool Tool2(g_option_parser->getCompilations(),
                             source);

    Tool2.run(tooling::newFrontendActionFactory<GenerateMutantAction>().get());
  }

  // bool shouldEraseOutputFiles() override
  // {
  //   cout << "shouldEraseOutputFiles 1 called\n";
  //   return false;
  // }

public:  
  virtual unique_ptr<ASTConsumer> CreateASTConsumer(
      CompilerInstance &CI, llvm::StringRef InFile)
  {
    g_CI = &CI;
    g_gatherer = new InformationGatherer(&CI);
    return unique_ptr<ASTConsumer>(g_gatherer);
  }
};

int main(int argc, const char *argv[])
{
  // cout << OptionL << endl;

  g_option_parser = new tooling::CommonOptionsParser(argc, argv, ComutOptions);

  // Parse option -o (if provided)
  // Terminate tool if given output directory does not exist.
  if (!OptionO.empty())
  {
    if (DirectoryExists(OptionO))
      g_output_dir = OptionO;
    else
    {
      cout << "Invalid directory for -o option: " << OptionO << endl;
      exit(1);
    }
  }

  if (g_output_dir.back() != '/')
    g_output_dir += "/";

  cout << "done with option o: " << g_output_dir << "\n";

  // Parse option -l (if provided)
  // Given input should be a positive integer.
  if (OptionL != UINT_MAX)
  {
    if (OptionL != 0)
      g_limit = OptionL;
    else
    {
      cout << "Invalid input for -l option, must be an positive integer smaller than 4294967296\n";
      cout << "Usage: -l <max>\n";
      exit(1);
    }
  }

  cout << "done with option l: " << g_limit << "\n";

  set<string> domain, range;

  // Parse option -m (if provided)
  if (OptionM.empty())
    AddAllMutantOperator(g_stmt_mutant_operator_list, 
                         g_expr_mutant_operator_list);
  else
    for (auto mutant_operator_name: OptionM)
    {
      for (int i = 0; i < mutant_operator_name.length() ; ++i)
      {
        if (mutant_operator_name[i] >= 'a' && 
            mutant_operator_name[i] <= 'z')
          mutant_operator_name[i] -= 32;
      }

      AddMutantOperator(mutant_operator_name, domain, range,
                        g_stmt_mutant_operator_list,
                        g_expr_mutant_operator_list);
    }

  cout << "done with option m\n";

  ofstream my_file("/home/duyloc1503/comut-libtool/multiple-compile-command-files.txt", ios::trunc);    

  /* Run tool separately for each input file. */
  for (auto file: g_option_parser->getSourcePathList())
  { 
    // cout << "Running COMUT on " << file << endl;

    if (g_option_parser->getCompilations().getCompileCommands(file).size() > 1)
    {
      cout << "This file has more than 1 compile commands\n" << file << endl;

      for (auto e: g_option_parser->getCompilations().getCompileCommands(file))
      {
        for (auto command: e.CommandLine)
          cout << command << " ";
        cout << endl;
      } 

      // getchar();
      continue;
    }

    // Print all compilation for this file
    int counter = 0;
    for (auto e: g_option_parser->getCompilations().getCompileCommands(file))
    {
      counter++;
      cout << "==========" << counter << "==========\n";
      cout << e.Directory << endl;
      cout << e.Filename << endl;
      for (auto command: e.CommandLine)
        cout << command << " ";
      cout << endl;
      cout << "=====================\n";
    }

    // getchar();  // pause the program 
    g_current_inputfile_path = file;

    // inputfile name is the string after the last slash (/)
    // in the provided path to inputfile. 
    string inputfile_path = file;
    vector<string> path;
    SplitStringIntoVector(inputfile_path, path, string("/"));
    g_inputfile_name = path.back();

    // Make mutation database file named <inputfilename>_mut_db.out
    g_mutdbfile_name = g_output_dir;

    if (g_mutdbfile_name.back() != '/')
      g_mutdbfile_name += "/";

    g_mutdbfile_name.append(g_inputfile_name, 0, g_inputfile_name.length()-2);
    g_mutdbfile_name += "_mut_db.out";

    cout << "g_inputfile_name = " << g_inputfile_name << endl;
    cout << "g_mutdbfile_name = " << g_mutdbfile_name << endl;

    vector<string> source{g_current_inputfile_path};
  
    // Run tool
    tooling::ClangTool Tool1(g_option_parser->getCompilations(),
                             source);

    Tool1.run(tooling::newFrontendActionFactory<GatherDataAction>().get());

    cout << "done tooling on " << file << endl;
  }

  my_file.close();

  return 0;
}
