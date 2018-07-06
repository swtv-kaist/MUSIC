#include <iostream>

#include "music_utility.h"
#include "clang/AST/PrettyPrinter.h"

set<string> arithemtic_operators{"+", "-", "*", "/", "%"};
set<string> bitwise_operators{"&", "|", "^"};
set<string> shift_operators{">>", "<<"};
set<string> logical_operators{"&&", "||"};
set<string> relational_operators{">", "<", "<=", ">=", "==", "!="};
set<string> arith_assignment_operators{"+=", "-=", "*=", "/=", "%="};
set<string> bitwise_assignment_operators{"&=", "|=", "^="};
set<string> shift_assignment_operators{"<<=", ">>="};
set<string> assignment_operator{"="};

string ConvertToString(Stmt *from, LangOptions &LangOpts)
{
  string SStr;
  llvm::raw_string_ostream S(SStr);
  from->printPretty(S, 0, PrintingPolicy(LangOpts));
  return S.str();
}

// Print out each element of a string set in a single line.
void PrintStringSet(std::set<std::string> &string_set)
{
  for (auto element: string_set)
  {
    std::cout << "//" << element;
  }
  std::cout << "//\n";
}

// Print out each elemet of a string vector in a single line.
void PrintStringVector(const vector<string> &string_vector)
{
  for (auto it: string_vector) 
    std::cout << "//" << it;
  
  std::cout << "//\n";
}

// remove spaces at the beginning and end_loc of string str
string TrimBeginningAndEndingWhitespace(const string &str)
{
  string whitespace{" "};

  auto first_non_whitespace_index = str.find_first_not_of(whitespace);

  if (first_non_whitespace_index == string::npos)
    return ""; // no content

  auto last_non_whitespace_index = str.find_last_not_of(whitespace);
  auto string_range = last_non_whitespace_index - first_non_whitespace_index + 1;
  return str.substr(first_non_whitespace_index, string_range);
}

// Divide string into elements of a string set with delimiter
void SplitStringIntoSet(string target, set<string> &out_set, 
                        const string delimiter)
{
  size_t pos = 0;
  string token;

  while ((pos = target.find(delimiter)) != string::npos) {
    token = target.substr(0, pos);
    out_set.insert(TrimBeginningAndEndingWhitespace(token));
    target.erase(0, pos + delimiter.length());
  }

  out_set.insert(TrimBeginningAndEndingWhitespace(target));
}

void SplitStringIntoVector(string target, vector<string> &out_vector, 
                           const string delimiter)
{
  size_t pos = 0;
  string token;

  while ((pos = target.find(delimiter)) != string::npos) {
    token = target.substr(0, pos);
    out_vector.push_back(token);
    target.erase(0, pos + delimiter.length());
  }
  out_vector.push_back(target);
}

/**
  @param  hexa: hexa string of the following form "'\xF...F'"
  @return string of the integer value of the given hexa string
*/
string ConvertHexaStringToIntString(const string hexa_str)
{
  char first_hexa_digit = hexa_str.at(3);

  if (!((first_hexa_digit >= '0' && first_hexa_digit <= '9') || 
    (first_hexa_digit >= 'a' && first_hexa_digit <= 'f') || 
    (first_hexa_digit >= 'A' && first_hexa_digit <= 'F')))
    return hexa_str;

  char secondh_hexa_digit = hexa_str.at(4);
  
  if (secondh_hexa_digit != '\'')
    if (!((secondh_hexa_digit >= '0' && secondh_hexa_digit <= '9') || 
        (secondh_hexa_digit >= 'a' && secondh_hexa_digit <= 'f') || 
        (secondh_hexa_digit >= 'A' && secondh_hexa_digit <= 'F')))
      return hexa_str;

  return to_string(stoul(hexa_str.substr(3, hexa_str.length() - 4), nullptr, 16));
}

string ConvertCharStringToIntString(const string s)
{
  // it is a single, non-escaped character like 'a'
  if (s.length() == 3)
    return to_string(int(s.at(1)));

  if (s.at(1) == '\\')
  {
    // it is an escaped character like '\n'

    int length = s.length() - 3;

    switch (s.at(2))
    {
      case 'a':
        if (length == 1)
          return to_string(int('\a'));
        break;
      case 'b':
        if (length == 1)
          return to_string(int('\b'));
        break;
      case 'f':
        if (length == 1)
          return to_string(int('\f'));
        break;
      case 'n':
        if (length == 1)
          return to_string(int('\n'));
        break;
      case 'r':
        if (length == 1)
          return to_string(int('\r'));
        break;
      case 't':
        if (length == 1)
          return to_string(int('\t'));
        break;
      case 'v':
        if (length == 1)
          return to_string(int('\v'));
        break;
      case '0':
        if (length == 1)
          return to_string(int('\0'));
        break; 
      case '\\':
      case '\'':
      case '\"':
        if (length == 1)
          return to_string(int(s.at(2)));
        break;
      case 'x':
      case 'X':
        if (length <= 3 && length > 1)
          return ConvertHexaStringToIntString(s);
        else 
          return s;   // hexadecimal value higher than FF. not a char
      default:
        cout << "cannot convert " << s << " to string of int\n";
        return s;  
    }
  }
  
  // the function does not handle cases like 'abc'
  cout << "cannot convert " << s << " to string of int\n";
  return s;
}

bool IsStringElementOfVector(string s, vector<string> &string_vector)
{
  auto it = string_vector.begin();

  while (it != string_vector.end() && (*it).compare(s) != 0)
    ++it;

  // if the iterator reach the end_loc then the string is NOT in vector v
  return !(it == string_vector.end());
}

bool IsStringElementOfSet(string s, set<string> &string_set)
{
  auto it = string_set.begin();

  while (it != string_set.end() && (*it).compare(s) != 0)
    ++it;

  return it != string_set.end();
}

