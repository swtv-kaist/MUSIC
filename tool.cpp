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

#include "music_utility.h"
#include "configuration.h"
#include "music_context.h"
#include "information_visitor.h"
#include "information_gatherer.h"
#include "mutant_database.h"
#include "music_ast_consumer.h"
#include "all_mutant_operators.h"

// #include <cstring>
// #include <cerrno>


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
    cout << "added " << mutant_name << endl;
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

bool IsAllDigits(const string s)
{
  return !s.empty() && s.find_first_not_of("0123456789") == string::npos;
}

/*namespace {
class ArgumentsAdjustingCompilations : public tooling::CompilationDatabase {
public:
  ArgumentsAdjustingCompilations(
      unique_ptr<tooling::CompilationDatabase> Compilations)
      : Compilations(move(Compilations)) {}

  void appendArgumentsAdjuster(tooling::ArgumentsAdjuster Adjuster) {
    Adjusters.push_back(move(Adjuster));
  }

  vector<tooling::CompileCommand>
  getCompileCommands(StringRef FilePath) const override {
    return adjustCommands(Compilations->getCompileCommands(FilePath));
  }

  vector<string> getAllFiles() const override {
    return Compilations->getAllFiles();
  }

  vector<tooling::CompileCommand> getAllCompileCommands() const override {
    return adjustCommands(Compilations->getAllCompileCommands());
  }

private:
  unique_ptr<tooling::CompilationDatabase> Compilations;
  vector<tooling::ArgumentsAdjuster> Adjusters;

  vector<tooling::CompileCommand>
  adjustCommands(vector<tooling::CompileCommand> Commands) const {
    for (tooling::CompileCommand &Command : Commands)
      for (const auto &Adjuster : Adjusters)
        Command.CommandLine = Adjuster(Command.CommandLine, Command.Filename);
    return Commands;
  }
};
}

class MusicOptionsParser {
public:
  /// \brief Parses command-line, initializes a compilation database.
  ///
  /// This constructor can change argc and argv contents, e.g. consume
  /// command-line options used for creating FixedCompilationDatabase.
  ///
  /// All options not belonging to \p Category become hidden.
  ///
  /// This constructor exits program in case of error.
  MusicOptionsParser(int &argc, const char **argv,
                      llvm::cl::OptionCategory &Category,
                      const char *Overview = nullptr)
      : MusicOptionsParser(argc, argv, Category, llvm::cl::Optional,
                            Overview) {}

  /// \brief Parses command-line, initializes a compilation database.
  ///
  /// This constructor can change argc and argv contents, e.g. consume
  /// command-line options used for creating FixedCompilationDatabase.
  ///
  /// All options not belonging to \p Category become hidden.
  ///
  /// I also allows calls to set the required number of positional parameters.
  ///
  /// This constructor exits program in case of error.
  MusicOptionsParser(int &argc, const char **argv,
                     llvm::cl::OptionCategory &Category,
                     llvm::cl::NumOccurrencesFlag OccurrencesFlag,
                     const char *Overview = nullptr)
  {
    static llvm::cl::opt<bool> Help("h", llvm::cl::desc("Alias for -help"), llvm::cl::Hidden);

    static llvm::cl::opt<std::string> BuildPath("p", llvm::cl::desc("Build path"),
                                          llvm::cl::Optional, llvm::cl::cat(Category));

    static llvm::cl::list<std::string> SourcePaths(
        "c", llvm::cl::desc("<source0> [... <sourceN>]"), OccurrencesFlag,
        llvm::cl::cat(Category));

    static llvm::cl::list<std::string> ArgsAfter(
        "extra-arg",
        llvm::cl::desc("Additional argument to append to the compiler command line"),
        llvm::cl::cat(Category));

    static llvm::cl::list<std::string> ArgsBefore(
        "extra-arg-before",
        llvm::cl::desc("Additional argument to prepend to the compiler command line"),
        llvm::cl::cat(Category));

    llvm::cl::HideUnrelatedOptions(Category);

    Compilations.reset(tooling::FixedCompilationDatabase::loadFromCommandLine(argc, argv));
    llvm::cl::ParseCommandLineOptions(argc, argv, Overview);
    llvm::cl::PrintOptionValues();

    SourcePathList = SourcePaths;
    if ((OccurrencesFlag == llvm::cl::ZeroOrMore || OccurrencesFlag == llvm::cl::Optional) &&
        SourcePathList.empty())
      return;
    if (!Compilations) {
      std::string ErrorMessage;
      if (!BuildPath.empty()) {
        Compilations =
            tooling::CompilationDatabase::autoDetectFromDirectory(BuildPath, ErrorMessage);
      } else {
        Compilations = tooling::CompilationDatabase::autoDetectFromSource(SourcePaths[0],
                                                                 ErrorMessage);
      }
      if (!Compilations) {
        llvm::errs() << "Error while trying to load a compilation database:\n"
                     << ErrorMessage << "Running without flags.\n";
        Compilations.reset(
            new tooling::FixedCompilationDatabase(".", std::vector<std::string>()));
      }
    }
    auto AdjustingCompilations =
        llvm::make_unique<ArgumentsAdjustingCompilations>(
            std::move(Compilations));
    AdjustingCompilations->appendArgumentsAdjuster(
        getInsertArgumentAdjuster(ArgsBefore, tooling::ArgumentInsertPosition::BEGIN));
    AdjustingCompilations->appendArgumentsAdjuster(
        getInsertArgumentAdjuster(ArgsAfter, tooling::ArgumentInsertPosition::END));
    Compilations = std::move(AdjustingCompilations);
  }

  /// Returns a reference to the loaded compilations database.
  tooling::CompilationDatabase &getCompilations() {
    return *Compilations;
  }

  /// Returns a list of source file paths to process.
  const std::vector<std::string> &getSourcePathList() const {
    return SourcePathList;
  }

  static const char *const HelpMessage;

private:
  std::unique_ptr<tooling::CompilationDatabase> Compilations;
  std::vector<std::string> SourcePathList;
  std::vector<std::string> ExtraArgsBefore;
  std::vector<std::string> ExtraArgsAfter;
};*/