bool ConvertStringToInt(string s, int &n)
{
  stringstream convert(s);
  if (!(convert >> n))
    return false;
  return true;
}

/**
  @param  s string literal from C input file
          pos index at which to check if the character there is a whitespace
  @return True if the character at pos is a whitespace
          False otherwise
          If the character at pos is whitespace, pos is changed to the
          position after the whitespace
*/
bool IsWhitespace(const string &s, int &pos)
{
  if (s[pos] == ' ')
  {
    ++pos;
    return true;
  }

  if (s[pos] == '\\')
  {
    if (s[pos+1] == 'f' || s[pos+1] == 'n' ||
      s[pos+1] == 'r' || s[pos+1] == 't' ||
      s[pos+1] == 'v')
    {
      pos += 2;
      return true;
    }

    if (pos + 3 < s.length())
    {
      string temp{s.substr(pos,4)};
      if (temp.compare("\\011") == 0 ||
        temp.compare("\\012") == 0 ||
        temp.compare("\\013") == 0 ||
        temp.compare("\\014") == 0 ||
        temp.compare("\\015") == 0)
      {
        pos += 4;
        return true;
      }
    }
  }

  return false;
}

bool IsCharAlphabetic(char c)
{
  return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

bool IsCharNumeric(char c)
{
  return c >= '0' && c <= '9';
}

bool IsValidVariableName(string s)
{
  // First char has to be alphabetic or underscore
  // String should not be empty
  if ((!IsCharAlphabetic(s[0]) && s[0] != '_') ||
      s.length() < 1)
    return false;

  // Variable name can only consists of alphabet, number and underscore.
  for (char c: s)
    if (!(IsCharAlphabetic(c) || IsCharNumeric(c) || c == '_'))
      return false;

  return true;
}

/**
  @param  s string literal from input file
  @return index of the first non whitespace character (space, \f, \n, \r, \t, \v)

  Return an int higher than 1
*/
int GetFirstNonWhitespaceIndex(const string &s)
{
  // Skip the first character which is double quote
  int ret{1};

  while (IsWhitespace(s, ret)) {}

  return ret;
}

int GetLastNonWhitespaceIndex(const string &s)
{
  int ret{GetFirstNonWhitespaceIndex(s)};
  int i = ret;
  int length{(int) s.length()};

  while (i != length-1)
  {   
    // If the character is not whitespace, change ret position.
    if (!IsWhitespace(s, i))
    {
      // i is not changed because this character is NOT whitespace
      if (s[i] == '\\')  // escaped character
        i += 2;
      else
        ++i;

      ret = i - 1;
    }
  }

  return ret;
}

bool NumIsFloat(std::string num) 
{
  for (int i = 0; i < num.length(); ++i)
  {
    if (num[i] == '.')
      return true;
  }
  return false;
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
  std::set<std::string> logical{/*"OLLN",*/ /*"OLAN",*/ /*"OLBN",*/ /*"OLRN",*/ /*"OLSN"*/};

  // set of operators mutating relational operator
  std::set<std::string> relational{/*"ORLN",*/ /*"ORAN",*/ /*"ORBN",*/ /*"ORRN",*/ /*"ORSN"*/};

  // set of operators mutating arithmetic operator
  std::set<std::string> arithmetic{/*"OALN",*/ /*"OAAN",*/ /*"OABN",*/ /*"OARN",*/ /*"OASN"*/};

  // set of operators mutating bitwise operator
  std::set<std::string> bitwise{/*"OBLN",*/ /*"OBAN",*/ /*"OBBN",*/ /*"OBRN",*/ /*"OBSN"*/};

  // set of operators mutating shift operator
  std::set<std::string> shift{/*"OSLN"*/ /*"OSAN",*/ /*"OSBN"*//*, "OSRN"*//*, "OSSN"*/};

  // set of operators mutating arithmetic assignment operator
  std::set<std::string> arithmetic_assignment{/*"OAAA",*/ /*"OABA",*/ /*"OAEA",*/ /*"OASA"*/};

  // set of operators mutating bitwise assignment operator
  std::set<std::string> bitwise_assignment{/*"OBAA",*/ /*"OBBA",*/ /*"OBEA",*/ /*"OBSA"*/};

  // set of operators mutating shift assignment operator
  std::set<std::string> shift_assignment{/*"OSAA", "OSBA", "OSEA", "OSSA"*/};

  // set of operators mutating plain assignment operator
  std::set<std::string> plain_assignment{/*"OEAA",*/ /*"OEBA",*/ /*"OESA"*/};

  // set of operators whose domain cannot be altered.
  std::set<std::string> empty_domain{/*"CRCR", "SSDL",*/ /*"VLSR",*/ /*"VGSR",*/ /*"VGAR",*/ 
  																	 /*"VLAR",*/ /*"VGTR",*/ /*"VLTR",*/ /*"VGPR",*/ /*"VLPR",*/ 
  																	 /*"VSCR",*/ /*"CGCR",*/ /*"CLCR",*/ /*"CGSR",*/ /*"CLSR",*/ 
  																	 /*"OMMO",*/ /*"OPPO",*/ /*"OLNG",*/ /*"OCNG",*/ /*"OBNG",*/ 
  																	 "OBOM", /*"VTWD",*/ /*"OIPM",*/ /*"OCOR",*/ /*"SANL",*/ 
  																	 /*"SRWS",*/ /*"SCSR",*/ /*"VLSF",*/ /*"VGSF",*/ /*"VGTF",*/ 
  																	 /*"VLTF",*/ /*"VGPF"*//*, "VLPF"*//*, "VTWF"*/};

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
    // exit(1);
    return;
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
  std::set<std::string> logical{/*"OALN",*/ /*"OBLN",*/ /*"OLLN",*/ /*"ORLN",*/ /*"OSLN"*/};

  // set of operators mutating to a relational operator
  std::set<std::string> relational{/*"OLRN",*/ /*"OARN",*/ /*"OBRN",*/ /*"ORRN",*/ /*"OSRN"*/};

  // set of operators mutating to a arithmetic operator
  std::set<std::string> arithmetic{/*"OLAN",*/ /*"OAAN",*/ /*"OBAN",*/ /*"ORAN",*/ /*"OSAN"*/};

  // set of operators mutating to a arithmetic operator
  std::set<std::string> bitwise{/*"OLBN",*/ /*"OABN",*/ /*"OBBN",*/ /*"ORBN",*/ /*"OSBN"*/};

  // set of operators mutating to a arithmetic operator
  std::set<std::string> shift{/*"OLSN", "OASN", "OBSN",*/ /*"ORSN"*//*, "OSSN"*/};

  // set of operators mutating to arithmetic assignment operator
  std::set<std::string> arithmetic_assignment{/*"OAAA",*/ /*"OBAA",*/ /*"OEAA"*//*, "OSAA"*/};

  // set of operators mutating to bitwise assignment operator
  std::set<std::string> bitwise_assignment{/*"OABA",*/ /*"OBBA",*/ /*"OEBA"*//*, "OSBA"*/};

  // set of operators mutating to shift assignment operator
  std::set<std::string> shift_assignment{/*"OASA", "OBSA",*/ /*"OESA"*//*, "OSSA"*/};

  // set of operators mutating to plain assignment operator
  std::set<std::string> plain_assignment{/*"OAEA",*/ /*"OBEA",*/ /*"OSEA"*/};

  // set of operators whose range cannot be altered.
  std::set<std::string> empty_range{/*"SSDL",*/ /*"VLSR",*/ /*"VGSR",*/ /*"VGAR",*/ /*"VLAR",*/ 
  																	/*"VGTR",*/ /*"VLTR",*/ /*"VGPR",*/ /*"VLPR",*/ /*"VSCR",*/ 
  																	/*"CGCR",*/ /*"CLCR",*/ /*"CGSR",*/ /*"CLSR",*/ /*"OMMO", */
  																	/*"OPPO",*/ /*"OLNG",*/ /*"OCNG",*/ /*"OBNG",*/ "OBOM", 
  																	/*"VTWD",*/ /*"OIPM",*/ /*"OCOR",*/ /*"SANL",*/  /*"SRWS",*/ 
  																	/*"SCSR",*/ /*"VLSF",*/ /*"VGSF",*/ /*"VGTF",*/ /*"VLTF",*/  
  																	/*"VGPF"*//*, "VLPF"*//*, "VTWF"*/};

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
  /*else if (mutant_name.compare("CRCR") == 0)
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
  }*/
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
    // exit(1);
    return;
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

int GetLineNumber(SourceManager &src_mgr, SourceLocation loc)
{
  return static_cast<int>(src_mgr.getExpansionLineNumber(loc));
}

int GetColumnNumber(SourceManager &src_mgr, SourceLocation loc)
{
  return static_cast<int>(src_mgr.getExpansionColumnNumber(loc));
}

SourceLocation GetEndLocOfStmt(SourceLocation loc, 
                               CompilerInstance *comp_inst)
{
  SourceLocation ret{};
  SourceManager &src_mgr = comp_inst->getSourceManager();

  if (*(src_mgr.getCharacterData(loc)) == '}' || 
      *(src_mgr.getCharacterData(loc)) == ';' ||
      *(src_mgr.getCharacterData(loc)) == ']' || 
      *(src_mgr.getCharacterData(loc)) == ')')
    ret = loc.getLocWithOffset(1);
  else
    ret = clang::Lexer::findLocationAfterToken(loc,
                                              tok::semi, src_mgr,
                                              comp_inst->getLangOpts(), 
                                              false);
  return ret;
}

void PrintLocation(SourceManager &src_mgr, SourceLocation loc)
{
  cout << "(" << GetLineNumber(src_mgr, loc) << " : ";
  cout << GetColumnNumber(src_mgr, loc) << ")" << endl;
}

void PrintRange(SourceManager &src_mgr, SourceRange range)
{
  cout << "this range is from ";
  PrintLocation(src_mgr, range.getBegin());
  cout << "        to ";
  PrintLocation(src_mgr, range.getEnd());
}

// Return the number of not-newline character
int CountNonNewlineChar(const string &s)
{  
  int res = 0;
  for (int i = 0; i < s.length(); ++i)
  {
    if (s[i] != '\n')
      ++res;
  }
  return res;
}

// Return True if location loc appears before range. 
// (location 1,1 appears before every other valid locations)
bool LocationBeforeRangeStart(SourceLocation loc, SourceRange range)
{
  SourceLocation start_of_range = range.getBegin();
  return loc < start_of_range;
}

// Return True if location loc appears after range. 
// (EOF appears after every other valid locations)
bool LocationAfterRangeEnd(SourceLocation loc, SourceRange range)
{
  SourceLocation end_of_range = range.getEnd();
  return loc != end_of_range && !(loc < end_of_range);
}

// Return True if the location is inside the range (inclusive)
// Return False otherwise
bool LocationIsInRange(SourceLocation loc, SourceRange range)
{
  return !LocationBeforeRangeStart(loc, range) &&
         !LocationAfterRangeEnd(loc, range);
}

bool Range1IsPartOfRange2(SourceRange range1, SourceRange range2)
{
  return LocationIsInRange(range1.getBegin(), range2) &&
         LocationIsInRange(range1.getEnd(), range2);
}

int CountNonNullStmtInCompoundStmt(CompoundStmt *c)
{
  int res{0};

  for (CompoundStmt::body_iterator it = c->body_begin(); it != c->body_end(); ++it)
  {
    if (!isa<NullStmt>(*it))
      ++res;
  }

  return res;
}

/**
This function assumes the given location is either before or after
a semicolon (;). Though I can use a while loop to go back and forth
until a semicolon is found, multi stmt on a single line can be 
confusing

@param  loc: target location with previous assumption
@return location after semicolon
    if cannot find then return given location
*/
SourceLocation GetLocationAfterSemicolon(SourceManager &src_mgr, 
                                         SourceLocation loc)
{
  // cout << "cp GetLocationAfterSemicolon\n";
  // PrintLocation(src_mgr, loc);
  if (loc.isInvalid() || GetColumnNumber(src_mgr, loc) == 1 ||
      GetLineNumber(src_mgr, loc) == 0)
    return loc;
  // cout << "passed\n";

  SourceLocation previous_loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(),
      GetLineNumber(src_mgr, loc),
      GetColumnNumber(src_mgr, loc) - 1);

  // cout << "done translate\n";

  if (*(src_mgr.getCharacterData(previous_loc)) == ';')
    return loc;

  // cout << "cp1\n";

  if (*(src_mgr.getCharacterData(loc)) == ';')
    return loc.getLocWithOffset(1);

  // cout << "cp2\n";

  return loc;
}

bool ExprIsPointer(Expr *e)
{
  return e->getType().getCanonicalType().getTypePtr()->isPointerType();
}

bool ExprIsScalar(Expr *e)
{
  auto type = ((e->getType().getCanonicalType()).getTypePtr());
  return type->isScalarType() && !ExprIsPointer(e); 
}

bool ExprIsFloat(Expr *e)
{
  return ((e->getType()).getCanonicalType().getTypePtr())->isFloatingType();
}

bool ExprIsIntegral(CompilerInstance *comp_inst, Expr *e)
{
  auto type = ((e->getType()).getCanonicalType().getTypePtr());
  return type->isIntegralType(comp_inst->getASTContext());
}

bool ExprIsArray(Expr *e)
{
  return ((e->getType().getCanonicalType()).getTypePtr())->isArrayType(); 
}

bool ExprIsStruct(Expr *e)
{
  return e->getType().getCanonicalType().getTypePtr()->isStructureType(); 
}

bool ExprIsDeclRefExpr(Expr *e)
{
  // An Enum value is not a declaration reference
  if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(e))
  {
    if (isa<EnumConstantDecl>(dre->getDecl()))
      return false;
    return true;
  }
  return false;
}

bool ExprIsPointerDereferenceExpr(Expr *e)
{
  if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
  {
    if (uo->getOpcode() == UO_Deref)
      return true;
    return false;
  }
  return false;
}

bool ExprIsScalarReference(Expr *e)
{
  if (ExprIsDeclRefExpr(e) ||  ExprIsPointerDereferenceExpr(e) ||
      isa<ArraySubscriptExpr>(e) || isa<MemberExpr>(e))
    if (ExprIsScalar(e))
      return true;

  return false;
}

bool ExprIsArrayReference(Expr *e)
{
  if (ExprIsDeclRefExpr(e) ||  ExprIsPointerDereferenceExpr(e) ||
      isa<ArraySubscriptExpr>(e) || isa<MemberExpr>(e))
    if (ExprIsArray(e))
      return true;

  return false;
}

bool ExprIsStructReference(Expr *e)
{
  if (ExprIsDeclRefExpr(e) ||  ExprIsPointerDereferenceExpr(e) ||
      isa<ArraySubscriptExpr>(e) || isa<MemberExpr>(e))
    if (ExprIsStruct(e))
      return true;

  return false;
}

bool ExprIsPointerReference(Expr *e)
{
  if (ExprIsDeclRefExpr(e) ||  ExprIsPointerDereferenceExpr(e) ||
      isa<ArraySubscriptExpr>(e) || isa<MemberExpr>(e))
    if (ExprIsPointer(e))
      return true;

  return false;
}