static llvm::cl::OptionCategory MusicOptions("MUSIC options");
static llvm::cl::extrahelp CommonHelp(tooling::CommonOptionsParser::HelpMessage);

// static llvm::cl::extrahelp MoreHelp("\nMore help text...");

static llvm::cl::list<string> OptionM(
    "m", llvm::cl::desc("Specify mutant operator name to use"), 
    llvm::cl::value_desc("mutantname[:domain[:range1]]"),
    llvm::cl::cat(MusicOptions));

static llvm::cl::opt<string> OptionO(
    "o", llvm::cl::desc("Specify output directory for MUSIC. \
Absolute directory is required."), 
    llvm::cl::value_desc("outputdir"),
    llvm::cl::cat(MusicOptions));

static llvm::cl::opt<unsigned int> OptionL(
    "l", llvm::cl::desc("Specify maximum number of mutants generated per \
mutation point & mutation operator"), 
    llvm::cl::value_desc("maxnum"),
    llvm::cl::init(UINT_MAX), llvm::cl::cat(MusicOptions));

static llvm::cl::list<string> OptionRS(
    "rs",
    llvm::cl::cat(MusicOptions));
static llvm::cl::list<string> OptionRE(
    "re",
    llvm::cl::cat(MusicOptions));

static llvm::cl::list<string> OptionX(
    "x", llvm::cl::desc("Specify list of lines to exclude for mutant generation for each file"),
    llvm::cl::cat(MusicOptions));

// static llvm::cl::list<unsigned int> OptionRE(
//     "re", llvm::cl::multi_val(2),
//     llvm::cl::cat(MusicOptions));

// static llvm::cl::opt<string> OptionA("A", llvm::cl::cat(MusicOptions));
// static llvm::cl::opt<string> OptionB("B", llvm::cl::cat(MusicOptions));

InformationGatherer *g_gatherer;
CompilerInstance *g_CI;
Configuration *g_config;
MutantDatabase *g_mutant_database;
MusicContext *g_music_context;
vector<ExprMutantOperator*> g_expr_mutant_operator_list;
vector<StmtMutantOperator*> g_stmt_mutant_operator_list;
map<string, vector<int>> g_rs_list;
map<string, vector<int>> g_re_list;
map<string, vector<int>> g_exclude_list;
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