bool OperatorIsArithmeticAssignment(BinaryOperator *b)
{
  return b->getOpcode() >= BO_MulAssign && 
         b->getOpcode() <= BO_SubAssign;
}

bool OperatorIsShiftAssignment(BinaryOperator *b)
{
  return b->getOpcode() == BO_ShlAssign && 
         b->getOpcode() == BO_ShrAssign;
}

bool OperatorIsBitwiseAssignment(BinaryOperator *b)
{
  return b->getOpcode() >= BO_AndAssign && 
         b->getOpcode() <= BO_OrAssign;
}

/**
  @param  start_loc: start_loc location of the targeted string literal
  @return end_loc location of string literal
*/
SourceLocation GetEndLocOfStringLiteral(
    SourceManager &src_mgr, SourceLocation start_loc)
{
  int line_num = GetLineNumber(src_mgr, start_loc);
  int col_num = GetColumnNumber(src_mgr, start_loc) + 1;

  if (line_num == 0 || col_num == 0)
    return start_loc;

  // cout << "cp GetEndLocOfStringLiteral\n";
  // Get the location right AFTER the first double quote
  SourceLocation ret = src_mgr.translateLineCol(src_mgr.getMainFileID(), 
                                                line_num, col_num);

  while (*(src_mgr.getCharacterData(ret)) != '\"')
  {
    // if there is a backslash then skip the next character
    if (*(src_mgr.getCharacterData(ret)) == '\\')
    {
      ret = ret.getLocWithOffset(1);
    }

    ret = ret.getLocWithOffset(1);
  }

  ret = ret.getLocWithOffset(1);
  return ret;
}

/**
  @param  start_loc: start_loc location of the targeted literal
  @return end_loc location of number, char literal
*/
SourceLocation GetEndLocOfConstantLiteral(
    SourceManager &src_mgr, SourceLocation start_loc)
{
  int line_num = GetLineNumber(src_mgr, start_loc);
  int col_num = GetColumnNumber(src_mgr, start_loc);

  if (line_num == 0 || col_num == 0)
    return start_loc;

  // cout << "cp GetEndLocOfConstantLiteral\n";
  SourceLocation ret = src_mgr.translateLineCol(
      src_mgr.getMainFileID(), line_num, col_num);

  // a char starts and ends with a single quote
  bool is_char_literal = false;

  if (*(src_mgr.getCharacterData(ret)) == '\'')
    is_char_literal = true;

  if (is_char_literal)
  {
    ret = ret.getLocWithOffset(1);

    while (*(src_mgr.getCharacterData(ret)) != '\'')
    {
      // if there is a backslash then skip the next character
      if (*(src_mgr.getCharacterData(ret)) == '\\')
      {
        ret = ret.getLocWithOffset(1);
      }

      ret = ret.getLocWithOffset(1);
    }

    // End of while loop result in location right before a single quote
    ret = ret.getLocWithOffset(1);
  }
  else  // not char
  {
    // Here, I am assuming the appearance of these characters
    // signals the end_loc of a number literal.
    while (*(src_mgr.getCharacterData(ret)) != ' ' &&
        *(src_mgr.getCharacterData(ret)) != ';' &&
        *(src_mgr.getCharacterData(ret)) != '+' &&
        *(src_mgr.getCharacterData(ret)) != '-' &&
        *(src_mgr.getCharacterData(ret)) != '*' &&
        *(src_mgr.getCharacterData(ret)) != '/' &&
        *(src_mgr.getCharacterData(ret)) != '%' &&
        *(src_mgr.getCharacterData(ret)) != ',' &&
        *(src_mgr.getCharacterData(ret)) != ')' &&
        *(src_mgr.getCharacterData(ret)) != ']' &&
        *(src_mgr.getCharacterData(ret)) != '}' &&
        *(src_mgr.getCharacterData(ret)) != '>' &&
        *(src_mgr.getCharacterData(ret)) != '<' &&
        *(src_mgr.getCharacterData(ret)) != '=' &&
        *(src_mgr.getCharacterData(ret)) != '!' &&
        *(src_mgr.getCharacterData(ret)) != '|' &&
        *(src_mgr.getCharacterData(ret)) != '?' &&
        *(src_mgr.getCharacterData(ret)) != '&' &&
        *(src_mgr.getCharacterData(ret)) != '~' &&
        *(src_mgr.getCharacterData(ret)) != '`' &&
        *(src_mgr.getCharacterData(ret)) != ':' &&
        *(src_mgr.getCharacterData(ret)) != '\n')
      ret = ret.getLocWithOffset(1);
  }

  return ret;
}

/**
  @param  uo: pointer to expression with unary operator
  @return end_loc location of given expression
*/
SourceLocation GetEndLocOfUnaryOpExpr(
    UnaryOperator *uo, CompilerInstance *comp_inst)
{
  SourceLocation ret = uo->getLocEnd();

  if (uo->getOpcode() == UO_PostInc || uo->getOpcode() == UO_PostDec)
    // for post increment/decrement, getLocEnd returns 
    // the location right BEFORE ++/--
    ret = ret.getLocWithOffset(2);
  else
    if (uo->getOpcode() == UO_PreInc || uo->getOpcode() == UO_PreDec ||
        uo->getOpcode() == UO_AddrOf || uo->getOpcode() == UO_Deref ||
        uo->getOpcode() == UO_Plus || uo->getOpcode() == UO_Minus ||
        uo->getOpcode() == UO_Not || uo->getOpcode() == UO_LNot)
    {
      // getLocEnd returns the location right AFTER the unary operator
      // end_loc location of the expression is end_loc location of the sub-expr
      Expr *subexpr = uo->getSubExpr()->IgnoreImpCasts();

      SourceLocation start_loc = uo->getLocStart();
      SourceLocation end_loc = uo->getLocEnd();

      ret = GetEndLocOfExpr(subexpr, comp_inst);
    }
    else  // other cases, if any
    {
      ;   // just return clang end_loc loc
    }

  return ret;
}