/*inline bool exists_test3 (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}*/

void ParseOptionRS()
{
  if (OptionRS.empty())
    return;

  for (auto e: OptionRS)
  {
    vector<string> temp;
    SplitStringIntoVector(e, temp, string(":"));

    if (temp.size() == 1 || temp.size() == 0)
    {
      cout << "Range start not specified in " << e << endl;
      exit(1);
    }

    if (!IsAllDigits(temp[1]))
    {
      PrintLineColNumberErrorMsg();
      exit(1);
    }

    int line_num;
    stringstream(temp[1]) >> line_num;

    if (line_num == 0)
    {
      cout << "Option RS specification error: line/col number must be larger than 0." << endl;
      exit(1);
    }

    g_rs_list[temp[0]] = vector<int>{line_num};

    if (temp.size() > 2)
    {
      if (!IsAllDigits(temp[2]))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      int col_num;
      stringstream(temp[2]) >> col_num;

      if (col_num == 0)
      {
        cout << "Option RS specification error: line/col number must be larger than 0." << endl;
        exit(1);
      }
      g_rs_list[temp[0]].push_back(col_num);
    }
  }

  // for (auto e: g_rs_list)
  // {
  //   cout << e.first << " has range start\n";
  //   for (auto d: e.second)
  //     cout << d << " ";
  //   cout << endl;
  // }
}

void ParseOptionRE()
{
  if (OptionRE.empty())
    return;

  for (auto e: OptionRE)
  {
    vector<string> temp;
    SplitStringIntoVector(e, temp, string(":"));

    if (temp.size() == 1 || temp.size() == 0)
    {
      cout << "Range end not specified in " << e << endl;
      exit(1);
    }

    if (!IsAllDigits(temp[1]))
    {
      PrintLineColNumberErrorMsg();
      exit(1);
    }

    int line_num;
    stringstream(temp[1]) >> line_num;

    if (line_num == 0)
    {
      cout << "Option RE specification error: line/col number must be larger than 0." << endl;
      exit(1);
    }

    g_re_list[temp[0]] = vector<int>{line_num};

    if (temp.size() > 2)
    {
      if (!IsAllDigits(temp[2]))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      int col_num;
      stringstream(temp[2]) >> col_num;

      if (col_num == 0)
      {
        cout << "Option RE specification error: line/col number must be larger than 0." << endl;
        exit(1);
      }
      g_re_list[temp[0]].push_back(col_num);
    }
  }

  // for (auto e: g_rs_list)
  // {
  //   cout << e.first << " has range end\n";
  //   for (auto d: e.second)
  //     cout << d << " ";
  //   cout << endl;
  // }
}

void ParseOptionX() 
{
  if (OptionX.empty())
    return;

  for (auto e: OptionX)
  {
    cout << "parsing " << e << endl;
    vector<string> temp;
    SplitStringIntoVector(e, temp, string(":"));

    if (temp.size() != 2)
    {
      cout << "Excluded lines not specified in " << e << endl;
      exit(1);
    }

    cout << "target is " << temp[0] << endl;

    vector<string> excluded_line_list;
    SplitStringIntoVector(temp[1], excluded_line_list, string(","));

    if (g_exclude_list.count(temp[0]) == 0)
      g_exclude_list[temp[0]] = vector<int>{};

    for (auto line_num_str: excluded_line_list) 
    {
      if (!IsAllDigits(line_num_str))
      {
        cout << "Invalid line number " << line_num_str << "\n";
        cout << "Usage: [-x <filename>:<line1>[,<line2>,...]]\n";
        exit(1);
      }

      int line_num;
      stringstream(line_num_str) >> line_num;

      if (line_num == 0)
      {
        cout << "Option X specification error: line number must be larger than 0." << endl;
        exit(1);
      }

      g_exclude_list[temp[0]].push_back(line_num);
    }
  }

  for (auto e: g_exclude_list)
  {
    cout << e.first << " has to exclude the following lines\n";
    for (auto d: e.second)
      cout << d << " ";
    cout << endl;
  }
}