SourceLocation GetEndLocOfExpr(Expr *e, CompilerInstance *comp_inst)
{
  bool print = false;
  int line = GetLineNumber(comp_inst->getSourceManager(), e->getLocStart());
  if (false)
    print = true;

  SourceManager &src_mgr = comp_inst->getSourceManager();
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr, comp_inst->getLangOpts());
  
  // cout << "GetEndLocOfExpr: " << ConvertToString(e, comp_inst->getLangOpts()) << endl << endl;

  // If this is macro, try the best to retrieve then end 
  // loc in source code
  if (e->getLocStart().isMacroID() && e->getLocEnd().isMacroID())
  {
    // cout << ConvertToString(e, comp_inst->getLangOpts()) << endl;

    pair<SourceLocation, SourceLocation> expansionRange = 
        rewriter.getSourceMgr().getImmediateExpansionRange(e->getLocEnd());
    SourceLocation end_macro = Lexer::getLocForEndOfToken(
        src_mgr.getExpansionLoc(expansionRange.second), 0, src_mgr, 
        comp_inst->getLangOpts());
    // cout << "macro: " << end_macro.printToString(src_mgr) << endl;

    // if (GetLineNumber(src_mgr, end_macro) == 2070)
    //   cout << end_macro.printToString(src_mgr) << endl;

    // Check whether it is a variable-typed or function-typed macro.
    SourceLocation it_loc = end_macro;
    while (*(src_mgr.getCharacterData(it_loc)) == ' ')
      it_loc = it_loc.getLocWithOffset(1);

     // Return end_macro if variable-typed. 
    if (*(src_mgr.getCharacterData(it_loc)) != '(')
      return end_macro;

    int parenthesis_counter = 1;
    it_loc = it_loc.getLocWithOffset(1);

     // Find the matching close-parenthesis for this open-parenthesis. 
    while (parenthesis_counter != 0)
    {
      if (*(src_mgr.getCharacterData(it_loc)) == '(')
        parenthesis_counter++;

      if (*(src_mgr.getCharacterData(it_loc)) == ')')
        parenthesis_counter--;

      it_loc = it_loc.getLocWithOffset(1);
    }

    // if (GetLineNumber(src_mgr, it_loc) == 2070)
    // {
    //   cout << it_loc.printToString(src_mgr) << endl;
    //   // exit(1);
    // }

    return it_loc;
  }

  if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
  {
    if (print) cout << "into unary\n";
    return GetEndLocOfUnaryOpExpr(uo, comp_inst);
  }

  if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
    return GetEndLocOfExpr(bo->getRHS()->IgnoreImpCasts(), comp_inst);

  if (UnaryExprOrTypeTraitExpr *uete = dyn_cast<UnaryExprOrTypeTraitExpr>(e))
    if (!uete->isArgumentType())
      return GetEndLocOfExpr(
          uete->getArgumentExpr()->IgnoreImpCasts(), comp_inst);

  SourceLocation ret = e->getLocEnd();

  if (print) cout << "cp 1\n";

  // classify expression and get end_loc location accordingly
  if (isa<ArraySubscriptExpr>(e))
  {
    if (print) cout << "cp 2\n";
    ret = e->getLocEnd();
    ret = GetEndLocOfStmt(ret, comp_inst);
  }
  else if (CallExpr *ce = dyn_cast<CallExpr>(e))
  {
    if (print) cout << "cp 3\n";
    // getRParenLoc returns the location before the right parenthesis
    ret = ce->getRParenLoc();

    /* Handling macro. */
    if (ret.isMacroID()) 
    {
      pair<SourceLocation, SourceLocation> expansionRange = 
          rewriter.getSourceMgr().getImmediateExpansionRange(ret);

      ret = expansionRange.second;
    }

    ret = ret.getLocWithOffset(1);
  }
  else if (isa<DeclRefExpr>(e) || isa<MemberExpr>(e))
  {
    if (print) cout << "cp 4\n";
    int length = ConvertToString(e, comp_inst->getLangOpts()).length();
    SourceLocation start_loc = e->getLocStart();

    if (GetLineNumber(src_mgr, start_loc) == 0 ||
        GetColumnNumber(src_mgr, start_loc) + length == 0)
      goto done;

    // cout << "cp GetEndLocOfExpr\n";
    ret = src_mgr.translateLineCol(
        src_mgr.getMainFileID(), 
        GetLineNumber(src_mgr, start_loc),
        GetColumnNumber(src_mgr, start_loc) + length);

    if (GetLineNumber(src_mgr, ret) == 0 ||
        GetColumnNumber(src_mgr, ret) - 1 == 0)
      goto done;

    SourceLocation prevLoc = src_mgr.translateLineCol(
        src_mgr.getMainFileID(),
        GetLineNumber(src_mgr, ret),
        GetColumnNumber(src_mgr, ret) - 1);

    if (*(src_mgr.getCharacterData(prevLoc)) == ';')
      ret = prevLoc;

    // if (GetLineNumber(src_mgr, ret) == 2070)
    // {
    //   cout << ConvertToString(e, comp_inst->getLangOpts()) << endl;
    //   cout << ret.printToString(src_mgr) << endl;
    // }
  }
  else if (isa<CharacterLiteral>(e) || 
            isa<FloatingLiteral>(e) || 
            isa<IntegerLiteral>(e))
  {
    if (print) cout << "cp 5\n";
    ret = GetEndLocOfConstantLiteral(src_mgr, e->getLocStart());
  }
  else if (isa<CStyleCastExpr>(e))  // explicit cast
  {
    if (print) cout << "cp 6\n";
    return GetEndLocOfExpr(
      cast<CStyleCastExpr>(e)->getSubExpr()->IgnoreImpCasts(),
      comp_inst);
  }
  else if (ParenExpr *pe = dyn_cast<ParenExpr>(e))
  {
    if (print) cout << "cp 7\n";
    ret = pe->getRParen();

    if (print && ret.isMacroID())
    {
      pair<SourceLocation, SourceLocation> expansionRange = 
          rewriter.getSourceMgr().getImmediateExpansionRange(ret);
      cout << expansionRange.first.printToString(src_mgr) << endl;
      cout << expansionRange.second.printToString(src_mgr) << endl;
      // cout << *(src_mgr.getCharacterData(expansionRange.second)) << endl;
      SourceLocation end_macro = Lexer::getLocForEndOfToken(src_mgr.getExpansionLoc(expansionRange.second), 0, src_mgr, comp_inst->getLangOpts());

      cout << end_macro.printToString(src_mgr) << endl;

      // if (src_mgr.getFileID(end_macro) == src_mgr.getMainFileID())
      //   cout << std::string(src_mgr.getCharacterData(expansionRange.first),
      //     src_mgr.getCharacterData(end_macro)-src_mgr.getCharacterData(expansionRange.first)) << endl;
    }

    ret = ret.getLocWithOffset(1);
  }
  else if (isa<StringLiteral>(e))
  {
    if (print) cout << "cp 8\n";
    ret = GetEndLocOfStringLiteral(src_mgr, e->getLocStart());
  }
  else 
  {
    if (print) cout << "cp 9\n";
    ret = e->getLocEnd();
    // cout << ConvertToString(e, comp_inst->getLangOpts()) << endl;
    // ret.dump(src_mgr);
    // cout << GetLineNumber(src_mgr, ret) << "=:=" << GetColumnNumber(src_mgr, ret) - 1 << endl;
    ret = GetEndLocOfStmt(ret, comp_inst);

    if (ret.isInvalid())
      ret = e->getLocEnd();

    if (GetColumnNumber(src_mgr, ret) == 1 || GetLineNumber(src_mgr, ret) == 0)
      goto done;

    // cout << "cp GetEndLocOfExpr2\n";
    // cout << GetLineNumber(src_mgr, ret) << ":" << GetColumnNumber(src_mgr, ret) - 1 << endl;
    // GetEndLocOfStmt sometimes returns location after semicolon
    SourceLocation prevLoc = src_mgr.translateLineCol(
        src_mgr.getMainFileID(),
        GetLineNumber(src_mgr, ret),
        GetColumnNumber(src_mgr, ret) - 1);

    if (*(src_mgr.getCharacterData(prevLoc)) == ';')
      ret = prevLoc;
  }

  done:
  return ret;
}

string GetVarDeclName(const VarDecl *vd)
{
  return vd->getNameAsString();
}

bool IsVarDeclConst(const VarDecl *vd)
{
  return (vd->getType()).isConstQualified();
}

bool IsVarDeclPointer(const VarDecl *vd)
{
  return ((vd->getType()).getTypePtr())->isPointerType();   
}

bool IsVarDeclArray(const VarDecl *vd)
{
  return ((vd->getType()).getTypePtr())->isArrayType();   
}

bool IsVarDeclScalar(const VarDecl *vd)
{
  return ((vd->getType()).getTypePtr())->isScalarType() && 
          !IsVarDeclPointer(vd);   
}

bool IsVarDeclFloating(const VarDecl *vd)
{
  return ((vd->getType()).getTypePtr())->isFloatingType();
}

bool IsVarDeclStruct(const VarDecl *vd)
{
  return ((vd->getType()).getTypePtr())->isStructureType();
}

/**
  Return the type of array element
  Example: int[] -> int

  @param: type  type of the array
*/
string getArrayElementType(QualType type)
{
  string res;

  // getElementType can only be called from an ArrayType
  if (const ArrayType *type_ptr = dyn_cast<ArrayType>((type.getTypePtr())))
  {
    QualType array_type = type_ptr->getElementType().getCanonicalType();
    res = TrimBeginningAndEndingWhitespace(array_type.getAsString());
  }
  else 
  {
    // Since type parameter is definitely an array type,
    // if somehow cannot convert to ArrayType, use string processing to retrieve element type.
    res = type.getCanonicalType().getAsString();
    auto pos = res.find_last_of('[');
    res = TrimBeginningAndEndingWhitespace(res.substr(0, pos));
  }

  // remove const from element type. why?
  /*string firstWord = res.substr(0, res.find_first_of(' '));
  if (firstWord.compare("const") == 0)
    res = res.substr(6);*/
  
  return res;
}

string getStructureType(QualType type)
{
  return type.getCanonicalType().getAsString();
}

// Return the type of the entity the pointer is pointing to.
string getPointerType(QualType type)
{
  const Type *type_ptr = cast<PointerType>(type.getCanonicalType().getTypePtr());
  return type_ptr->getPointeeType().getCanonicalType().getAsString();
}

// Return True if the 2 types are same
bool sameArrayElementType(QualType type1, QualType type2)
{
  if (!type1.getTypePtr()->isArrayType() || 
      !type2.getTypePtr()->isArrayType())
  {
    cout << "sameArrayElementType: one of the type is not array type\n";
    return false;
  }

  string elementType1 = getArrayElementType(type1);
  string elementType2 = getArrayElementType(type2);

  return elementType1.compare(elementType2) == 0;
}

/** 
  Check if this directory exists

  @param  directory the directory to be checked
  @return True if the directory exists
      False otherwise
*/
bool DirectoryExists(const std::string &directory)
{
  if( !directory.empty() )
  {
    if( access(directory.c_str(), 0) == 0 )
    {
      struct stat status;
      stat( directory.c_str(), &status );
      if( status.st_mode & S_IFDIR )
        return true;
    }
  }
  // if any condition fails
  return false;
}

void PrintUsageErrorMsg()
{
  cout << "Invalid command.\n";
  cout << "Usage: tool <filename> [-m <mutant_name> [-A \"<domain>\"] ";
  cout << "[-B \"<range>\"]] [-rs <line #> <col #>] [-re <line #> <col #>]";
  cout << " [-l <max>] [-o <dir>]" << endl;
}

void PrintLineColNumberErrorMsg()
{
  cout << "Invalid line/column number\n";
  cout << "Usage: [-rs <line #> <col #>] [-re <line #> <col #>]\n";
}

const Stmt* GetParentOfStmt(Stmt *s, CompilerInstance *comp_inst)
{
  const Stmt* stmt_ptr = s;

  //get parents
  const auto& parent_stmt = comp_inst->getASTContext().getParents(*stmt_ptr);

  if (parent_stmt.empty()) {
    return nullptr;
  }
  // cout << "find parent size=" << parents.size() << "\n";
  stmt_ptr = parent_stmt[0].get<Stmt>();
 
  if (!stmt_ptr)
    return nullptr;

  return stmt_ptr;
}

// Mutating an operator can make its lhs target change.
// Return the new lhs of the mutated operator.
// Applicable for multiplicative operator
Expr* GetLeftOperandAfterMutationToMultiplicativeOp(Expr *lhs)
{
  if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
  {
    BinaryOperator::Opcode opcodeOfB = b->getOpcode();
    if (opcodeOfB > BO_Rem)
    {
      // multiplicative operator has the highest precedence.
      // go rhs right away if the operator is not multiplicative.
      return GetLeftOperandAfterMutationToMultiplicativeOp(
          b->getRHS()->IgnoreImpCasts());
    }
    else 
    {
      // * / % yield non-pointer only.
      return lhs;   
    }
  }
  else
    return lhs->IgnoreImpCasts();
}

// Mutating an operator can make its rhs target change.
// Return the new rhs of the mutated operator.
// Applicable for multiplicative operator
Expr* GetRightOperandAfterMutationToMultiplicativeOp(Expr *rhs)
{
  if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
    return GetRightOperandAfterMutationToMultiplicativeOp(
        b->getLHS()->IgnoreImpCasts());
  else
    return rhs->IgnoreImpCasts();
}