void ParseOptionO()
{
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
}

void ParseOptionL()
{
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
}

void ParseOptionM() 
{
  // Parse option -m (if provided)
  if (OptionM.empty())
  {
    AddAllMutantOperator(g_stmt_mutant_operator_list, 
                         g_expr_mutant_operator_list);
    cout << "done with option m\n";
    return;
  }

  for (auto e: OptionM)
  {
    set<string> domain, range;

    cout << "analyzing " << e << endl;
    vector<string> mutant_operator;

    // Split input into mutant operator name, domain, range (if specified)
    SplitStringIntoVector(e, mutant_operator, string(":"));

    for (auto it: mutant_operator)
      cout << it << endl;

    // Capitalize mutant operator name.
    for (int i = 0; i < mutant_operator[0].length() ; ++i)
    {
      if (mutant_operator[0][i] >= 'a' && 
          mutant_operator[0][i] <= 'z')
        mutant_operator[0][i] -= 32;
    }

    // Gather domain if specified.
    if (mutant_operator.size() > 1)
    {
      SplitStringIntoSet(mutant_operator[1], domain, string(","));

      // Remove empty strings
      for (auto it = domain.begin(); it != domain.end(); )
      {
        if ((*it).empty())
          it = domain.erase(it);
        else
          ++it;
      }
    }

    // Gather range if specified.
    if (mutant_operator.size() > 2)
    {
      SplitStringIntoSet(mutant_operator[2], range, string(","));
      for (auto it = range.begin(); it != range.end(); )
      {
        if ((*it).empty())
          it = range.erase(it);
        else
          ++it;
      }
    }

    AddMutantOperator(mutant_operator[0], domain, range,
                      g_stmt_mutant_operator_list,
                      g_expr_mutant_operator_list);
  }

  // return 0;
  cout << "done with option m\n";
}

class GenerateMutantAction : public ASTFrontendAction
{
protected:
  void ExecuteAction() override
  {
    CompilerInstance &CI = getCompilerInstance();
    CI.getPreprocessor().createPreprocessingRecord();
    
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

    out_mutDb << "Mutant Filename,Mutation Operator,Line#,Before Mutation,,,,,After Mutation" << endl;
    out_mutDb << ",,,Start Line#,Start Col#,End Line#,End Col#,Target Token,";
    out_mutDb << "Start Line#,Start Col#,End Line#,End Col#,Mutated Token" << endl;
    out_mutDb.close();

    g_mutant_database->ExportAllEntries();
    // g_mutant_database->WriteAllEntriesToDatabaseFile();
  }

public:
  virtual unique_ptr<ASTConsumer> CreateASTConsumer(
      CompilerInstance &CI, llvm::StringRef InFile)
  {
    // Parse rs and re option.
    SourceManager &sm = CI.getSourceManager();
    g_mutation_range_start = sm.getLocForStartOfFile(sm.getMainFileID());
    g_mutation_range_end = sm.getLocForEndOfFile(sm.getMainFileID());

    // If user specifies range for this input file,
    // verify that the given input range start is valid before setting 
    // g_mutation_range_start.
    if (g_rs_list.count(g_inputfile_name))
    {
      int line_num = g_rs_list[g_inputfile_name].front();
      int col_num = 1;

      if (g_rs_list[g_inputfile_name].size() == 2)
        col_num = g_rs_list[g_inputfile_name].back();

      SourceLocation interpreted_loc = sm.translateLineCol(
          sm.getMainFileID(), line_num, col_num);

      if (line_num != GetLineNumber(sm, interpreted_loc) ||
          col_num != GetColumnNumber(sm, interpreted_loc))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      g_mutation_range_start = sm.translateLineCol(
          sm.getMainFileID(), line_num, col_num);
    }

    if (g_re_list.count(g_inputfile_name))
    {
      int line_num = g_re_list[g_inputfile_name].front();
      int col_num = 1;

      if (g_re_list[g_inputfile_name].size() == 2)
        col_num = g_re_list[g_inputfile_name].back();

      SourceLocation interpreted_loc = sm.translateLineCol(
          sm.getMainFileID(), line_num, col_num);

      if (line_num != GetLineNumber(sm, interpreted_loc) ||
          col_num != GetColumnNumber(sm, interpreted_loc))
      {
        PrintLineColNumberErrorMsg();
        exit(1);
      }

      g_mutation_range_end = sm.translateLineCol(
          sm.getMainFileID(), line_num, col_num);
    }

    // cout << g_inputfile_name << endl;
    // PrintLocation(sm, g_mutation_range_start);
    // PrintLocation(sm, g_mutation_range_end);

    vector<int> excluded_lines;
    if (g_exclude_list.count(g_inputfile_name))
      excluded_lines = vector<int>(g_exclude_list[g_inputfile_name]);
    else
      if (g_exclude_list.count(g_current_inputfile_path))
        excluded_lines = vector<int>(g_exclude_list[g_current_inputfile_path]); 

    /* Create Configuration object pointer to pass as attribute 
       for MusicASTConsumer. */
    g_config = new Configuration(
        g_inputfile_name, g_mutdbfile_name, g_mutation_range_start, 
        g_mutation_range_end, excluded_lines, g_output_dir, g_limit);

    // for (auto e: g_config->getExcludedLines())
    //   cout << e << endl;
    // exit(1);

    g_mutant_database = new MutantDatabase(
        &CI, g_config->getInputFilename(),
        g_config->getOutputDir(), g_limit);

    g_music_context = new MusicContext(
        &CI, g_config, g_gatherer->getLabelToGotoListMap(),
        g_gatherer->getSymbolTable(), *g_mutant_database);

    return unique_ptr<ASTConsumer>(new MusicASTConsumer(
        &CI, g_gatherer->getLabelToGotoListMap(),
        g_stmt_mutant_operator_list,
        g_expr_mutant_operator_list, *g_music_context));
  }
};