BinaryOperator::Opcode TranslateToOpcode(const string &binary_operator)
{
  if (binary_operator.compare("+") == 0)
    return BO_Add;
  if (binary_operator.compare("-") == 0)
    return BO_Sub;
  if (binary_operator.compare("*") == 0)
    return BO_Mul;
  if (binary_operator.compare("/") == 0)
    return BO_Div;
  if (binary_operator.compare("%") == 0)
    return BO_Rem;
  if (binary_operator.compare("<<") == 0)
    return BO_Shl;
  if (binary_operator.compare(">>") == 0)
    return BO_Shr;
  if (binary_operator.compare("|") == 0)
    return BO_Or;
  if (binary_operator.compare("&") == 0)
    return BO_And;
  if (binary_operator.compare("^") == 0)
    return BO_Xor;
  if (binary_operator.compare("<") == 0)
    return BO_LT;
  if (binary_operator.compare(">") == 0)
    return BO_GT;
  if (binary_operator.compare("<=") == 0)
    return BO_LE;
  if (binary_operator.compare(">=") == 0)
    return BO_GE;
  if (binary_operator.compare("==") == 0)
    return BO_EQ;
  if (binary_operator.compare("!=") == 0)
    return BO_NE;
  if (binary_operator.compare("&&") == 0)
    return BO_LAnd;
  if (binary_operator.compare("||") == 0)
    return BO_LOr;
  cout << "cannot translate " << binary_operator << " to opcode\n";
  exit(1);
}

int GetPrecedenceOfBinaryOperator(BinaryOperator::Opcode opcode)
{
  switch (opcode)
  {
    case BO_Mul:
    case BO_Div:
    case BO_Rem:
      return 10;
    case BO_Add:
    case BO_Sub:
      return 9;
    case BO_Shl:
    case BO_Shr:
      return 8;
    case BO_LT:
    case BO_GT:
    case BO_LE:
    case BO_GE:
      return 7;
    case BO_EQ:
    case BO_NE:
      return 6;
    case BO_And:
      return 5;
    case BO_Xor:
      return 4;
    case BO_Or:
      return 3;
    case BO_LAnd:
      return 2;
    case BO_LOr:
      return 1;
    default:
      cout << "Unknown opcode\n";
      exit(1);
  }
  return 0;
}

Expr* GetLeftOperandAfterMutation(
    Expr *lhs, const BinaryOperator::Opcode mutated_opcode)
{
  if (BinaryOperator *bo = dyn_cast<BinaryOperator>(lhs))
  {
    if (GetPrecedenceOfBinaryOperator(bo->getOpcode()) >= \
        GetPrecedenceOfBinaryOperator(mutated_opcode))
      return lhs;
    else
      return GetLeftOperandAfterMutation(
          bo->getRHS()->IgnoreImpCasts(), mutated_opcode);
  }

  return lhs;
}

Expr* GetRightOperandAfterMutation(
    Expr *rhs, const BinaryOperator::Opcode mutated_opcode)
{
  if (BinaryOperator *bo = dyn_cast<BinaryOperator>(rhs))
  {
    if (GetPrecedenceOfBinaryOperator(bo->getOpcode()) > \
        GetPrecedenceOfBinaryOperator(mutated_opcode))
      return rhs;
    else
      return GetRightOperandAfterMutation(
          bo->getLHS()->IgnoreImpCasts(), mutated_opcode);
  }

  return rhs;
}

ostream& operator<<(ostream &stream, const MutantEntry &entry)
{
  stream << "============ entry =============" << endl;
  stream << "proteum line num: " << entry.getProteumStyleLineNum() << endl;
  stream << "start location: "; PrintLocation(entry.src_mgr_, entry.getStartLocation());
  stream << "end location: "; PrintLocation(entry.src_mgr_, entry.getTokenEndLocation());
  stream << "token: " << entry.getToken() << endl;
  stream << "mutated token: " << entry.getMutatedToken() << endl;
  stream << "================================";

  return stream;
}

ostream& operator<<(ostream &stream, const MutantDatabase &database)
{
  for (auto line_map_iter: database.getEntryTable())
  {
    cout << "LINE " << line_map_iter.first << endl;
    for (auto column_map_iter: line_map_iter.second)
    {
      cout << "\tCOL " << column_map_iter.first << endl;
      for (auto mutantname_map_iter: column_map_iter.second)
      {
        cout << "\t\t" << mutantname_map_iter.first << endl;
        for (auto entry: mutantname_map_iter.second)
          cout << entry << endl;
      }
    }
  }

  return stream;
}

// return the first non-ParenExpr inside this Expr e
Expr* IgnoreParenExpr(Expr *e)
{
  Expr *ret = e;

  if (isa<ParenExpr>(ret))
  {
    ParenExpr *pe;

    while (pe = dyn_cast<ParenExpr>(ret))
      ret = pe->getSubExpr()->IgnoreImpCasts();
  }
  
  return ret;
}

void ConvertConstIntExprToIntString(Expr *e, CompilerInstance *comp_inst,
                                    string &str)
{
  llvm::APSInt result;
  if (e->EvaluateAsInt(result, comp_inst->getASTContext()))
    str = result.toString(10);
  
  // if case value is char, convert it to int value
  if (str.front() == '\'' && str.back() == '\'')
    str = ConvertCharStringToIntString(str);
}

void ConvertConstFloatExprToFloatString(Expr *e, CompilerInstance *comp_inst,
                                        string &str)
{
  llvm::APFloat result(0.0);
  if (e->EvaluateAsFloat(result, comp_inst->getASTContext()))
  {
    double val = result.convertToDouble();
    stringstream ss;
    ss << val;
    str = ss.str();
  }
}

bool SortFloatAscending (long double i,long double j) { return (i<j); }
bool SortIntAscending (long long i,long long j) { return (i<j); }
bool SortStringAscending (string i,string j) { return (i.compare(j) < 0); }