class GatherDataAction : public ASTFrontendAction
{
protected:
  void ExecuteAction() override
  {
    CompilerInstance &CI = getCompilerInstance();
    CI.getPreprocessor().createPreprocessingRecord();

    HeaderSearchOptions &hso = CI.getHeaderSearchOpts();

    cout << "executing action from GatherDataAction\n";
    ASTFrontendAction::ExecuteAction();

    // cout << g_gatherer->getLabelToGotoListMap()->size() << endl;

    vector<string> source{g_current_inputfile_path};

    tooling::ClangTool Tool2(g_option_parser->getCompilations(),
                             source);

    Tool2.run(tooling::newFrontendActionFactory<GenerateMutantAction>().get());
  }

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
  g_option_parser = new tooling::CommonOptionsParser(
      argc, argv, MusicOptions/*, llvm::cl::Optional*/);

  // Randomization for option -l.
  srand (time(NULL));

  ParseOptionRS();
  ParseOptionRE();
  ParseOptionX();
  ParseOptionO();
  ParseOptionL();
  ParseOptionM();

  // ofstream my_file("/home/duyloc1503/comut-libtool/multiple-compile-command-files.txt", ios::trunc);    

  /* Run tool separately for each input file. */
  for (auto file: g_option_parser->getSourcePathList())
  { 
    // cout << "Running MUSIC on " << file << endl;

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
    // int counter = 0;
    // for (auto e: g_option_parser->getCompilations().getCompileCommands(file))
    // {
    //   counter++;
    //   cout << "==========" << counter << "==========\n";
    //   cout << e.Directory << endl;
    //   cout << e.Filename << endl;
    //   for (auto command: e.CommandLine)
    //     cout << command << " ";
    //   cout << endl;
    //   cout << "=====================\n";
    // }

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
    g_mutdbfile_name += "_mut_db.csv";

    cout << "g_current_inputfile_path = " << g_current_inputfile_path << endl;
    cout << "g_inputfile_name = " << g_inputfile_name << endl;
    cout << "g_mutdbfile_name = " << g_mutdbfile_name << endl;

    vector<string> source{g_current_inputfile_path};
  
    // Run tool
    tooling::ClangTool Tool1(g_option_parser->getCompilations(),
                             source);

    Tool1.run(tooling::newFrontendActionFactory<GatherDataAction>().get());

    cout << "Done tooling on " << file << endl;
  }

  // my_file.close();

  return 0;
}
