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
#include <map>
#include <limits.h>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Frontend/Utils.h"

#include "clang/Sema/Scope.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/ScopeInfo.h"

using namespace clang;
using namespace std;

typedef pair<int, int> LabelDeclLocation;           // <line number, column number>
typedef vector<SourceLocation> GotoLocationList;    // list of locations of goto statements

// Block scope are bounded by curly braces {}.
// The closer the scope is to the end of vector, the smaller it is.
// NestedBlockScope = {scope1, scope2, scope3, ...}
// {...scope1
//     {...scope2
//         {...scope3
//         }
//     }
// }
typedef vector<SourceRange> NestedBlockScope;  
typedef vector<VarDecl *> DeclInScope;              // list of var decl in a certain scope


typedef vector<pair<string, bool>> GlobalScalarConstants;   // pair<string representation, isFloat?>

// pair<string representation, pair<location, isFloat?>>
typedef vector<pair<string, pair<SourceLocation, bool>>> LocalScalarConstants;

// pair<range of switch statement, list of case values' string representation>
typedef vector<pair<SourceRange, vector<string>>> SwitchCaseTracker;

struct comp
{
    bool operator() (const LabelDeclLocation &lhs, const LabelDeclLocation &rhs) const
    {
        // if lhs appear before rhs, return true
        // else return false.

        if (lhs.first < rhs.first)
            return true;

        if (lhs.first == rhs.first && lhs.second < rhs.second)
            return true;

        return false;
    }
};

// Map label declaration location with its usage locations (goto)
typedef map<LabelDeclLocation, GotoLocationList, comp> LabelUsageMap;

// states used for analyzing user input, used in function handleInput()
enum class State
{
    NONE,                   // expecting an option like -m -l -o -rs -re
    MUTANT_NAME,            // expecting a mutant operator name
    MUTANT_NAME_OPTIONAL,   // expecting a mutant operator name, or another option
    OUTPUT_DIR,             // expecting a directory
    RS_LINE, RS_COL,        // expecting a number
    RE_LINE, RE_COL,        // expecting a number
    LIMIT_MUTANT,           // expecting a number    
    A_OPTIONAL, B_OPTIONAL, // expecting either -A/-B, mutant op name or another option
    A_SDL_LINE, A_SDL_COL,  // expecting a number
    B_SDL_LINE, B_SDL_COL,  // expecting a number
    A_OP,                   // expecting domain for previous mutant operator
    B_OP,                   // expecting range for previous mutant operator
};


// Print out each elemet of a string vector in a single line.
void print_vec(vector<string> &vec)
{
    for (vector<string>::iterator x = vec.begin(); x != vec.end(); ++x) {
         std::cout << "//" << *x;
    }
    std::cout << "//\n";
}

// Print out each element of a string set in a single line.
void print_set(set<string> &set)
{
    for (auto e: set)
    {
        cout << "//" << e;
    }
    cout << "//\n";
}

// remove spaces at the beginning and end of string str
string trimSpaces(string str)
{
    string whitespace{" "};

    auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == string::npos)
        return ""; // no content

    auto strEnd = str.find_last_not_of(whitespace);
    auto strRange = strEnd - strBegin + 1;
    return str.substr(strBegin, strRange);
}

// Divide string into elements of a string set with delimiter
void split_string_into_set(string target, set<string> &output, string delimiter)
{
    size_t pos = 0;

    string token;
    while ((pos = target.find(delimiter)) != string::npos) {
        token = target.substr(0, pos);
        output.insert(trimSpaces(token));
        target.erase(0, pos + delimiter.length());
    }
    output.insert(trimSpaces(target));
}

void split_string_into_vector(string target, vector<string> &output, string delimiter)
{
    size_t pos = 0;

    string token;
    while ((pos = target.find(delimiter)) != string::npos) {
        token = target.substr(0, pos);
        output.push_back(token);
        target.erase(0, pos + delimiter.length());
    }
    output.push_back(target);
}

// Return the number of not-newline character
int count_non_enter(string &s)
{  
    int res = 0;
    for (int i = 0; i < s.length(); ++i)
    {
        if (s[i] != '\n')
            ++res;
    }
    return res;
}

bool convertStringToInt(string s, int &n)
{
    stringstream convert(s);
    if (!(convert >> n))
        return false;
    return true;
}

/** 
    Check whether the domain of an operator specified by user is correct

    @param  op name of the operator
            replacee string set containing targeted tokens
*/
void validate_replacee(string &op, set<string> &replacee) 
{  
    // set of operators mutating logical operator
    set<string> logical{"OLLN", "OLAN", "OLBN", "OLRN", "OLSN"};

    // set of operators mutating relational operator
    set<string> relational{"ORLN", "ORAN", "ORBN", "ORRN", "ORSN"};

    // set of operators mutating arithmetic operator
    set<string> arithmetic{"OALN", "OAAN", "OABN", "OARN", "OASN"};

    // set of operators mutating bitwise operator
    set<string> bitwise{"OBLN", "OBAN", "OBBN", "OBRN", "OBSN"};

    // set of operators mutating shift operator
    set<string> shift{"OSLN", "OSAN", "OSBN", "OSRN", "OSSN"};

    // set of operators mutating arithmetic assignment operator
    set<string> arithmeticAssignment{"OAAA", "OABA", "OAEA", "OASA"};

    // set of operators mutating bitwise assignment operator
    set<string> bitwiseAssignment{"OBAA", "OBBA", "OBEA", "OBSA"};

    // set of operators mutating shift assignment operator
    set<string> shiftAssignment{"OSAA", "OSBA", "OSEA", "OSSA"};

    // set of operators mutating plain assignment operator
    set<string> plainAssignment{"OEAA", "OEBA", "OESA"};

    // set of operators whose domain cannot be altered.
    set<string> empty_replacee{"CRCR", "SSDL", "VLSR", "VGSR", "VGAR", "VLAR", 
                            "VGTR", "VLTR", "VGPR", "VLPR", "VSCR", "CGCR", "CLCR",
                            "CGSR", "CLSR", "OMMO", "OPPO", "OLNG", "OCNG", "OBNG"};

    // Determine the set of valid tokens based on the type of operator
    set<string> valid;
    if (logical.find(op) != logical.end())
    {
        valid = {"&&", "||"};
    }
    else if (relational.find(op) != relational.end())
    {
        valid = {">", "<", "<=", ">=", "==", "!="};
    } 
    else if (arithmetic.find(op) != arithmetic.end())
    {
        valid = {"+", "-", "*", "/", "%"};
    }
    else if (bitwise.find(op) != bitwise.end())
    {
        valid = {"&", "|", "^"};
    }
    else if (shift.find(op) != shift.end())
    {
        valid = {"<<", ">>"};
    }
    else if (arithmeticAssignment.find(op) != arithmeticAssignment.end())
    {
        valid = {"+=", "-=", "*=", "/=", "%="};
    }
    else if (bitwiseAssignment.find(op) != bitwiseAssignment.end())
    {
        valid = {"&=", "|=", "^="};
    }
    else if (shiftAssignment.find(op) != shiftAssignment.end())
    {
        valid = {"<<=", ">>="};
    }
    else if (plainAssignment.find(op) != plainAssignment.end())
    {
        valid = {"="};
    }
    else if (empty_replacee.find(op) != empty_replacee.end())
    {
        if (!replacee.empty())
        {
            replacee.clear();
            cout << "-A options are not available for " << op << endl;
        }
        return;
    }
    else
    {
        cout << "Cannot identify mutant operator " << op << endl;
        exit(1);
    }

    // if replacee is not empty, then check if each element in the set is valid
    if (replacee.size() > 0)
    {
        for (auto e: replacee)
        {
            if (valid.find(e) == valid.end())
            {
                cout << "Mutant operator " << op << " cannot mutate " << e << "\n";
                exit(1);
            }
        }
    }
    else
    {
        // user did not specify domain for this operator
        // So mutate every valid token for this operator.
        replacee = valid;
    }
}

/**
    Check whether a string is a number (int or real)

    @param  num targeted string
    @return true if it is
            false otherwise
*/
bool validate_number(string num)
{
    bool firstDot = true;

    for (int i = 0; i < num.length(); ++i)
    {
        //-------add test of string length to limit number

        if (isdigit(num[i]))
            continue;

        // the dash cannot only appear at the first position
        // and there must be numbers behind it
        if (num[i] == '-' && i == 0 && num.length() > 1)
            continue;

        // prevent cases of 2 dot in a number
        if (num[i] == '.' && firstDot)
        {
            firstDot = false;
            continue;
        }

        return false;
    }
    return true;
}

/** 
    Check whether the range of an operator specified by user is correct

    @param  op name of the operator
            replacer string set containing tokens to mutate with
*/
void validate_replacer(string &op, set<string> &replacer) 
{
    // set of operators mutating to a logical operator
    set<string> logical{"OALN", "OBLN", "OLLN", "ORLN", "OSLN"};

    // set of operators mutating to a relational operator
    set<string> relational{"OLRN", "OARN", "OBRN", "ORRN", "OSRN"};

    // set of operators mutating to a arithmetic operator
    set<string> arithmetic{"OLAN", "OAAN", "OBAN", "ORAN", "OSAN"};

    // set of operators mutating to a arithmetic operator
    set<string> bitwise{"OLBN", "OABN", "OBBN", "ORBN", "OSBN"};

    // set of operators mutating to a arithmetic operator
    set<string> shift{"OLSN", "OASN", "OBSN", "ORSN", "OSSN"};

    // set of operators mutating to arithmetic assignment operator
    set<string> arithmeticAssignment{"OAAA", "OBAA", "OEAA", "OSAA"};

    // set of operators mutating to bitwise assignment operator
    set<string> bitwiseAssignment{"OABA", "OBBA", "OEBA", "OSBA"};

    // set of operators mutating to shift assignment operator
    set<string> shiftAssignment{"OASA", "OBSA", "OESA", "OSSA"};

    // set of operators mutating to plain assignment operator
    set<string> plainAssignment{"OAEA", "OBEA", "OSEA"};

    // set of operators whose range cannot be altered.
    set<string> empty_replacer{"SSDL", "VLSR", "VGSR", "VGAR", "VLAR", "VGTR",
                                "VLTR", "VGPR", "VLPR", "VSCR", "CGCR", "CLCR",
                                "CGSR", "CLSR", "OMMO", "OPPO", "OLNG", "OCNG",
                                "OBNG"};

    // Determine the set of valid tokens based on the type of operator
    set<string> valid;  
    if (logical.find(op) != logical.end())
    {
        valid = {"&&", "||"};
    }
    else if (relational.find(op) != relational.end())
    {
        valid = {">", "<", "<=", ">=", "==", "!="};
    } 
    else if (arithmetic.find(op) != arithmetic.end())
    {
        valid = {"+", "-", "*", "/", "%"};
    }
    else if (bitwise.find(op) != bitwise.end())
    {
        valid = {"&", "|", "^"};
    }
    else if (shift.find(op) != shift.end())
    {
        valid = {"<<", ">>"};
    }
    else if (arithmeticAssignment.find(op) != arithmeticAssignment.end())
    {
        valid = {"+=", "-=", "*=", "/=", "%="};
    }
    else if (bitwiseAssignment.find(op) != bitwiseAssignment.end())
    {
        valid = {"&=", "|=", "^="};
    }
    else if (shiftAssignment.find(op) != shiftAssignment.end())
    {
        valid = {"<<=", ">>="};
    }
    else if (plainAssignment.find(op) != plainAssignment.end())
    {
        valid = {"="};
    }
    else if (op.compare("CRCR") == 0)
    {
        if (replacer.size() > 0)
        {
            for (auto num = replacer.begin(); num != replacer.end(); )
            {
                // if user input something not a number, it is ignored. (delete from the set)
                if (!validate_number(*num))
                {
                    cout << "Warning: " << *num << " is not a number -> ignored.\n";
                    num = replacer.erase(num);
                }  
                else
                    ++num;
            }
        }
        return;
    }
    else if (empty_replacer.find(op) != empty_replacer.end())
    {
        if (!replacer.empty())
        {
            replacer.clear();
            cout << "-B options are not available for " << op << endl;
        }
        return;
    }
    else
    {
        cout << "Cannot identify mutant operator " << op << endl;
        exit(1);
    }

    // if replacer is not empty, then check if each element in the set is valid
    if (replacer.size() > 0) {
        for (auto e: replacer)
        {
            if (valid.find(e) == valid.end())
            {
                cout << "Mutant operator " << op << " cannot mutate to " << e << "\n";
                exit(1);
            }
        }
    }
    else {
        // user did not specify range for this operator
        // So mutate to every valid token for this operator.
        replacer = valid;
    }
}

/**
    Represent a mutant operator

    @param  name of the operator.
            set of tokens that the operator can mutate.
            set of tokens that the operator can mutate to.
*/
class MutantOperator
{
public:
    string m_name;
    set<string> m_targets;
    set<string> m_replaceOptions;

    MutantOperator(string name, set<string> targets, set<string> options)
        :m_name(name), m_targets(targets), m_replaceOptions(options)
    {
        cout << "using mutant operator " << name << endl;
        cout << "Domain: "; print_set(m_targets);
        cout << "Range:  "; print_set(m_replaceOptions);
    }   

    ~MutantOperator(){}

    string& getName() {return m_name;}
    
    /*bool operator<(const MutantOperator &op) const
    {
        return m_name < op.m_name;
    }*/
};

/**
    Hold all the mutant operators that will be applied to the input file.

    @param  vectors containing Operator MutantOperator mutating arithmetic op,
            arithmetic assignment, bitwise op, bitwise assignment,
            shift op, shift assignment, logical op, relational op,
            plain assignment.

            CRCR_integral   
*/
class MutantOperatorHolder
{
public:
    vector<MutantOperator> bin_arith;
    vector<MutantOperator> bin_arithAssign;
    vector<MutantOperator> bin_bitwise;
    vector<MutantOperator> bin_bitwiseAssign;
    vector<MutantOperator> bin_shift;
    vector<MutantOperator> bin_shiftAssign;
    vector<MutantOperator> bin_logical;
    vector<MutantOperator> bin_relational;
    vector<MutantOperator> bin_plainAssign;

    set<string> CRCR_integral;
    set<string> CRCR_floating;

    bool doSSDL;
    bool doVLSR;
    bool doVGSR;
    bool doVGAR;
    bool doVLAR;
    bool doVGTR;
    bool doVLTR;
    bool doVGPR;
    bool doVLPR;
    bool doVSCR;
    bool doCGCR;
    bool doCLCR;
    bool doCGSR;
    bool doCLSR;
    bool doOPPO;
    bool doOMMO;
    bool doOLNG;
    bool doOBNG;
    bool doOCNG;

    MutantOperatorHolder()
    {
        doSSDL = false;
        doVLSR = false;
        doVGSR = false;
        doVGAR = false;
        doVLAR = false;
        doVGTR = false;
        doVLTR = false;
        doVGPR = false;
        doVLPR = false;
        doVSCR = false;
        doCGCR = false;
        doCLCR = false;
        doCGSR = false;
        doCLSR = false;
        doOPPO = false;
        doOMMO = false;
        doOLNG = false;
        doOBNG = false;
        doOCNG = false; 
    }
    ~MutantOperatorHolder(){}

    /**
        Check if a given string is a float number. (existence of dot(.))

        @param  num targeted string
        @return true if it is
                false if it is an int
    */
    bool isFloat(string num) 
    {
        for (int i = 0; i < num.length(); ++i)
        {
            if (num[i] == '.')
                return true;
        }
        return false;
    }

    /**
        Put a new mutant operator into the correct MutantOperator vector
        based on the token it can mutate.

        @param  op new mutant operator to be added.
        @return True if the operator is added successfully.
                False if there is the operator cannot be added into any vector.
                The operator is not supported by the tool (yet).
    */
    bool addOperator(MutantOperator &op)
    {
        // set of operators mutating logical operator
        set<string> logical{"OLLN", "OLAN", "OLBN", "OLRN", "OLSN"};

        // set of operators mutating relational operator.
        set<string> relational{"ORLN", "ORAN", "ORBN", "ORRN", "ORSN"};

        // set of operators mutating arithmetic operators.
        set<string> arithmetic{"OALN", "OAAN", "OABN", "OARN", "OASN"};

        // set of operators mutating bitwise operator
        set<string> bitwise{"OBLN", "OBAN", "OBBN", "OBRN", "OBSN"};

        // set of operators mutating shift operator
        set<string> shift{"OSLN", "OSAN", "OSBN", "OSRN", "OSSN"};

        // set of operators mutating arithmetic assignment operator
        set<string> arithmeticAssignment{"OAAA", "OABA", "OAEA", "OASA"};

        // set of operators mutating bitwise assignment operator
        set<string> bitwiseAssignment{"OBAA", "OBBA", "OBEA", "OBSA"};

        // set of operators mutating shift assignment operator
        set<string> shiftAssignment{"OSAA", "OSBA", "OSEA", "OSSA"};

        // set of operators mutating plain assignment operator
        set<string> plainAssignment{"OEAA", "OEBA", "OESA"};

        // Check which set does the operator belongs to.
        // Add the operator to the corresponding vector.
        if (logical.find(op.getName()) != logical.end())
        {
            bin_logical.push_back(op);
        }
        else if (relational.find(op.getName()) != relational.end())
        {
            bin_relational.push_back(op);
        }
        else if (arithmetic.find(op.getName()) != arithmetic.end())
        {
            bin_arith.push_back(op);
        }
        else if (bitwise.find(op.getName()) != bitwise.end())
        {
            bin_bitwise.push_back(op);
        }
        else if (shift.find(op.getName()) != shift.end())
        {
            bin_shift.push_back(op);
        }
        else if (arithmeticAssignment.find(op.getName()) != arithmeticAssignment.end())
        {
            bin_arithAssign.push_back(op);
        }
        else if (bitwiseAssignment.find(op.getName()) != bitwiseAssignment.end())
        {
            bin_bitwiseAssign.push_back(op);
        }
        else if (shiftAssignment.find(op.getName()) != shiftAssignment.end())
        {
            bin_shiftAssign.push_back(op);
        }
        else if (plainAssignment.find(op.getName()) != plainAssignment.end())
        {
            bin_plainAssign.push_back(op);
        }
        else if (op.getName().compare("CRCR") == 0) 
        {
            CRCR_integral = {"0", "1", "-1"};
            CRCR_floating = {"0.0", "1.0", "-1.0"};

            for (auto it: op.m_replaceOptions)
            {
                if (isFloat(it))
                    CRCR_floating.insert(it);
                else
                    CRCR_integral.insert(it);
            }

            cout << "crcr float\n";
            print_set(CRCR_floating);
            cout << "crcr integral\n";
            print_set(CRCR_integral);
        }
        else if (op.getName().compare("SSDL") == 0)
        {
            doSSDL = true;
        }
        else if (op.getName().compare("VLSR") == 0)
        {
            doVLSR = true;
        }
        else if (op.getName().compare("VGSR") == 0)
        {
            doVGSR = true;
        }
        else if (op.getName().compare("VGAR") == 0)
        {
            doVGAR = true;
        }
        else if (op.getName().compare("VLAR") == 0)
        {
            doVLAR = true;
        }
        else if (op.getName().compare("VGTR") == 0)
        {
            doVGTR = true;
        }
        else if (op.getName().compare("VLTR") == 0)
        {
            doVLTR = true;
        }
        else if (op.getName().compare("VGPR") == 0)
        {
            doVGPR = true;
        }
        else if (op.getName().compare("VLPR") == 0)
        {
            doVLPR = true;
        }
        else if (op.getName().compare("VSCR") == 0)
        {
            doVSCR = true;
        }
        else if (op.getName().compare("CGCR") == 0)
        {
            doCGCR = true;
        }
        else if (op.getName().compare("CLCR") == 0)
        {
            doCLCR = true;
        }
        else if (op.getName().compare("CGSR") == 0)
        {
            doCGSR = true;
        }
        else if (op.getName().compare("CLSR") == 0)
        {
            doCLSR = true;
        }
        else if (op.getName().compare("OMMO") == 0)
        {
            doOMMO = true;
        }
        else if (op.getName().compare("OPPO") == 0)
        {
            doOPPO = true;
        }
        else if (op.getName().compare("OLNG") == 0)
        {
            doOLNG = true;
        }
        else if (op.getName().compare("OCNG") == 0)
        {
            doOCNG = true;
        }
        else if (op.getName().compare("OBNG") == 0)
        {
            doOBNG = true;
        }
        else {
            // The operator does not belong to any of the supported categories
            return false;
        }

        return true;
    }

    void addOperatorMutantOperator(string op)
    {
        set<string> domain;
        set<string> range;

        validate_replacer(op, range);
        validate_replacee(op, domain);
        
        MutantOperator newOp{op, domain, range};
        addOperator(newOp);
    }

    /**
        Add to each Mutant Operator vector all the MutantOperator that it
        can receive, with tokens before and after mutation of each token
        same as in Agrawal's Design of Mutation Operators for C.        
        Called when user does not specify any specific operator(s) to use.
    */
    void useAll()
    {
        addOperatorMutantOperator(string("OLLN"));
        addOperatorMutantOperator(string("OLAN"));
        addOperatorMutantOperator(string("OLBN"));
        addOperatorMutantOperator(string("OLRN"));
        addOperatorMutantOperator(string("OLSN"));

        addOperatorMutantOperator(string("ORLN"));
        addOperatorMutantOperator(string("ORAN"));
        addOperatorMutantOperator(string("ORBN"));
        addOperatorMutantOperator(string("ORRN"));
        addOperatorMutantOperator(string("ORSN"));

        addOperatorMutantOperator(string("OALN"));
        addOperatorMutantOperator(string("OAAN"));
        addOperatorMutantOperator(string("OABN"));
        addOperatorMutantOperator(string("OARN"));
        addOperatorMutantOperator(string("OASN"));

        addOperatorMutantOperator(string("OBLN"));
        addOperatorMutantOperator(string("OBAN"));
        addOperatorMutantOperator(string("OBBN"));
        addOperatorMutantOperator(string("OBRN"));
        addOperatorMutantOperator(string("OBSN"));

        addOperatorMutantOperator(string("OSLN"));
        addOperatorMutantOperator(string("OSAN"));
        addOperatorMutantOperator(string("OSBN"));
        addOperatorMutantOperator(string("OSRN"));
        addOperatorMutantOperator(string("OSSN"));

        addOperatorMutantOperator(string("OAAA"));
        addOperatorMutantOperator(string("OABA"));
        addOperatorMutantOperator(string("OAEA"));
        addOperatorMutantOperator(string("OASA"));

        addOperatorMutantOperator(string("OBAA"));
        addOperatorMutantOperator(string("OBBA"));
        addOperatorMutantOperator(string("OBEA"));
        addOperatorMutantOperator(string("OBSA"));

        addOperatorMutantOperator(string("OSAA"));
        addOperatorMutantOperator(string("OSBA"));
        addOperatorMutantOperator(string("OSEA"));
        addOperatorMutantOperator(string("OSSA"));

        addOperatorMutantOperator(string("OEAA"));
        addOperatorMutantOperator(string("OEBA"));
        addOperatorMutantOperator(string("OESA"));


        CRCR_integral = {"0", "1", "-1"};
        CRCR_floating = {"0.0", "1.0", "-1.0"};

        doSSDL = true;
        doVLSR = true;
        doVGSR = true;
        doVGAR = true;
        doVLAR = true;
        doVGTR = true;
        doVLTR = true;
        doVGPR = true;
        doVLPR = true;
        doVSCR = true;
        doCGCR = true;
        doCLCR = true;
        doCGSR = true;
        doCLSR = true;
        doOPPO = true;
        doOMMO = true;
        doOLNG = true;
        doOBNG = true;
        doOCNG = true;    
    }
};

/**
    Contain the interpreted user input based on command option(s).

    @param  m_inFilename            name of input file
            m_mutDbFilename         name of mutation database file
            m_rangeStartLocation    start location of mutation range
            m_rangeEndLocation      end location of mutation range
            m_outputDir             output directory of generated files
            m_limit                 max number of mutants per mutation point per mutant operator
*/
class UserInput 
{
public:
    string m_inFilename;
    string m_mutDbFilename;
    SourceLocation m_rangeStartLocation;
    SourceLocation m_rangeEndLocation;
    string m_outputDir;
    int m_limit;

    UserInput(string inFile, string mutDbFile, SourceLocation startloc,
                SourceLocation endloc, string dir = "./", int lim = 0)
        :m_inFilename(inFile), m_mutDbFilename(mutDbFile), m_rangeStartLocation(startloc),
        m_rangeEndLocation(endloc), m_outputDir(dir), m_limit(lim) 
    {
    } 

    // Accessors for each member variable.
    string getInputFilename() {return m_inFilename;}
    string getMutDbFilename() {return m_mutDbFilename;}
    SourceLocation* getStartOfRange() {return &m_rangeStartLocation;}
    SourceLocation* getEndOfRange() {return &m_rangeEndLocation;}
    string getOutputDir() {return m_outputDir;}
    int getLimit() {return m_limit;}
};

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
    SourceManager &m_srcmgr;
    LangOptions &m_langopt;

    // ID of the next mutated code file
    int m_mutFileNum;

    // name of mutated code file = m_outFilename + m_mutFileNum
    // m_outFilename = (input filename without .c) + ".MUT"
    string m_outFilename;

    UserInput *m_userinput;

    // this object holds all info about mutant operators to be applied to input file.
    MutantOperatorHolder *m_holder;
    
    CompilerInstance *m_compinst;
    // ASTContext &m_astcontext;

    Rewriter m_rewriter;

    bool checkForZero;
    SourceRange *targetExpr;

    // True when visit Statement Expression,
    // False when out of Statement Expression.
    bool isInsideStmtExpr;

    // True when enter array size of array declaration
    // False when out of array size of array declaration.
    bool isInsideArraySize;

    // Range of the latest parsed array declaration statement
    SourceRange *arrayDeclRange;

    // True if the next Binary Operator is the expression evaluating to array size.
    bool firstBinOpAfterArrayDecl;

    // Expression evaluating to array size.
    // Used to prevent generating-negative-array-size mutants.
    BinaryOperator *arraySize;

    // Store the locations of all the labels inside input file.
    vector<SourceLocation> *m_labels;

    // Map label name to label usages (goto)
    LabelUsageMap *m_labelUsageMap;

    NestedBlockScope m_currentScope;

    GlobalScalarConstants *m_allGlobalConsts;
    LocalScalarConstants *m_allLocalConsts;

    vector<VarDecl *> m_allGlobalScalars;
    vector<DeclInScope> m_allLocalScalars;

    vector<VarDecl *> m_allGlobalArrays;
    vector<DeclInScope> m_allLocalArrays;

    vector<VarDecl *> m_allGlobalStructs;
    vector<DeclInScope> m_allLocalStructs;

    vector<VarDecl *> m_allGlobalPointers;
    vector<DeclInScope> m_allLocalPointers;

    // bool isInsideFunctionDecl;
    SourceRange *currentFunctionDeclRange;
    SourceRange *lhsOfAssignment;
    SourceRange *switchConditionRange;
    SourceRange *typedefRange;
    SourceRange *addressOfOpRange;
    SourceRange *functionPrototypeRange;
    SourceRange *unaryIncrementRange;
    SourceRange *unaryDecrementRange;
    SourceRange *arraySubscriptRange;
    // variable declaration inside a struct or union is called field declaration
    SourceRange *fieldDeclRange; 
    SourceRange *switchCaseRange;   

    Sema *m_sema;

    bool isInsideEnumDecl;

    SwitchCaseTracker m_switchCaseTracker;

    MyASTVisitor(SourceManager &sm, LangOptions &langopt, UserInput *userInput,  
                MutantOperatorHolder *holder, CompilerInstance *CI, vector<SourceLocation> *labels,
                LabelUsageMap *usageMap, GlobalScalarConstants *globalConsts, 
                LocalScalarConstants *localConsts) 
        : m_srcmgr(sm), m_langopt(langopt), m_holder(holder),
        m_userinput(userInput), m_mutFileNum(1), m_compinst(CI),
        m_labels(labels), m_labelUsageMap(usageMap),
        m_allGlobalConsts(globalConsts), m_allLocalConsts(localConsts)
    {
        // name of mutated code file = m_outFilename + m_mutFileNum
        // m_outFilename = (input filename without .c) + ".MUT"
        m_outFilename.assign(m_userinput->getInputFilename(), 0, (m_userinput->getInputFilename()).length()-2);
        m_outFilename += ".MUT";

        m_rewriter.setSourceMgr(m_srcmgr, m_langopt);

        checkForZero = false;
        isInsideStmtExpr = false;
        isInsideArraySize = false;
        firstBinOpAfterArrayDecl = false;
        // isInsideFunctionDecl = false;
        isInsideEnumDecl = false;

        lhsOfAssignment = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));
        switchConditionRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));
        typedefRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));
        addressOfOpRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));
        functionPrototypeRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));
        unaryIncrementRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));
        unaryDecrementRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));
        currentFunctionDeclRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));
        arraySubscriptRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));
        fieldDeclRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID())); 
        switchCaseRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                            m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID())); 

        arrayDeclRange = nullptr;
        targetExpr = nullptr;
    }

    void printIdentifierTable(const ASTContext &astcontext)
    {
        IdentifierTable &identTable = astcontext.Idents;
        // identTable.AddKeywords(m_langopt);
        for (IdentifierTable::iterator e = identTable.begin(); e != identTable.end(); ++e)
        {
            cout << string(e->first()) << endl;
        }
    }

    void setSema(Sema *sema)
    {
        m_sema = sema;
    }

    string getVarDeclName(VarDecl *vd)
    {
        return vd->getNameAsString();
    }

    bool varDeclIsConst(VarDecl *vd)
    {
        return (vd->getType()).isConstQualified();
    }

    bool varDeclIsPointer(VarDecl *vd)
    {
        return ((vd->getType()).getTypePtr())->isPointerType();   
    }

    bool varDeclIsArray(VarDecl *vd)
    {
        return ((vd->getType()).getTypePtr())->isArrayType();   
    }

    bool varDeclIsScalar(VarDecl *vd)
    {
        return ((vd->getType()).getTypePtr())->isScalarType() && !varDeclIsPointer(vd);   
    }

    bool varDeclIsFloat(VarDecl *vd)
    {
        return ((vd->getType()).getTypePtr())->isFloatingType();
    }

    bool varDeclIsStruct(VarDecl *vd)
    {
        return ((vd->getType()).getTypePtr())->isStructureType();
    }

    void printAllGlobalScalars()
    {
        cout << "all global scalars:\n";
        for (auto e: m_allGlobalScalars)
        {
            cout << getVarDeclName(e) << "\t" << varDeclIsConst(e) << endl;
        }
        cout << "end all global scalars\n";
    }

    void printAllGlobalArrays()
    {
        cout << "all global arrays:\n";
        for (auto e: m_allGlobalArrays)
        {
            cout << getVarDeclName(e) << "\t" << varDeclIsConst(e) << endl;
        }
        cout << "end all global arrays\n";
    }

    void printAllLocalScalars()
    {
        cout << "all local scalars:\n";
        for (auto e: m_allLocalScalars)
        {
            cout << "new scope\n";
            for (auto scalar: e)
                cout << getVarDeclName(scalar) << "\t" << varDeclIsConst(scalar) << endl;
        }
        cout << "end all local scalars\n";
    }

    void printAllLocalArrays()
    {
        cout << "all local arrays:\n";
        for (auto e: m_allLocalArrays)
        {
            cout << "new scope\n";
            for (auto scalar: e)
                cout << getVarDeclName(scalar) << "\t" << varDeclIsConst(scalar) << endl;
        }
        cout << "end all local arrays\n";
    }

    // remove spaces at the beginning and end of string str
    string trim(string str)
    {
        string whitespace{" "};

        auto strBegin = str.find_first_not_of(whitespace);
        if (strBegin == string::npos)
            return ""; // no content

        auto strEnd = str.find_last_not_of(whitespace);
        auto strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }

    /**
        Return the type of array element
        Example: int[] -> int

        @param: type    type of the array
    */
    string getArrayElementType(QualType type)
    {
        string res;

        // getElementType can only be called from an ArrayType
        if (dyn_cast<ArrayType>((type.getTypePtr())))
        {
            res = trim(cast<ArrayType>(type.getTypePtr())->getElementType().getCanonicalType().getAsString());
        }
        else 
        {
            // Since type parameter is definitely an array type,
            // if somehow cannot convert to ArrayType, use string processing to retrieve element type.
            res = type.getCanonicalType().getAsString();
            auto pos = res.find_last_of('[');
            res = trim(res.substr(0, pos));
        }

        // remove const from element type
        string firstWord = res.substr(0, res.find_first_of(' '));
        if (firstWord.compare("const") == 0)
            res = res.substr(6);
        return res;
    }

    string getStructureType(QualType type)
    {
        return type.getCanonicalType().getAsString();
    }

    string getPointerType(QualType type)
    {
        return cast<PointerType>(type.getCanonicalType().getTypePtr())->getPointeeType().getCanonicalType().getAsString();
    }

    bool sameArrayElementType(QualType type1, QualType type2)
    {
        if (!type1.getTypePtr()->isArrayType() || !type2.getTypePtr()->isArrayType())
        {
            cout << "sameArrayElementType: one of the type is not array type\n";
            return false;
        }

        string elementType1 = getArrayElementType(type1);
        string elementType2 = getArrayElementType(type2);

        return elementType1.compare(elementType2) == 0;
    }

    // Return True if location loc appears before range. 
    // (location 1,1 appears before every other valid locations)
    bool locationBeforeRange(SourceLocation loc, SourceRange range)
    {
        SourceLocation start = range.getBegin();

        return (getLineNumber(&loc) < getLineNumber(&start) ||
                (getLineNumber(&loc) == getLineNumber(&start) && getColNumber(&loc) < getColNumber(&start)));
    }

    // Return True if location loc appears after range. 
    // (EOF appears after every other valid locations)
    bool locationAfterRange(SourceLocation loc, SourceRange range)
    {
        SourceLocation end = range.getEnd();

        return (getLineNumber(&loc) > getLineNumber(&end) ||
                (getLineNumber(&loc) == getLineNumber(&end) && getColNumber(&loc) > getColNumber(&end)));
    }

    // Return True if the location is inside the range
    // Return False otherwise
    bool locationIsInRange(SourceLocation loc, SourceRange range)
    {
        SourceLocation start = range.getBegin();
        SourceLocation end = range.getEnd();

        // line number of loc is out of range
        if (getLineNumber(&loc) < getLineNumber(&start) || getLineNumber(&loc) > getLineNumber(&end))
            return false;

        // line number of loc is same as line number of start but col number of loc is out of range
        if (getLineNumber(&loc) == getLineNumber(&start) && getColNumber(&loc) < getColNumber(&start))
            return false;

        // line number of loc is same as line number of end but col number of loc is out of range
        if (getLineNumber(&loc) == getLineNumber(&end) && getColNumber(&loc) > getColNumber(&end))
            return false;

        return true;
    }

    // Return True if there is no labels inside specified code range.
    // Return False otherwise.
    bool noLabelsInside(SourceRange stmtRange)
    {
        // Return false if there is a label inside the range
        for (auto label: *(m_labels))
        {
            if (locationIsInRange(label, stmtRange))
            {
                return false;
            }
        }

        return true;
    }

    // an unremovable label is a label defined inside range stmtRange,
    // but goto-ed outside of range stmtRange.
    // Deleting such label can cause goto-undefined-label error.
    bool noUnremoveableLabelInside(SourceRange stmtRange)
    {
        for (auto label: *m_labelUsageMap)
        {
            SourceLocation loc = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                                            label.first.first,
                                                            label.first.second);
            // check only those labels inside stmtRange
            if (locationIsInRange(loc, stmtRange))
            {
                // check if this label is goto-ed from outside of the statement
                // if yes then the label is unremovable, return False
                for (auto e: label.second)  // traverse each location of goto this label
                {   
                    if (!locationIsInRange(e, stmtRange))  // the goto is outside of the statement
                        return false;
                }
            }

            // the LabelUsageMap is traversed in the order of increasing location
            // if the location is after the range then all the rest is outside statement range
            if (locationAfterRange(loc, stmtRange))
                return true;
        }

        return true;
    }

    // Return True if given operator is an arithmetic operator.
    bool isArithmeticOperator(const string &op) 
    {
        return op == "+" || op == "-" || 
                op == "*" || op == "/" || op == "%";
    }

    // Return the line number of a source location.
    int getLineNumber(SourceLocation *loc) 
    {
        return static_cast<int>(m_srcmgr.getExpansionLineNumber(*loc));
    }

    // Return the column number of a source location.
    int getColNumber(SourceLocation *loc) 
    {
        return static_cast<int>(m_srcmgr.getExpansionColumnNumber(*loc));
    }   

    string convertIntToString(int num)
    {
        stringstream convert;
        convert << num;
        string ret;
        convert >> ret;
        return ret;
    }

    string convertHexaStringToIntString(string hexa)
    {
        char first = hexa.at(3);
        if (!((first >= '0' && first <= '9')
                || (first >= 'a' && first <= 'f')))
            return hexa;

        char second = hexa.at(4);
        if (second != '\'')
            if (!((second >= '0' && second <= '9')
                || (second >= 'a' && second <= 'f')))
                return hexa;

        return to_string(stoul(hexa.substr(3, hexa.length() - 4), nullptr, 16));
    }

    string convertCharStringToIntString(string s)
    {
        // it is a single, non-escaped character like 'a'
        if (s.length() == 3)
            return to_string(int(s.at(1)));

        if (s.at(1) == '\\')
        {
            int length = s.length() - 3;
            // it is an escaped character like '\n'
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
                        return convertHexaStringToIntString(s);
                    else 
                        return s;   // hexadecimal value higher than FF. not a char
                default:
                    return s;    
            }
        }
        
        // the function does not handle cases like 'abc'
        cout << "cannot convert " << s << " to string of int\n";
        return s;
    }

    // Return the name of the next mutated code file
    string getNextMutFilename() {
        string ret(m_userinput->getOutputDir());
        ret += m_outFilename;
        ret += convertIntToString(m_mutFileNum);
        ret += ".c";
        return ret;
    }

    /**
        Write information about the next mutated code file into mutation database file.

        @param  op name of the operator
                startloc start location of the token before mutation
                endloc end location of the token before mutation
                newstartloc start location of the token after mutation
                newendloc end location of the token after mutation
                token targeted token
                newtoken the token to be used to replace targeted token
    */
    void writeToMutationDb(string &op, SourceLocation *startloc, SourceLocation *endloc, 
                            SourceLocation *newstartloc, SourceLocation *newendloc,
                            string &token, string &newtoken) 
    {
        // Open mutattion database file in APPEND mode (write to the end of file)
        ofstream out_mutDb((m_userinput->getMutDbFilename()).data(), ios::app);
        out_mutDb << m_userinput->getInputFilename() << "\t";   // write input file name
        out_mutDb << m_outFilename << m_mutFileNum << "\t";     // write output file name
        out_mutDb << op << "\t";                                // write name of operator
        
        // write information about token BEFORE mutation (range and token)
        out_mutDb << getLineNumber(startloc) << "\t";           
        out_mutDb << getColNumber(startloc) << "\t";
        out_mutDb << getLineNumber(endloc) << "\t";
        out_mutDb << getColNumber(endloc) << "\t";
        out_mutDb << token << "\t";

        // write information about token AFTER mutation (range and token)
        out_mutDb << getLineNumber(newstartloc) << "\t";
        out_mutDb << getColNumber(newstartloc) << "\t";
        out_mutDb << getLineNumber(newendloc) << "\t";
        out_mutDb << getColNumber(newendloc) << "\t";
        out_mutDb << newtoken << endl;

        out_mutDb.close(); 
    }

    /**
        Make a new mutated code file in the specified output directory based on given param.

        @param  op name of the operator
                startloc start location of the token before mutation
                endloc end location of the token before mutation
                token targeted token
                replacingToken the token to be used to replace targeted token 
    */
    void generateMutant(string &op, SourceLocation *startloc, SourceLocation *endloc, 
                        string &token, string &replacingToken) 
    {
        // Create a Rewriter objected needed for edit input file
        Rewriter theRewriter;
        theRewriter.setSourceMgr(m_srcmgr, m_langopt);
        
        // Replace the string started at startloc, length equal to token's length
        // with the replacingToken. 
        theRewriter.ReplaceText(*startloc, token.length(), replacingToken);

        string mutFilename;
        mutFilename = getNextMutFilename();

        // Make and write mutated code to output file.
        const RewriteBuffer *RewriteBuf = theRewriter.getRewriteBufferFor(m_srcmgr.getMainFileID());
        ofstream output(mutFilename.data());
        output << string(RewriteBuf->begin(), RewriteBuf->end());
        output.close();

        SourceLocation newendloc = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                                            getLineNumber(startloc),
                                                            getColNumber(startloc)+count_non_enter(replacingToken));

        // Write information of the new mutated file into mutation database file
        writeToMutationDb(op, startloc, endloc, startloc, &newendloc, token, replacingToken);

        // each output mutated code file needs to have different name.
        ++m_mutFileNum;
    }

    void generateSdlMutant(string &op, SourceLocation *startloc, SourceLocation *endloc, 
                        string &token, string &replacingToken) 
    {
        // Create a Rewriter objected needed for edit input file
        Rewriter theRewriter;
        theRewriter.setSourceMgr(m_srcmgr, m_langopt);

        int length = m_srcmgr.getFileOffset(*endloc) - m_srcmgr.getFileOffset(*startloc);

        theRewriter.ReplaceText(*startloc, length, replacingToken);

        string mutFilename;
        mutFilename = getNextMutFilename();

        // Make and write mutated code to output file.
        const RewriteBuffer *RewriteBuf = theRewriter.getRewriteBufferFor(m_srcmgr.getMainFileID());
        ofstream output(mutFilename.data());
        output << string(RewriteBuf->begin(), RewriteBuf->end());
        output.close();

        SourceLocation newendloc = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                                            getLineNumber(startloc),
                                                            getColNumber(startloc)+count_non_enter(replacingToken));

        // Write information of the new mutated file into mutation database file
        writeToMutationDb(op, startloc, endloc, startloc, &newendloc, token, replacingToken);

        // each output mutated code file needs to have different name.
        ++m_mutFileNum;
    }

    void generateMutant_new(string &op, SourceLocation *startloc, SourceLocation *endloc, 
                        string &token, string &replacingToken) 
    {
        // Create a Rewriter objected needed for edit input file
        Rewriter theRewriter;
        theRewriter.setSourceMgr(m_srcmgr, m_langopt);
        
        int length = m_srcmgr.getFileOffset(*endloc) - m_srcmgr.getFileOffset(*startloc);

        theRewriter.ReplaceText(*startloc, length, replacingToken);

        string mutFilename;
        mutFilename = getNextMutFilename();

        // Make and write mutated code to output file.
        const RewriteBuffer *RewriteBuf = theRewriter.getRewriteBufferFor(m_srcmgr.getMainFileID());
        ofstream output(mutFilename.data());
        output << string(RewriteBuf->begin(), RewriteBuf->end());
        output.close();

        SourceLocation newendloc = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                                            getLineNumber(startloc),
                                                            getColNumber(startloc)+count_non_enter(replacingToken));

        // Write information of the new mutated file into mutation database file
        writeToMutationDb(op, startloc, endloc, startloc, &newendloc, token, replacingToken);

        // each output mutated code file needs to have different name.
        ++m_mutFileNum;
    }

    // Return True if the given range is within the mutation range (mutation is permitted)
    bool targetInMutationRange(SourceLocation *startloc, SourceLocation *endloc)
    {
        // If line is out of bound, then this token is not mutated
        if ((getLineNumber(startloc) < getLineNumber(m_userinput->getStartOfRange())) ||
            (getLineNumber(endloc) > getLineNumber(m_userinput->getEndOfRange())))
        {
            // cout << "false\n";
            return false;
        }

        // If column is out of bound, then this token is not mutated
        if ((getLineNumber(startloc) == getLineNumber(m_userinput->getStartOfRange())) &&
            (getColNumber(startloc) < getColNumber(m_userinput->getStartOfRange())))
        {
            // cout << "false\n";
            return false;
        }

        if ((getLineNumber(endloc) == getLineNumber(m_userinput->getEndOfRange())) &&
            (getColNumber(endloc) > getColNumber(m_userinput->getEndOfRange())))
        {
            return false;
        }

        return true;
    }

    SourceLocation getRealEndLoc(SourceLocation *loc)
    {
        SourceLocation realendloc{};
        if (*(m_srcmgr.getCharacterData(*loc)) == '}' || *(m_srcmgr.getCharacterData(*loc)) == ';' ||
            *(m_srcmgr.getCharacterData(*loc)) == ']' || *(m_srcmgr.getCharacterData(*loc)) == ')')
            realendloc = (*loc).getLocWithOffset(1);
        else
            realendloc = clang::Lexer::findLocationAfterToken(*loc,
                                                            tok::semi, m_srcmgr,
                                                            m_langopt, 
                                                            false);
        return realendloc;
    }

    SourceLocation getEndLocForConst(SourceLocation start)
    {
        int line = getLineNumber(&start);
        int col = getColNumber(&start);

        SourceLocation ret = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(), line, col);

        bool isChar = false;
        if (*(m_srcmgr.getCharacterData(ret)) == '\'')
            isChar = true;

        // a char starts and ends with a single quote
        if (isChar)
        {
            ret = ret.getLocWithOffset(1);
            while (*(m_srcmgr.getCharacterData(ret)) != '\'')
            {
                if (*(m_srcmgr.getCharacterData(ret)) == '\\')
                {
                    ret = ret.getLocWithOffset(1);
                }
                ret = ret.getLocWithOffset(1);
            }
            ret = ret.getLocWithOffset(1);
        }
        else
        {
            // these are the characters that cannot be inside a const.
            // the appearance of them signals the end of the const
            while (*(m_srcmgr.getCharacterData(ret)) != ' ' &&
                    *(m_srcmgr.getCharacterData(ret)) != ';' &&
                    *(m_srcmgr.getCharacterData(ret)) != '+' &&
                    *(m_srcmgr.getCharacterData(ret)) != '-' &&
                    *(m_srcmgr.getCharacterData(ret)) != '*' &&
                    *(m_srcmgr.getCharacterData(ret)) != '/' &&
                    *(m_srcmgr.getCharacterData(ret)) != '%' &&
                    *(m_srcmgr.getCharacterData(ret)) != ',' &&
                    *(m_srcmgr.getCharacterData(ret)) != ')' &&
                    *(m_srcmgr.getCharacterData(ret)) != ']' &&
                    *(m_srcmgr.getCharacterData(ret)) != '}' &&
                    *(m_srcmgr.getCharacterData(ret)) != '>' &&
                    *(m_srcmgr.getCharacterData(ret)) != '<' &&
                    *(m_srcmgr.getCharacterData(ret)) != '=' &&
                    *(m_srcmgr.getCharacterData(ret)) != '!' &&
                    *(m_srcmgr.getCharacterData(ret)) != '|' &&
                    *(m_srcmgr.getCharacterData(ret)) != '?' &&
                    *(m_srcmgr.getCharacterData(ret)) != '&' &&
                    *(m_srcmgr.getCharacterData(ret)) != '~' &&
                    *(m_srcmgr.getCharacterData(ret)) != '`' &&
                    *(m_srcmgr.getCharacterData(ret)) != ':' &&
                    *(m_srcmgr.getCharacterData(ret)) != '\n')
                ret = ret.getLocWithOffset(1);
        }

        return ret;
    }

    SourceLocation getEndLocForUnaryOp(UnaryOperator *uo)
    {
        SourceLocation ret = uo->getLocEnd();

        if (uo->getOpcode() == UO_PostInc || uo->getOpcode() == UO_PostDec)
            // the post increment/decrement, getLocEnd returns the location right before ++/--
            ret = ret.getLocWithOffset(2);
        else
            if (uo->getOpcode() == UO_PreInc || uo->getOpcode() == UO_PreDec ||
                uo->getOpcode() == UO_AddrOf || uo->getOpcode() == UO_Deref ||
                uo->getOpcode() == UO_Plus || uo->getOpcode() == UO_Minus ||
                uo->getOpcode() == UO_Not || uo->getOpcode() == UO_LNot)
            {
                // for these cases, getLocEnd returns the location right after ++/--
                Expr *subExpr = uo->getSubExpr()->IgnoreImpCasts();
                ret = getEndLocOfExpr(subExpr);
            }
            else
            {
                ;   // just return clang end loc
            }

        return ret;
    }

    SourceLocation getLocBeforeSemiColon(Expr *e)
    {
        SourceLocation ret = e->getLocEnd();

        if (isa<ArraySubscriptExpr>(e))
        {
            ret = e->getLocEnd();
            ret = getRealEndLoc(&ret);
        }
        else
            if (isa<DeclRefExpr>(e) || isa<MemberExpr>(e))
            {
                int length = m_rewriter.ConvertToString(e).length();
                SourceLocation start = e->getLocStart();
                ret = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                                getLineNumber(&start),
                                                getColNumber(&start) + length);
            }
            else if (isa<CharacterLiteral>(e) || isa<FloatingLiteral>(e) || isa<IntegerLiteral>(e))
            {
                ret = getEndLocForConst(e->getLocStart());
            }
            else if (isa<CStyleCastExpr>(e))    // explicit cast
            {
                return getEndLocOfExpr(cast<CStyleCastExpr>(e)->getSubExpr()->IgnoreImpCasts());
            }
            else 
            {
                ret = e->getLocEnd();
                ret = getRealEndLoc(&ret);

                if (ret.isInvalid())
                    ret = e->getLocEnd();

                // getRealEndLoc returns location after semicolon
                SourceLocation prevLoc = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                                                    getLineNumber(&ret),
                                                                    getColNumber(&ret) - 1);
                if (*(m_srcmgr.getCharacterData(prevLoc)) == ';')
                    ret = prevLoc;
            }

        return ret;
    }

    void generateCRCRMutant(DeclRefExpr *dre)
    {
        SourceLocation start = dre->getLocStart();
        SourceLocation end = dre->getLocEnd();
        string varName = m_rewriter.ConvertToString(dre);
        string opName = "CRCR";

        int cap = m_userinput->getLimit();

        if (((dre->getType()).getTypePtr())->isIntegralType(m_compinst->getASTContext()))
        {
            for (auto it: m_holder->CRCR_integral)
            {
                string replacingToken{"(" + it + ")"};
                generateMutant(opName, &start, &end, varName, replacingToken);

                // Apply limit option on number of generated mutants 
                --cap;
                if (cap == 0)
                    break;
            }
            return;
        }

        if (((dre->getType()).getTypePtr())->isFloatingType())
        {
            for (auto it: m_holder->CRCR_floating)
            {
                string replacingToken{"(" + it + ")"};
                generateMutant(opName, &start, &end, varName, replacingToken);
                
                // Apply limit option on number of generated mutants 
                --cap;
                if (cap == 0)
                    break;
            }
            return;
        }
    }

    void generateVGSRMutant(SourceLocation *start, SourceLocation *end, string refName)
    {
        // cannot mutate the variable in switch condition to a floating-type variable
        bool skipFloating = locationIsInRange(*start, *switchConditionRange);

        // cannot mutate a variable in lhs of assignment to a const variable
        bool skipConst = locationIsInRange(*start, *lhsOfAssignment);

        string op{"VGSR"};

        int cap = m_userinput->getLimit();

        for (auto scalar: m_allGlobalScalars)
        {
            if (skipConst && varDeclIsConst(scalar)) 
                continue;   // this is a const variable and we cant mutate to const variable

            if (skipFloating && varDeclIsFloat(scalar))
                continue;

            string replacingToken{getVarDeclName(scalar)};

            if (refName.compare(replacingToken) != 0)
            {
                generateMutant_new(op, start, end, refName, replacingToken);

                // Apply limit option on number of generated mutants 
                --cap;
                if (cap == 0)
                    break;
            }
        }
    }

    void generateVLSRMutant(SourceLocation *start, SourceLocation *end, string refName)
    {
        // cannot mutate the variable in switch condition to a floating-type variable
		bool skipFloating = locationIsInRange(*start, *switchConditionRange);

        // cannot mutate a variable in lhs of assignment to a const variable
        bool skipConst = locationIsInRange(*start, *lhsOfAssignment);

        bool skipRegister = locationIsInRange(*start, *addressOfOpRange);

        string op{"VLSR"};

        int cap = m_userinput->getLimit();

        for (auto scope: m_allLocalScalars)
        {
            for (auto scalar: scope)
            {
                if (skipConst && varDeclIsConst(scalar)) 
                    continue;   // this is a const variable and we cant mutate to const variable

                if (skipFloating && varDeclIsFloat(scalar))
                    continue;

                if (skipRegister && scalar->getStorageClass() == SC_Register)
                {
                    // cout << "cannot mutate to " << getVarDeclName(scalar) << endl;
                    continue;
                }

                string replacingToken{getVarDeclName(scalar)};

                if (refName.compare(replacingToken) != 0)
                {
                    generateMutant_new(op, start, end, refName, replacingToken);

                    // Apply limit option on number of generated mutants 
                    --cap;
                    if (cap == 0)
                        return;
                }
            }
        }
    }

    void generateVGARMutant(SourceLocation *start, SourceLocation *end, string refName, QualType type)
    {
        // cannot mutate the variable in switch condition to a floating-type variable
		bool skipFloating = locationIsInRange(*start, *switchConditionRange);

        // cannot mutate a variable in lhs of assignment to a const variable
        bool skipConst = locationIsInRange(*start, *lhsOfAssignment);

        string op{"VGAR"};

        int cap = m_userinput->getLimit();

        for (auto array: m_allGlobalArrays)
        {
            if (skipConst && varDeclIsConst(array)) 
                continue;   // this is a const variable and we cant mutate to const variable

            if (skipFloating && varDeclIsFloat(array))
                continue;

            string replacingToken{getVarDeclName(array)};

            if (refName.compare(replacingToken) != 0 && sameArrayElementType(type, array->getType()))
            {
                generateMutant_new(op, start, end, refName, replacingToken);

                // Apply limit option on number of generated mutants 
                --cap;
                if (cap == 0)
                    break;
            }
        }
    }

    void generateVLARMutant(SourceLocation *start, SourceLocation *end, string refName, QualType type)
    {
        // cannot mutate the variable in switch condition to a floating-type variable
		bool skipFloating = locationIsInRange(*start, *switchConditionRange);

        // cannot mutate a variable in lhs of assignment to a const variable
		bool skipConst = locationIsInRange(*start, *lhsOfAssignment);

        // cannot mutate a variable inside addressOf op (&) to a register variable
        bool skipRegister = locationIsInRange(*start, *addressOfOpRange);

        string op{"VLAR"};

        int cap = m_userinput->getLimit();

        for (auto scope: m_allLocalArrays)
        {
            for (auto array: scope)
            {
                if (skipConst && varDeclIsConst(array)) 
                    continue;   // this is a const variable and we cant mutate to const variable

                if (skipFloating && varDeclIsFloat(array))
                    continue;

                if (skipRegister && array->getStorageClass() == SC_Register)
                {
                    continue;
                }

                string replacingToken{getVarDeclName(array)};

                if (refName.compare(replacingToken) != 0 && sameArrayElementType(type, array->getType()))
                {
                    generateMutant_new(op, start, end, refName, replacingToken);

                    // Apply limit option on number of generated mutants 
                    --cap;
                    if (cap == 0)
                        return;
                }
            }
        }
    }

    void generateVGTRMutant(SourceLocation *start, SourceLocation *end, string refName, QualType type)
    {
        // cannot mutate the variable in switch condition to a floating-type variable
		bool skipFloating = locationIsInRange(*start, *switchConditionRange);

        // cannot mutate a variable in lhs of assignment to a const variable
		bool skipConst = locationIsInRange(*start, *lhsOfAssignment);

        string structType = getStructureType(type);

        string op{"VGTR"};

        int cap = m_userinput->getLimit();

        for (auto structure: m_allGlobalStructs)
        {
            if (skipConst && varDeclIsConst(structure)) 
                continue;   // this is a const variable and we cant mutate to const variable

            if (skipFloating && varDeclIsFloat(structure))
                continue;

            string replacingToken{getVarDeclName(structure)};

            if (refName.compare(replacingToken) != 0 &&
                structType.compare(getStructureType(structure->getType())) == 0)
            {
                generateMutant_new(op, start, end, refName, replacingToken);

                // Apply limit option on number of generated mutants 
                --cap;
                if (cap == 0)
                    break;
            }
        }
    }

    void generateVLTRMutant(SourceLocation *start, SourceLocation *end, string refName, QualType type)
    {
        // cannot mutate the variable in switch condition to a floating-type variable
		bool skipFloating = locationIsInRange(*start, *switchConditionRange);

        // cannot mutate a variable in lhs of assignment to a const variable
		bool skipConst = locationIsInRange(*start, *lhsOfAssignment);

        // cannot mutate a variable inside addressOf op (&) to a register variable
        bool skipRegister = locationIsInRange(*start, *addressOfOpRange);

        string structType = getStructureType(type);

        string op{"VLTR"};

        int cap = m_userinput->getLimit();

        for (auto scope: m_allLocalStructs)
        {
            for (auto structure: scope)
            {
                if (skipConst && varDeclIsConst(structure)) 
                    continue;   // this is a const variable and we cant mutate to const variable

                if (skipFloating && varDeclIsFloat(structure))
                    continue;

                if (skipRegister && structure->getStorageClass() == SC_Register)
                {
                    cout << "cannot mutate to " << getVarDeclName(structure) << endl;
                    continue;
                }

                string replacingToken{getVarDeclName(structure)};

                if (refName.compare(replacingToken) != 0 &&
                    structType.compare(getStructureType(structure->getType())) == 0)
                {
                    generateMutant_new(op, start, end, refName, replacingToken);

                    // Apply limit option on number of generated mutants 
                    --cap;
                    if (cap == 0)
                        return;
                }
            }
        }
    }

    void generateVGPRMutant(SourceLocation *start, SourceLocation *end, string refName, QualType type)
    {
        // cannot mutate the variable in switch condition to a floating-type variable
		bool skipFloating = locationIsInRange(*start, *switchConditionRange);

        // cannot mutate a variable in lhs of assignment to a const variable
		bool skipConst = locationIsInRange(*start, *lhsOfAssignment);

        string pointeeType = getPointerType(type);

        string op{"VGPR"};

        int cap = m_userinput->getLimit();

        for (auto pointer: m_allGlobalPointers)
        {
            if (skipConst && varDeclIsConst(pointer)) 
                continue;   // this is a const variable and we cant mutate to const variable

            if (skipFloating && varDeclIsFloat(pointer))
                continue;

            string replacingToken{getVarDeclName(pointer)};

            if (refName.compare(replacingToken) != 0 &&
                pointeeType.compare(getPointerType(pointer->getType())) == 0)
            {
                generateMutant_new(op, start, end, refName, replacingToken);

                // Apply limit option on number of generated mutants 
                --cap;
                if (cap == 0)
                    break;
            }
        }
    }

    void generateVLPRMutant(SourceLocation *start, SourceLocation *end, string refName, QualType type)
    {
        // cannot mutate the variable in switch condition to a floating-type variable
		bool skipFloating = locationIsInRange(*start, *switchConditionRange);

        // cannot mutate a variable in lhs of assignment to a const variable
		bool skipConst = locationIsInRange(*start, *lhsOfAssignment);

        // cannot mutate a variable inside addressOf op (&) to a register variable
        bool skipRegister = locationIsInRange(*start, *addressOfOpRange);

        string pointeeType = getPointerType(type);

        string op{"VLPR"};

        int cap = m_userinput->getLimit();

        for (auto scope: m_allLocalPointers)
        {
            for (auto pointer: scope)
            {
                if (skipConst && varDeclIsConst(pointer)) 
                    continue;   // this is a const variable and we cant mutate to const variable

                if (skipFloating && varDeclIsFloat(pointer))
                    continue;

                if (skipRegister && pointer->getStorageClass() == SC_Register)
                {
                    // cout << "cannot mutate to " << getVarDeclName(pointer) << endl;
                    continue;
                }

                string replacingToken{getVarDeclName(pointer)};

                if (refName.compare(replacingToken) != 0 &&
                    pointeeType.compare(getPointerType(pointer->getType())) == 0)
                {
                    generateMutant_new(op, start, end, refName, replacingToken);

                    // Apply limit option on number of generated mutants 
                    --cap;
                    if (cap == 0)
                        return;
                }
            }
        }
    }

    void generateVSCRMutant(MemberExpr *me, SourceLocation end)
    {
        auto baseQualType = me->getBase()->getType().getCanonicalType();
        if (me->isArrow())  // structPointer->structMember
            baseQualType = cast<PointerType>(baseQualType.getTypePtr())->getPointeeType().getCanonicalType();

        // type of the member expression
        auto *type = me->getType().getCanonicalType().getTypePtr();

        // name of the specific member
        string refName{me->getMemberDecl()->getNameAsString()};

        SourceLocation start = me->getMemberLoc();
        string op{"VSCR"};

        int cap = m_userinput->getLimit();

        if (auto rt = dyn_cast<RecordType>(baseQualType.getTypePtr()))
        {
            RecordDecl *rd = rt->getDecl()->getDefinition();

            for (auto field = rd->field_begin(); field != rd->field_end(); ++field)
            {
                string replacingToken{field->getNameAsString()};

                if (refName.compare(replacingToken) == 0)
                    continue;

                if (field->isAnonymousStructOrUnion())
                    continue;

                auto fieldType = field->getType().getCanonicalType().getTypePtr();

                if (fieldType->isScalarType() && !fieldType->isPointerType() &&
                    exprIsScalar(cast<Expr>(me)) && !exprIsPointer(cast<Expr>(me)))
                {
                    // this field & parameter me are both scalar type. exact type does not matter.
                    generateMutant_new(op, &start, &end, refName, replacingToken);

                    // Apply limit option on number of generated mutants 
                    --cap;
                    if (cap == 0)
                        break;
                }
                else if (fieldType->isPointerType() && exprIsPointer(cast<Expr>(me)))
                {
                    // this field & parameter me are both pointer type. exact type does matter.
                    string exprPointeeType = getPointerType(me->getType().getCanonicalType());
                    string fieldPointeeType = getPointerType(field->getType().getCanonicalType());

                    if (exprPointeeType.compare(fieldPointeeType) == 0)
                    {
                        generateMutant_new(op, &start, &end, refName, replacingToken);

                        // Apply limit option on number of generated mutants 
                        --cap;
                        if (cap == 0)
                            break;
                    }
                }
                else if (fieldType->isArrayType() && exprIsArray(cast<Expr>(me)))
                {
                    // this field & parameter me are both array type. exact type does matter.
                    string exprMemberType = cast<ArrayType>(me->getType().getCanonicalType().getTypePtr())->getElementType().getCanonicalType().getAsString();
                    string fieldMemberType = cast<ArrayType>(field->getType().getCanonicalType().getTypePtr())->getElementType().getCanonicalType().getAsString();

                    if (exprMemberType.compare(fieldMemberType) == 0)
                    {
                        generateMutant_new(op, &start, &end, refName, replacingToken);

                        // Apply limit option on number of generated mutants 
                        --cap;
                        if (cap == 0)
                            break;
                    }
                }
                else if (fieldType->isStructureType() && exprIsStruct(cast<Expr>(me)))
                {
                    // this field & parameter me are both structure type. exact type does matter.
                    string exprStructureType = me->getType().getCanonicalType().getAsString();
                    string fieldStructureType = field->getType().getCanonicalType().getAsString();

                    if (exprStructureType.compare(fieldStructureType) == 0)
                    {
                        generateMutant_new(op, &start, &end, refName, replacingToken);

                        // Apply limit option on number of generated mutants 
                        --cap;
                        if (cap == 0)
                            break;
                    }
                }
            }            
        } 
        else
        {
            cout << "generateVSCRMutant: cannot convert to record type at "; printLocation(start);
            exit(1);
        }    
    }

    void generateOPPOMutant(UnaryOperator *uo)
    {
        if (m_holder->doOPPO)
        {
            SourceLocation start = uo->getLocStart();
            SourceLocation end = getEndLocForUnaryOp(uo);

            string op{"OPPO"};
            string token = m_rewriter.ConvertToString(uo);

            if (uo->getOpcode() == UO_PostInc)  // x++
            {   
                // generate ++x
                uo->setOpcode(UO_PreInc);
                string replacingToken = m_rewriter.ConvertToString(uo);

                generateMutant_new(op, &start, &end, token, replacingToken);

                // generate x--
                uo->setOpcode(UO_PostDec);
                replacingToken = m_rewriter.ConvertToString(uo);

                if (m_userinput->getLimit() > 1)
                    generateMutant_new(op, &start, &end, token, replacingToken);

                // reset the code structure
                uo->setOpcode(UO_PostInc);
            }
            else    // ++x
            {
                // generate x++
                uo->setOpcode(UO_PostInc);
                string replacingToken = m_rewriter.ConvertToString(uo);
                generateMutant_new(op, &start, &end, token, replacingToken);

                // generate --x
                uo->setOpcode(UO_PreDec);
                replacingToken = m_rewriter.ConvertToString(uo);
                if (m_userinput->getLimit() > 1)
                    generateMutant_new(op, &start, &end, token, replacingToken);

                // reset the code structure
                uo->setOpcode(UO_PreInc);
            }
        }
    }

    void generateOMMOMutant(UnaryOperator *uo)
    {        
        if (m_holder->doOMMO)
        {
            SourceLocation start = uo->getLocStart();
            SourceLocation end = getEndLocForUnaryOp(uo);

            string op{"OMMO"};
            string token = m_rewriter.ConvertToString(uo);

            if (uo->getOpcode() == UO_PostDec)  // x--
            {   
                // generate --x
                uo->setOpcode(UO_PreDec);
                string replacingToken = m_rewriter.ConvertToString(uo);

                generateMutant_new(op, &start, &end, token, replacingToken);

                // generate x++
                uo->setOpcode(UO_PostInc);
                replacingToken = m_rewriter.ConvertToString(uo);
                if (m_userinput->getLimit() > 1)
                    generateMutant_new(op, &start, &end, token, replacingToken);

                // reset the code structure
                uo->setOpcode(UO_PostDec);
            }
            else    // --x
            {
                // generate x--
                uo->setOpcode(UO_PostDec);
                string replacingToken = m_rewriter.ConvertToString(uo);
                generateMutant_new(op, &start, &end, token, replacingToken);

                // generate --x
                uo->setOpcode(UO_PreDec);
                replacingToken = m_rewriter.ConvertToString(uo);
                if (m_userinput->getLimit() > 1)
                    generateMutant_new(op, &start, &end, token, replacingToken);

                // reset the code structure
                uo->setOpcode(UO_PreDec);
            }
        }
    }

    void generateMutantByNegation(Expr *e, string op)
    {
        SourceLocation start = e->getLocStart();
        string token{m_rewriter.ConvertToString(e)};
        SourceLocation end = getEndLocOfExpr(e); 

        string replacingToken = "!(" + token + ")";
        if (op.compare("OBNG") == 0)
            replacingToken = "~(" + token + ")";

        generateMutant_new(op, &start, &end, token, replacingToken);
    }

    SourceLocation getEndLocOfExpr(Expr *e)
    {
        if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
            return getEndLocForUnaryOp(uo);

        if (BinaryOperator *bo = dyn_cast<BinaryOperator>(e))
            return getEndLocOfExpr(bo->getRHS()->IgnoreImpCasts());

        return getLocBeforeSemiColon(e);
    }

    // Called to check if lhs of an additive expression is a pointer
    bool leftIsPointer(Expr *e)
    {
        bool ret = (((e->IgnoreImpCasts())->getType()).getTypePtr())->isPointerType();
        if (isa<ParenExpr>(e))
            return ret;
        else
        {
            if (BinaryOperator *b = dyn_cast<BinaryOperator>(e))
            {
                string op = b->getOpcodeStr();
                // if lhs is a multiplicative expression it cannot be a pointer
                if (op.compare("*") == 0 || op.compare("/") == 0 || op.compare("%") == 0)
                    return ret;
                else
                    return leftIsPointer(b->getRHS());
            }
            else
                return ret;
        }
    }

    BinaryOperator::Opcode translateToOpcode(string op)
    {
        if (op.compare("+") == 0)
            return BO_Add;
        if (op.compare("-") == 0)
            return BO_Sub;
        if (op.compare("*") == 0)
            return BO_Mul;
        if (op.compare("/") == 0)
            return BO_Div;
        if (op.compare("%") == 0)
            return BO_Rem;
        if (op.compare("<<") == 0)
            return BO_Shl;
        if (op.compare(">>") == 0)
            return BO_Shr;
        if (op.compare("|") == 0)
            return BO_Or;
        if (op.compare("&") == 0)
            return BO_And;
        if (op.compare("^") == 0)
            return BO_Xor;
        if (op.compare("<") == 0)
            return BO_LT;
        if (op.compare(">") == 0)
            return BO_GT;
        if (op.compare("<=") == 0)
            return BO_LE;
        if (op.compare(">=") == 0)
            return BO_GE;
        if (op.compare("==") == 0)
            return BO_EQ;
        if (op.compare("!=") == 0)
            return BO_NE;
        if (op.compare("&&") == 0)
            return BO_LAnd;
        if (op.compare("||") == 0)
            return BO_LOr;
        cout << "cannot translate " << op << " to opcode\n";
        exit(1);
    }

    bool mutationCauseZero(BinaryOperator *b, string op)
    {
        BinaryOperator::Opcode original = b->getOpcode();
        b->setOpcode(translateToOpcode(op));
        llvm::APSInt val;

        if (b->EvaluateAsInt(val, m_compinst->getASTContext()))
        {
            // check if the value is zero.
            // reset the opcode and return true/false depending on the check
            b->setOpcode(original);
            string zero = "0";
            if ((val.toString(10)).compare("0") == 0)
                return true;
            return false;
        }
        else
        {
            // cannot evaluate as integer
            // reset the opcode and return false
            b->setOpcode(original);
            return false;
        }
    }

    bool mutationCauseDivideByZero(Expr *rhs)
    {
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
        {
            string op = b->getOpcodeStr();
            return mutationCauseZero(b, op);
        }
        else    // not binary operator
        {
            llvm::APSInt val;
            if (rhs->EvaluateAsInt(val, m_compinst->getASTContext()))
            {
                // check if the value is zero.
                // reset the opcode and return true/false depending on the check
                string zero = "0";
                if ((val.toString(10)).compare("0") == 0)
                    return true;
                return false;
            }
            else
            {
                // cannot evaluate as integer
                return false;
            }
        }
    }

    void printLocation(SourceLocation loc)
    {
        cout << getLineNumber(&loc) << "\t" << getColNumber(&loc) << endl;
    }

    void printRange(SourceRange range)
    {
        cout << "this range is from ";
        printLocation(range.getBegin());
        cout << "                to ";
        printLocation(range.getEnd());
    }

    void handleStmtExpr(StmtExpr *se)
    {
        CompoundStmt::body_iterator stmt = (se->getSubStmt())->body_begin();
        CompoundStmt::body_iterator nextStmt = stmt;
        ++nextStmt;

        // retrieve the last statement inside Statement Expression
        while (nextStmt != (se->getSubStmt())->body_end())
        {
            ++stmt;
            ++nextStmt;
        }

        SourceLocation start = (*stmt)->getLocStart();
        SourceLocation end = (*stmt)->getLocEnd();

        if (targetInMutationRange(&start, &end))
        {
            isInsideStmtExpr = true;
        }
    }

    void handleArraySubscriptExpr(ArraySubscriptExpr *ase)
    {
        SourceLocation start = ase->getLocStart();
        SourceLocation end = ase->getLocEnd();
        end = getRealEndLoc(&end);
        string refName{m_rewriter.ConvertToString(ase)};

        if (arraySubscriptRange != nullptr)
            delete arraySubscriptRange;

        arraySubscriptRange = new SourceRange(start, end);

        //=================================================
        //=== GENERATING Vsrr, Varr, Vtrr, Vprr MUTANTS ===
        //=================================================
        if (m_holder->doVGSR || m_holder->doVLSR || m_holder->doVGAR || m_holder->doVLAR ||
            m_holder->doVGTR || m_holder->doVLTR || m_holder->doVGPR || m_holder->doVLPR)
        {
            if (!isInsideArraySize && !isInsideEnumDecl && targetInMutationRange(&start, &end))
            {
                if (exprIsScalar(cast<Expr>(ase)) && !exprIsPointer(cast<Expr>(ase)))
                {
                    if (m_holder->doVGSR)
                        generateVGSRMutant(&start, &end, refName);

                    if (m_holder->doVLSR)
                    {
                        generateVLSRMutant(&start, &end, refName);
                    }
                }
                else if (exprIsArray(cast<Expr>(ase)))
                {
                    if (m_holder->doVGAR)
                        generateVGARMutant(&start, &end, refName, ase->getType());

                    if (m_holder->doVLAR)
                        generateVLARMutant(&start, &end, refName, ase->getType());
                }
                else if (exprIsStruct(cast<Expr>(ase)))
                {
                    if (m_holder->doVGTR)
                        generateVGTRMutant(&start, &end, refName, ase->getType());

                    if (m_holder->doVLTR)
                        generateVLTRMutant(&start, &end, refName, ase->getType());
                }
                else if (exprIsPointer(cast<Expr>(ase)))
                {
                    if (m_holder->doVGPR)
                        generateVGPRMutant(&start, &end, refName, ase->getType());

                    if (m_holder->doVLPR)
                        generateVLPRMutant(&start, &end, refName, ase->getType());
                }
            }
        }
    }

    void handleDeclRefExpr(DeclRefExpr *dre)
    {
        SourceLocation start = dre->getLocStart();
        string refName{dre->getDecl()->getNameAsString()};
        SourceLocation end = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                        getLineNumber(&start),
                                        getColNumber(&start) + refName.length());

        // Enum value used outside of enum declaration scope are considered as Declaration Reference Expression
        // Do not mutate those.
        if (isa<EnumConstantDecl>(dre->getDecl()))
            return;

        //===============================
        //=== GENERATING Ccsr MUTANTS ===
        //===============================
        if ((m_holder->doCGSR || m_holder->doCLSR) && exprIsScalar(cast<Expr>(dre)) && !exprIsPointer(cast<Expr>(dre)))
        {
            // no mutation applied inside array declaration, enum declaration, outside mutation range,
            // left hand of assignment, inside address-of operator and left hand of unary increment operator.
            if (!isInsideArraySize && !isInsideEnumDecl && targetInMutationRange(&start, &end) &&
                !locationIsInRange(start, *lhsOfAssignment) && !locationIsInRange(start, *unaryIncrementRange) &&
                !locationIsInRange(start, *unaryDecrementRange) && !locationIsInRange(start, *addressOfOpRange))
            {
                // cannot mutate the variable in switch condition or array subscript to a floating-type variable
                bool skipFloating = locationIsInRange(start, *arraySubscriptRange) ||
                                    locationIsInRange(start, *switchConditionRange);

                if (m_holder->doCGSR)
                {
                    string op{"CGSR"};
                    int cap = m_userinput->getLimit();

                    for (auto replacingToken: *m_allGlobalConsts)
                    {
                        if (skipFloating && replacingToken.second)
                            continue;

                        generateMutant_new(op, &start, &end, refName, replacingToken.first);

                        // Apply limit option on number of generated mutants 
                        --cap;
                        if (cap == 0)
                            break;
                    }
                }

                // CLSR can only be applied to references inside a function, not a global reference
                if (m_holder->doCLSR && locationIsInRange(start, *currentFunctionDeclRange))
                {
                    string op{"CLSR"};

                    int cap = m_userinput->getLimit();

                    // generate mutants with constants inside the current function
                    for (auto constant: *m_allLocalConsts)
                    {
                        // all the consts after this are outside the currently parsed function
                        if (!locationIsInRange(constant.second.first, *currentFunctionDeclRange))
                            break;

                        if (skipFloating && constant.second.second)
                            continue;

                        generateMutant_new(op, &start, &end, refName, constant.first);

                        // Apply limit option on number of generated mutants 
                        --cap;
                        if (cap == 0)
                            break;
                    }
                }
            }
        }

        //===============================
        //=== GENERATING CRCR MUTANTS ===
        //===============================
        if ((m_holder->CRCR_floating).size() != 0)
        {
            if (!isInsideArraySize && !isInsideEnumDecl && targetInMutationRange(&start, &end))
            {
                if (!locationIsInRange(start, *lhsOfAssignment) && !locationIsInRange(start, *unaryIncrementRange) 
                    && !locationIsInRange(start, *unaryDecrementRange) && !locationIsInRange(start, *addressOfOpRange))
                    generateCRCRMutant(dre);
            }
        }

        //=================================================
        //=== GENERATING Vsrr, Varr, Vtrr, Vprr MUTANTS ===
        //=================================================
        if (m_holder->doVGSR || m_holder->doVLSR || m_holder->doVGAR || m_holder->doVLAR ||
            m_holder->doVGTR || m_holder->doVLTR || m_holder->doVGPR || m_holder->doVLPR)
        {
            if (!isInsideArraySize && !isInsideEnumDecl && targetInMutationRange(&start, &end))
            {
                //===============================
                //=== GENERATING Vsrr MUTANTS ===
                //===============================
                if (exprIsScalar(cast<Expr>(dre)) && !exprIsPointer(cast<Expr>(dre)))
                {
                    if (m_holder->doVGSR)
                        generateVGSRMutant(&start, &end, refName);

                    if (m_holder->doVLSR)
                    {
                        generateVLSRMutant(&start, &end, refName);
                    }
                }
                //===============================
                //=== GENERATING Varr MUTANTS ===
                //===============================
                else if (exprIsArray(cast<Expr>(dre)))
                {
                    if (m_holder->doVGAR)
                        generateVGARMutant(&start, &end, refName, dre->getType());

                    if (m_holder->doVLAR)
                        generateVLARMutant(&start, &end, refName, dre->getType());
                }
                //===============================
                //=== GENERATING Vtrr MUTANTS ===
                //===============================
                else if (exprIsStruct(cast<Expr>(dre)))
                {
                    // cout << "found a structure: " << m_rewriter.ConvertToString(dre) << endl;

                    if (m_holder->doVGTR)
                        generateVGTRMutant(&start, &end, refName, dre->getType());

                    if (m_holder->doVLTR)
                        generateVLTRMutant(&start, &end, refName, dre->getType());
                }
                //===============================
                //=== GENERATING Vprr MUTANTS ===
                //===============================
                else if (exprIsPointer(cast<Expr>(dre)))
                {
                    if (m_holder->doVGPR)
                        generateVGPRMutant(&start, &end, refName, dre->getType());

                    if (m_holder->doVLPR)
                        generateVLPRMutant(&start, &end, refName, dre->getType());
                }
            }
        }
    }

    void handleAbstractConditionalOperator(AbstractConditionalOperator *aco)
    {
        // handle (condition) ? (then) : (else)

        //===============================
        //=== GENERATING OCNG MUTANTS ===
        //===============================
        if (m_holder->doOCNG && aco->getCond() != nullptr)
        {
            // Negate the condition of if statement
            Expr *cond = aco->getCond()->IgnoreImpCasts();
            SourceLocation start = cond->getLocStart();
            SourceLocation end = cond->getLocEnd();
            string op{"OCNG"};

            if (targetInMutationRange(&start, &end))
                generateMutantByNegation(cond, op);
        }
    }

    void handleUnaryOperator(UnaryOperator *uo)
    {
        SourceLocation start = uo->getLocStart();
        SourceLocation end = uo->getLocEnd();

        // Retrieve the range of UnaryOperator getting address of single expression
        // to prevent getting-address-of-a-register error.
        if (uo->getOpcode() == UO_AddrOf && 
            (isa<DeclRefExpr>(uo->getSubExpr()) || 
                isa<MemberExpr>(uo->getSubExpr()) ||
                isa<ArraySubscriptExpr>(uo->getSubExpr())))
        {
            addressOfOpRange = new SourceRange(uo->getLocStart(), uo->getLocEnd());
        }
        else if (uo->getOpcode() == UO_PostInc || uo->getOpcode() == UO_PreInc)
        {
            unaryIncrementRange = new SourceRange(uo->getLocStart(), uo->getLocEnd());

            //===============================
            //=== GENERATING OPPO MUTANTS ===
            //===============================
            if (targetInMutationRange(&start, &end))
                generateOPPOMutant(uo);
        }   
        else if (uo->getOpcode() == UO_PostDec || uo->getOpcode() == UO_PreDec)
        {
            unaryDecrementRange = new SourceRange(uo->getLocStart(), uo->getLocEnd());

            //===============================
            //=== GENERATING OMMO MUTANTS ===
            //===============================
            if (targetInMutationRange(&start, &end))
                generateOMMOMutant(uo);
        }
    }

    void handleScalarConstant(Expr *e)
    {
        SourceLocation start = e->getLocStart();

        string token{m_rewriter.ConvertToString(e)};
        SourceLocation end = getEndLocForConst(start);

        //===============================
        //=== GENERATING Cccr MUTANTS ===
        //===============================
        if (m_holder->doCGCR || m_holder->doCLCR)
        {
            // cannot mutate the variable in switch condition, case value, array subscript to a floating-type variable
            bool skipFloating = locationIsInRange(start, *arraySubscriptRange) ||
                                locationIsInRange(start, *switchConditionRange) ||
                                locationIsInRange(start, *switchCaseRange);

            if (!isInsideArraySize && !isInsideEnumDecl &&
                targetInMutationRange(&start, &end) &&
                !locationIsInRange(start, *fieldDeclRange))
            {
                if (m_holder->doCGCR)
                {
                    string op{"CGCR"};

                    int cap = m_userinput->getLimit();

                    for (auto replacingToken: *m_allGlobalConsts)
                    {
                        if (skipFloating && replacingToken.second)
                            continue;

                        if (token.compare(replacingToken.first) == 0)
                            continue;

                        bool duplicatedCase = false;
                        if (locationIsInRange(start, *switchCaseRange))
                        {   
                            string temp = replacingToken.first;
                            if (temp.front() == '\'' && temp.back() == '\'')
                                temp = convertCharStringToIntString(temp);

                            for (auto e: m_switchCaseTracker.back().second)
                                if (temp.compare(e) == 0)
                                {
                                    duplicatedCase = true;
                                    break;
                                }
                        }

                        if (!duplicatedCase)
                        {
                            generateMutant_new(op, &start, &end, token, replacingToken.first);

                            // Apply limit option on number of generated mutants 
                            --cap;
                            if (cap == 0)
                                break;                        
                        }
                    }
                }

                // CLCR can only be applied to constants inside a function, not a global constant
                if (m_holder->doCLCR && locationIsInRange(start, *currentFunctionDeclRange))
                {
                    string op{"CLCR"};

                    int cap = m_userinput->getLimit();

                    // generate mutants with constants inside the current function
                    for (auto constant: *m_allLocalConsts)
                    {
                        if (locationBeforeRange(constant.second.first, *currentFunctionDeclRange))
                        {
                            // cout << "before range. NEXT.\n";
                            continue;
                        }

                        // all the consts after this are outside the currently parsed function
                        if (!locationIsInRange(constant.second.first, *currentFunctionDeclRange))
                        {
                            // cout << "out of the function\n";
                            break;
                        }

                        // Prevent mutating anything inside an array subscript to floating type
                        if (skipFloating && constant.second.second)
                        {
                            // cout << "skip floating\n";
                            continue;
                        }

                        if (token.compare(constant.first) == 0)
                        {
                            // cout << "exact same\n";
                            continue;
                        }

                        bool duplicatedCase = false;
                        if (locationIsInRange(start, *switchCaseRange))
                        {
                            // for (auto e: m_switchCaseTracker)
                            //     print_vec(e.second);

                            string temp = constant.first;
                            if (temp.front() == '\'' && temp.back() == '\'')
                                temp = convertCharStringToIntString(temp);

                            for (auto e: m_switchCaseTracker.back().second)
                                if (temp.compare(e) == 0)
                                {
                                    duplicatedCase = true;
                                    break;
                                }
                        }

                        if (!duplicatedCase)
                        {
                            generateMutant_new(op, &start, &end, token, constant.first);

                            // Apply limit option on number of generated mutants 
                            --cap;
                            if (cap == 0)
                                break;
                        }
                    }
                }
            }
        }
    }

    int getPrecedence(BinaryOperator::Opcode op)
    {
        switch (op)
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

    // CURRENTLY UNUSED
    /*BinaryOperator* getRightmostBinaryOperator(BinaryOperator *b)
    {
        Expr *rhs = b->getRHS()->IgnoreImpCasts();
        if (BinaryOperator *e = dyn_cast<BinaryOperator>(rhs))
            return getRightmostBinaryOperator(e);
        else
            return b;
    }

    // CURRENTLY UNUSED
    // Return True if this operator has higher precedence than the previous operator on its left.
    // This means mutation can cause a change in calculation order => potential uncompilable mutants.
    bool higherPrecedenceThanLeftOperator(Expr *lhs, string op)
    {
        if (!isa<BinaryOperator>(lhs))  // there is no operator on the left
            return false;

        int opPrecedence(0);
        if (op.compare(">>") == 0)
            opPrecedence = getPrecedence(BO_Shr);
        else if (op.compare("<<") == 0)
            opPrecedence = getPrecedence(BO_Shl);
        // cout << "op precedence is: " << opPrecedence << endl;

        BinaryOperator *e{nullptr};
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
            e = getRightmostBinaryOperator(b);
        else
        {
            cout << "Error: higherPrecedenceThanLeftOperator: cannot convert to BinaryOperator\n";
            cout << m_rewriter.ConvertToString(lhs) << endl;
            exit(1);
        }

        cout << "left op precedence: " << getPrecedence(e->getOpcode()) << endl;

        return opPrecedence > getPrecedence(e->getOpcode());
    }

    // CURRENTLY UNUSED
    BinaryOperator* getLeftmostBinaryOperator(BinaryOperator *b)
    {
        Expr *lhs = b->getLHS()->IgnoreImpCasts();
        if (BinaryOperator *e = dyn_cast<BinaryOperator>(lhs))
            return getLeftmostBinaryOperator(e);
        else
            return b;
    }

    // CURRENTLY UNUSED
    // Return True if this operator has higher precedence than the next operator on its right.
    // This means mutation can cause a change in calculation order => potential uncompilable mutants.
    bool higherPrecedenceThanRightOperator(Expr *rhs, string op)
    {

        if (!isa<BinaryOperator>(rhs))
            return false;

        int opPrecedence(0);
        if (op.compare(">>") == 0)
            opPrecedence = getPrecedence(BO_Shr);
        else if (op.compare("<<") == 0)
            opPrecedence = getPrecedence(BO_Shl);

        BinaryOperator *e{nullptr};
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
            e = getLeftmostBinaryOperator(b);
        else
        {
            cout << "Error: higherPrecedenceThanRightOperator: cannot convert to BinaryOperator\n";
            cout << m_rewriter.ConvertToString(rhs) << endl;
            exit(1);
        }

        return opPrecedence > getPrecedence(e->getOpcode());
    }

    // CURRENTLY UNUSED
    bool leftLongestArithmeticExprYieldPtr(Expr *lhs, int opPrecedence)
    {
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
        {
            if (opPrecedence > getPrecedence(b->getOpcode()))
                return leftLongestArithmeticExprYieldPtr(b->getRHS()->IgnoreImpCasts(), opPrecedence);
            else
                return (((b->getRHS()->IgnoreImpCasts())->getType()).getTypePtr())->isPointerType() ||
                        leftLongestArithmeticExprYieldPtr(b->getLHS()->IgnoreImpCasts(), opPrecedence);
        }
        else
            return ((lhs->getType()).getTypePtr())->isPointerType();
    }*/

    // Mutating an operator can make its lhs target change.
    // Return True if the new lhs of the mutated operator is pointer type.
    // Applicable for operator with precedence lower than +/-
    // Refer to function getPrecedence() for precedence order
    bool leftNewOperandIsPointer(Expr *lhs, BinaryOperator::Opcode op)
    {
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
        {
            BinaryOperator::Opcode opcodeOfB = b->getOpcode();

            if (opcodeOfB >= op)    // precedence of op is higher, analyze rhs of expr lhs
                return leftNewOperandIsPointer(b->getRHS()->IgnoreParenImpCasts(), op);
            else if (opcodeOfB > BO_Sub || opcodeOfB < BO_Add)    // not addittion/subtraction operator
                return false;   // result of these is not pointer
            else    
                // addition/substraction, if one of operands is pointer type then the expr is pointer type
                return leftNewOperandIsPointer(b->getLHS()->IgnoreParenImpCasts(), BO_Assign) ||
                        leftNewOperandIsPointer(b->getRHS()->IgnoreParenImpCasts(), BO_Assign);
        }
        else
            return ((lhs->getType()).getTypePtr())->isPointerType();
    }

    // Mutating an operator can make its rhs target change.
    // Return True if the new rhs of the mutated operator is pointer type.
    // Applicable for operator with precedence lower than +/-
    // Refer to function getPrecedence() for precedence order
    bool rightNewOperandIsPointer(Expr *rhs, BinaryOperator::Opcode op)
    {
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
        {
            BinaryOperator::Opcode opcodeOfB = b->getOpcode();
            if (opcodeOfB >= op)    // precedence of op is higher, analyze lhs of expr rhs
                return rightNewOperandIsPointer(b->getLHS()->IgnoreParenImpCasts(), op);
            else if (opcodeOfB > BO_Sub || opcodeOfB < BO_Add)    // not addittion/subtraction operator
                return false;   // result of these is not pointer
            else
                // addition/substraction, if one of operands is pointer type then the expr is pointer type
                return rightNewOperandIsPointer(b->getLHS()->IgnoreParenImpCasts(), BO_Assign) ||
                        rightNewOperandIsPointer(b->getRHS()->IgnoreParenImpCasts(), BO_Assign);
        }
        else
            return ((rhs->getType()).getTypePtr())->isPointerType();
    }

    // Mutating an operator can make its lhs target change.
    // Return the new lhs of the mutated operator.
    // Applicable for multiplicative operator
    Expr* leftNewOperand_Multi(Expr *lhs)
    {
        // cout << "in here " << m_rewriter.ConvertToString(lhs) << endl;
        // cout << cast<Stmt>(lhs)->getStmtClassName() << endl;
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
        {
            // cout << "is binary\n";
            BinaryOperator::Opcode opcodeOfB = b->getOpcode();
            if (opcodeOfB > BO_Rem)
            {
                // multiplicative operator has the highest precedence.
                // go rhs right away if the operator is not multiplicative.
                return leftNewOperand_Multi(b->getRHS()->IgnoreImpCasts());
            }
            else 
            {
                // cout << "opcode not multiplicative is " << string(b->getOpcodeStr()) << endl;
                return lhs;   // * / % yield non-pointer only.
            }
        }
        else
            return lhs->IgnoreParens()->IgnoreImpCasts();
    }

    // Mutating an operator can make its rhs target change.
    // Return the new rhs of the mutated operator.
    // Applicable for multiplicative operator
    Expr* rightNewOperand_Multi(Expr *rhs)
    {
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
        {
            BinaryOperator::Opcode opcodeOfB = b->getOpcode();
            return rightNewOperand_Multi(b->getLHS()->IgnoreImpCasts());
        }
        else
            return rhs->IgnoreParens()->IgnoreImpCasts();
    }

    // Mutating an operator can make its lhs target change.
    // Return the new lhs of the mutated operator.
    // Applicable for additive operator
    Expr* leftNewOperand_Add(Expr *lhs)
    {
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(lhs))
        {
            BinaryOperator::Opcode opcodeOfB = b->getOpcode();
            if (opcodeOfB < BO_Add) // multiplicative operator
                return lhs; // they have higher precedence, calculated first -> new lhs
            else if (opcodeOfB > BO_Sub)
                // recursive call to rhs of lhs
                return leftNewOperand_Add(b->getRHS()->IgnoreImpCasts());  
            else    // addition or subtraction
                return lhs;
        }
        else
            return lhs->IgnoreParens()->IgnoreImpCasts();
    }

    // Mutating an operator can make its rhs target change.
    // Return the new rhs of the mutated operatore.
    // Applicable for additive operator
    Expr* rightNewOperand_Add(Expr *rhs)
    {
        if (BinaryOperator *b = dyn_cast<BinaryOperator>(rhs))
        {
            BinaryOperator::Opcode opcodeOfB = b->getOpcode();
            if (opcodeOfB < BO_Add) // multiplicative operator
                return rhs; // they have higher precedence, calculated first -> new lhs
            else
                // if it is additive, calculate from left to right -> go left
                // if it is lower precedence than additive, go left because additive is calculated first
                return rightNewOperand_Add(b->getLHS()->IgnoreImpCasts());
        }
        else
            return rhs->IgnoreParens()->IgnoreImpCasts();
    }

    bool isArithmeticAssignmentOperator(BinaryOperator *b)
    {
        return b->getOpcode() >= BO_MulAssign && b->getOpcode() <= BO_SubAssign;
    }

    bool isShiftAssignmentOperator(BinaryOperator *b)
    {
        return b->getOpcode() == BO_ShlAssign && b->getOpcode() == BO_ShrAssign;
    }

    bool isBitwiseAssignmentOperator(BinaryOperator *b)
    {
        return b->getOpcode() >= BO_AndAssign && b->getOpcode() <= BO_OrAssign;
    }

    bool exprIsPointer(Expr *e)
    {
        return ((e->getType()).getTypePtr())->isPointerType();
    }

    bool exprIsFloat(Expr *e)
    {
        return ((e->getType()).getTypePtr())->isFloatingType();
    }

    bool exprIsIntegral(Expr *e)
    {
        return ((e->getType()).getTypePtr())->isIntegralType(m_compinst->getASTContext());
    }

    bool exprIsScalar(Expr *e)
    {
        return ((e->getType()).getTypePtr())->isScalarType(); 
    }

    bool exprIsArray(Expr *e)
    {
        return ((e->getType()).getTypePtr())->isArrayType(); 
    }

    bool exprIsStruct(Expr *e)
    {
        return ((e->getType()).getTypePtr())->isStructureType(); 
    }

    bool VisitEnumDecl(EnumDecl *ed)
    {
        // cout << "visit enum decl\n";
        SourceLocation start = ed->getLocStart();
        SourceLocation end = ed->getLocEnd();

        isInsideEnumDecl = true;
        // cout << "found enum at line " << getLineNumber(&start) << endl;
        return true;
    }

    bool VisitTypedefDecl(TypedefDecl *td)
    {
        typedefRange = new SourceRange(td->getLocStart(), td->getLocEnd());
        return true;
    }

    bool VisitExpr(Expr *e)
    {
        // Do not mutate or consider anything inside a typedef definition
        if (locationIsInRange(e->getLocStart(), *typedefRange))
            return true;

        if (isa<CharacterLiteral>(e) || isa<FloatingLiteral>(e) || isa<IntegerLiteral>(e))
        {
            handleScalarConstant(e);
        }
        else if (AbstractConditionalOperator *aco = dyn_cast<AbstractConditionalOperator>(e))   // condition ? then : else
        {
            handleAbstractConditionalOperator(aco);
        }
        else if (StmtExpr *se = dyn_cast<StmtExpr>(e))  
        {
            handleStmtExpr(se);
        }
        else if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(e))
        {
            handleDeclRefExpr(dre);
        }
        else if (ArraySubscriptExpr *ase = dyn_cast<ArraySubscriptExpr>(e))
        {
            handleArraySubscriptExpr(ase);
        }
        else if (UnaryOperator *uo = dyn_cast<UnaryOperator>(e))
        {
            handleUnaryOperator(uo);
        }
        else if (BinaryOperator *binOp_s = dyn_cast<BinaryOperator>(e)) 
        {
            //===============================
            //=== GENERATING Obom MUTANTS ===
            //===============================

            // Retrieve the location and the operator of the expression
            string op = static_cast<string>(binOp_s->getOpcodeStr());
            SourceLocation startloc = binOp_s->getOperatorLoc();
            SourceLocation endloc = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                                                getLineNumber(&startloc),
                                                                getColNumber(&startloc)+op.length());

            // Certain mutations are not syntactically correct for left side of assignment.
            // Store location for later consideration/prevention of generating uncompilable mutants.
            if (binOp_s->isAssignmentOp())
            {
                if (lhsOfAssignment != nullptr)
                    delete lhsOfAssignment;

                lhsOfAssignment = new SourceRange(binOp_s->getLHS()->getLocStart(), startloc);
            }

            // No mutation needs to be done if the token is not in the mutation range, inside array decl size or enum decl
            if (targetInMutationRange(&startloc, &endloc) && !isInsideArraySize && !isInsideEnumDecl)
            {
                vector<MutantOperator>::iterator startIt;
                vector<MutantOperator>::iterator endIt;

                // Classify the type of operator to traverse the correct MutantOperator vector
                if (isArithmeticOperator(op)) 
                {
                    startIt = (m_holder->bin_arith).begin();
                    endIt = (m_holder->bin_arith).end();

                    // mark an expression that should not be 0 to prevent divide-by-zero error.
                    if (op.compare("/") == 0)
                    {
                        checkForZero = true;
                        Expr *rhs = binOp_s->getRHS()->IgnoreParenImpCasts();
                        
                        if (targetExpr != nullptr)
                            delete targetExpr;
                        
                        targetExpr = new SourceRange(rhs->getLocStart(), rhs->getLocEnd());
                    }
                }
                else if (binOp_s->isLogicalOp())
                {
                    startIt = (m_holder->bin_logical).begin();
                    endIt = (m_holder->bin_logical).end();

                    //===============================
                    //=== GENERATING OLNG MUTANTS ===
                    //===============================
                    if (m_holder->doOLNG)
                    {
                        string olng{"OLNG"};

                        generateMutantByNegation(e, olng);
                        if (m_userinput->getLimit() > 1)
                            generateMutantByNegation(binOp_s->getRHS()->IgnoreImpCasts(), olng);

                        if (m_userinput->getLimit() > 2)
                            generateMutantByNegation(binOp_s->getLHS()->IgnoreImpCasts(), olng);
                    }
                }
                else if (binOp_s->isRelationalOp() || binOp_s->isEqualityOp())
                {
                    startIt = (m_holder->bin_relational).begin();
                    endIt = (m_holder->bin_relational).end();
                }

                else if (binOp_s->isBitwiseOp())
                {
                    startIt = (m_holder->bin_bitwise).begin();
                    endIt = (m_holder->bin_bitwise).end();

                    //===============================
                    //=== GENERATING OBNG MUTANTS ===
                    //===============================
                    if (m_holder->doOBNG)
                    {
                        string obng{"OBNG"};

                        generateMutantByNegation(e, obng);

                        if (m_userinput->getLimit() > 1)
                            generateMutantByNegation(binOp_s->getRHS()->IgnoreImpCasts(), obng);

                        if (m_userinput->getLimit() > 2)
                            generateMutantByNegation(binOp_s->getLHS()->IgnoreImpCasts(), obng);
                    }
                }
                else if (binOp_s->isShiftOp())
                {
                    startIt = (m_holder->bin_shift).begin();
                    endIt = (m_holder->bin_shift).end();
                }
                else if (isArithmeticAssignmentOperator(binOp_s))
                {
                    startIt = (m_holder->bin_arithAssign).begin();
                    endIt = (m_holder->bin_arithAssign).end();
                }
                else if (binOp_s->isShiftAssignOp())
                {
                    startIt = (m_holder->bin_shiftAssign).begin();
                    endIt = (m_holder->bin_shiftAssign).end();
                }
                else if (isBitwiseAssignmentOperator(binOp_s))
                {
                    startIt = (m_holder->bin_bitwiseAssign).begin();
                    endIt = (m_holder->bin_bitwiseAssign).end();
                }
                else if (binOp_s->getOpcode() == BO_Assign)
                {
                    startIt = (m_holder->bin_plainAssign).begin();
                    endIt = (m_holder->bin_plainAssign).end();
                }
                else
                {
                }

                // Check whether left side, right side are pointers or integers
                // to prevent generating uncompliable mutants by OAAN
                bool rightIsPtr = (((binOp_s->getRHS()->IgnoreImpCasts())->getType()).getTypePtr())->isPointerType();
                bool rightIsInt = (((binOp_s->getRHS()->IgnoreImpCasts())->getType()).getTypePtr())->isIntegralType(m_compinst->getASTContext());
                bool leftIsPtr = (((binOp_s->getLHS()->IgnoreParenImpCasts())->getType()).getTypePtr())->isPointerType();
                bool leftIsStruct = (((binOp_s->getLHS()->IgnoreParenImpCasts())->getType()).getTypePtr())->isStructureType();

                for (auto it = startIt; it != endIt; ++it)
                {
                    bool onlyPlusAndMinus = false;
                    bool onlyMinusAssignemt = false;

                    if ((it->getName()).compare("OAAN") == 0)
                    {
                        // only (int + ptr) and (ptr - ptr) are allowed.
                        if (rightIsPtr)
                            continue;

                        // When changing from additive operator to other arithmetic operator,
                        // if the right operand is integral and the closest operand on the left is a pointer,
                        // then only mutate to either + or -.
                        if (rightIsInt && (op.compare("+") == 0 || op.compare("-") == 0))
                        {
                            if (leftIsPointer(binOp_s->getLHS()->IgnoreImpCasts()))
                                onlyPlusAndMinus = true;
                        }
                    }

                    if ((it->getName()).compare("OAAA") == 0)
                    {
                        if (leftIsPtr)
                            onlyPlusAndMinus = true;
                    }

                    if ((it->getName()).compare("OABA") == 0 ||
                        (it->getName()).compare("OASA") == 0)
                        if (leftIsPtr)
                            continue;

                    if ((it->getName()).compare("OESA") == 0 ||
                        (it->getName()).compare("OEBA") == 0)
                        if (leftIsPtr || leftIsStruct)
                            continue;

                    if ((it->getName()).compare("OEAA") == 0)
                        if (leftIsStruct)
                            continue;
                        if (leftIsPtr)
                            onlyMinusAssignemt = true;

                    // Generate mutants only for operators that can mutate this operator
                    if ((it->m_targets).find(op) != (it->m_targets).end())
                    {
                        // Getting the max # of mutants per mutation location.
                        int cap = m_userinput->getLimit();

                        for (auto replacingToken: it->m_replaceOptions)
                        {
                            if ((it->getName()).compare("OLSN") == 0 ||
                                (it->getName()).compare("OLBN") == 0 ||
                                (it->getName()).compare("ORBN") == 0 ||
                                (it->getName()).compare("ORSN") == 0 ||
                                (it->getName()).compare("OASN") == 0 ||
                                (it->getName()).compare("OABN") == 0)
                            {
                                // if new lhs or rhs is pointer, cannot mutate to bitwise or shift operator
                                if (leftNewOperandIsPointer(binOp_s->getLHS(), translateToOpcode(replacingToken)) ||
                                    rightNewOperandIsPointer(binOp_s->getRHS(), translateToOpcode(replacingToken)))
                                    continue;
                            }


                            if ((it->getName()).compare("OLAN") == 0 ||
                                (it->getName()).compare("ORAN") == 0)
                            {
                                Expr *newLeft = leftNewOperand_Add(binOp_s->getLHS()->IgnoreImpCasts());
                                Expr *newRight = rightNewOperand_Add(binOp_s->getRHS()->IgnoreImpCasts());
                                bool newLeftIsPtr = exprIsPointer(newLeft);
                                bool newLeftIsIntegral = exprIsIntegral(newLeft);
                                bool newRightIsPtr = exprIsPointer(newRight);
                                bool newRightIsIntegral = exprIsIntegral(newRight);

                                if ((!newLeftIsPtr && !newLeftIsIntegral) ||
                                    (!newRightIsPtr && !newRightIsIntegral))
                                    // cannot perform addition subtraction on non-pointer, non-int
                                    continue;

                                if (replacingToken.compare("+") == 0)
                                {
                                    // Adding 2 pointers is syntactically incorrect.
                                    if (newLeftIsPtr && newRightIsPtr)
                                        continue;
                                }
                                else if (replacingToken.compare("-") == 0)
                                {
                                    if ((newLeftIsIntegral && newRightIsPtr) ||
                                        (newLeftIsPtr && newRightIsPtr))
                                        continue;
                                }
                                else    // multiplicative operator
                                {
                                    // multiplicative operators can only be applied to numbers
                                    newLeftIsIntegral = exprIsIntegral(leftNewOperand_Multi(binOp_s->getLHS()->IgnoreImpCasts()));
                                    newRightIsIntegral = exprIsIntegral(rightNewOperand_Multi(binOp_s->getRHS()->IgnoreImpCasts()));
                                    if (!newLeftIsIntegral || !newRightIsIntegral)
                                        continue;
                                }
                            }              

                            // Do not replace the token with itself
                            if (replacingToken.compare(op) == 0)
                                continue;

                            if (onlyPlusAndMinus)
                            {
                                if (replacingToken.compare("+") != 0 && replacingToken.compare("-") != 0 &&
                                    replacingToken.compare("+=") != 0 && replacingToken.compare("-=") != 0)
                                    continue;
                            }

                            if (onlyMinusAssignemt && replacingToken.compare("-=") != 0)
                                continue;

                            // Prevent generation of umcompilable mutant because of divide-by-zero
                            if (checkForZero && *targetExpr == SourceRange(binOp_s->getLocStart(), binOp_s->getLocEnd()))
                            {
                                if (mutationCauseZero(binOp_s, replacingToken))
                                {
                                    continue;
                                }
                            }

                            if (replacingToken.compare("/") == 0 &&
                                mutationCauseDivideByZero(binOp_s->getRHS()->IgnoreParenImpCasts()))
                                continue;

                            if (firstBinOpAfterArrayDecl)
                            {
                                arraySize = binOp_s;
                                firstBinOpAfterArrayDecl = false;
                            }

                            if (isInsideArraySize)
                                if (locationIsInRange(startloc, *arrayDeclRange))
                                {
                                    binOp_s->setOpcode(translateToOpcode(replacingToken));
                                    llvm::APSInt val;
                                    if (arraySize->EvaluateAsInt(val, m_compinst->getASTContext()))
                                    {
                                        if (val.isNegative())
                                        {
                                            binOp_s->setOpcode(translateToOpcode(op));
                                            continue;
                                        }
                                    }
                                    binOp_s->setOpcode(translateToOpcode(op));
                                }                              

                            generateMutant(it->getName(), &startloc, &endloc, op, replacingToken);

                            // make sure not to generate more mutants than user wants/specifies
                            --cap;
                            if (cap == 0)
                            {
                                break;
                            }
                        }
                    }
                }

                if (checkForZero && *targetExpr == SourceRange(binOp_s->getLocStart(), binOp_s->getLocEnd()))
                    checkForZero = false;
            }   
        }

        return true;
    }

    bool VisitIfStmt(IfStmt *is)
    {
        //===============================
        //=== GENERATING SSDL MUTANTS ===
        //===============================
        if (m_holder->doSSDL)
        {
            // generate sdl mutant for case of single non-compound statement of THEN part of if statement
            if (Stmt *thenStmt = is->getThen())
            {
                if (!isa<CompoundStmt>(thenStmt) && !isa<NullStmt>(thenStmt))
                {
                    string opName{"SSDL"};
                    string token{m_rewriter.ConvertToString(thenStmt)};
                    SourceLocation startloc = thenStmt->getLocStart();
                    SourceLocation endloc = thenStmt->getLocEnd();

                    // make replacing token
                    string replacingToken{";"};
                    replacingToken.append(getLineNumber(&endloc)-getLineNumber(&startloc), '\n');

                    SourceLocation realendloc = getRealEndLoc(&endloc);

                    if (realendloc.isInvalid())
                    {
                        cout << "error getting end location. no SSDL mutants generated here.\n";
                        cout << "error endloc is: " << getLineNumber(&endloc) << ":" << getColNumber(&endloc) << endl;
                        // exit(1);
                    }
                    else
                    {
                        if (targetInMutationRange(&startloc, &realendloc) /*&& noLabelsInside(SourceRange(startloc, realendloc))*/)
                        generateSdlMutant(opName, &startloc, &realendloc, token, replacingToken);
                    }
                }    
            }
            else 
            {
                // cout << "no then\n";
                ;
            }

            if (Stmt *elseStmt = is->getElse())
            {
                if (!isa<NullStmt>(elseStmt))
                {
                    // generate sdl mutant deleting the whole ELSE part of if statement
                    string opName{"SSDL"};
                    string token{m_rewriter.ConvertToString(elseStmt)};
                    SourceLocation startloc = is->getElseLoc();
                    SourceLocation endloc = elseStmt->getLocEnd();

                    // make replacing token
                    string replacingToken{";"};
                    replacingToken.append(getLineNumber(&endloc)-getLineNumber(&startloc), '\n');

                    SourceLocation realendloc = getRealEndLoc(&endloc);

                    if (realendloc.isInvalid())
                    {
                        cout << "error getting end location. no SSDL mutants generated here.\n";
                        cout << "error endloc is: " << getLineNumber(&endloc) << ":" << getColNumber(&endloc) << endl;
                        // exit(1);
                    }
                    else 
                    {
                        if (targetInMutationRange(&startloc, &realendloc) && noUnremoveableLabelInside(SourceRange(startloc, realendloc)))
                            generateSdlMutant(opName, &startloc, &realendloc, token, replacingToken);
                    }                        
                }
            }
            else
                // cout << "no else\n";
                ;
        }

        //===============================
        //=== GENERATING OCNG MUTANTS ===
        //===============================
        if (m_holder->doOCNG && is->getCond() != nullptr)
        {
            // Negate the condition of if statement
            Expr *cond = is->getCond()->IgnoreImpCasts();
            SourceLocation start = cond->getLocStart();
            SourceLocation end = cond->getLocEnd();
            string op{"OCNG"};

            if (targetInMutationRange(&start, &end))
                generateMutantByNegation(cond, op);
        }

        return true;
    }

    bool VisitWhileStmt(WhileStmt *ws)
    {
        //===============================
        //=== GENERATING OCNG MUTANTS ===
        //===============================
        if (m_holder->doOCNG && ws->getCond() != nullptr)
        {
            // Negate the condition of while statement
            Expr *cond = ws->getCond()->IgnoreImpCasts();
            SourceLocation start = cond->getLocStart();
            SourceLocation end = cond->getLocEnd();
            string op{"OCNG"};

            if (targetInMutationRange(&start, &end))
                generateMutantByNegation(cond, op);
        }

        return true;
    }

    bool VisitDoStmt(DoStmt *ds)
    {
        //===============================
        //=== GENERATING OCNG MUTANTS ===
        //===============================
        if (m_holder->doOCNG && ds->getCond() != nullptr)
        {
            // Negate the condition of if statement
            Expr *cond = ds->getCond()->IgnoreImpCasts();
            SourceLocation start = cond->getLocStart();
            SourceLocation end = cond->getLocEnd();
            string op{"OCNG"};

            if (targetInMutationRange(&start, &end))
                generateMutantByNegation(cond, op);
        }

        return true;
    }

    bool VisitForStmt(ForStmt *fs)
    {
        //===============================
        //=== GENERATING OCNG MUTANTS ===
        //===============================
        if (m_holder->doOCNG && fs->getCond() != nullptr)
        {
            // Negate the condition of if statement
            Expr *cond = fs->getCond()->IgnoreImpCasts();
            SourceLocation start = cond->getLocStart();
            SourceLocation end = cond->getLocEnd();
            string op{"OCNG"};

            if (targetInMutationRange(&start, &end))
                generateMutantByNegation(cond, op);
        }

        return true;
    }

    void deleteStatement(Stmt *s)
    {
        // cannot delete declaration statement
        // deleting null statement causes equivalent mutant
        if (isa<DeclStmt>(s) || isa<NullStmt>(s))
            return; 

        // Do not delete the label, case, default but only the statement right below them.
        if (SwitchCase *sc = dyn_cast<SwitchCase>(s))
        {
            deleteStatement(sc->getSubStmt()); 
            return;
        }
        else 
            if (DefaultStmt *ds = dyn_cast<DefaultStmt>(s))
            {
                deleteStatement(ds->getSubStmt());
                return;
            }
            else
                if (LabelStmt *ls = dyn_cast<LabelStmt>(s))
                {
                    deleteStatement(ls->getSubStmt());
                    return;
                }
                else
                    ;   // execute the rest of the function.


        string opName = "SSDL";
        string token{m_rewriter.ConvertToString(s)};
        SourceLocation startloc = (s)->getLocStart();
        SourceLocation endloc = (s)->getLocEnd();

        // make replacing token
        string replacingToken{";"};
        replacingToken.append(getLineNumber(&endloc)-getLineNumber(&startloc), '\n');

        SourceLocation realendloc = getRealEndLoc(&endloc);

        if (realendloc.isInvalid())
        {
            cout << "error endloc is: " << getLineNumber(&endloc) << ":" << getColNumber(&endloc) << endl;
            cout << "error getting real end location\n";
            // exit(1);
        }
        else
        {
            if (targetInMutationRange(&startloc, &realendloc))
            {
                if (isa<DoStmt>(s) || isa<ForStmt>(s) || isa<WhileStmt>(s) || isa<IfStmt>(s) || isa<SwitchStmt>(s))
                {
                    if (noUnremoveableLabelInside(SourceRange(startloc, realendloc)))
                        generateSdlMutant(opName, &startloc, &realendloc, token, replacingToken);
                }
                else
                    generateSdlMutant(opName, &startloc, &realendloc, token, replacingToken);
            }

            // In case of Do, For, While statement,
            // delete their body statement if that is not compound.
            Stmt *target;
            if (DoStmt *ds = dyn_cast<DoStmt>(s))
                target = ds->getBody();
            else 
                if (ForStmt *fs = dyn_cast<ForStmt>(s))
                    target = fs->getBody();
                else 
                    if (WhileStmt *fs = dyn_cast<WhileStmt>(s))
                        target = fs->getBody();
                    else 
                        return;         

            // no braces surrounding the Body part of this statement
            if (!isa<CompoundStmt>(target))
            {
                deleteStatement(target);
            }
        }            
    }

    bool VisitCompoundStmt(CompoundStmt *c)
    {
        // new scope
        m_currentScope.push_back(SourceRange(c->getLocStart(), c->getLocEnd()));
        m_allLocalScalars.push_back(DeclInScope());
        m_allLocalArrays.push_back(DeclInScope());
        m_allLocalStructs.push_back(DeclInScope());
        m_allLocalPointers.push_back(DeclInScope());

        if (!m_holder->doSSDL)
            return true;

        // Generate SSDL mutants for each statement in the compound that is not
        // declaration, switch case, default case, label or null statement.
        for (CompoundStmt::body_iterator it = c->body_begin(); it != c->body_end(); ++it)
        {
            // Do not apply SSDL to the last statement of statement expression
            if (isInsideStmtExpr)
            {
                CompoundStmt::body_iterator nextStmt = it;
                ++nextStmt;
                if (nextStmt == c->body_end())  // this is the last statement of a statement expression
                {
                    isInsideStmtExpr = false;
                    continue;   // no SDL mutants generated for this statement
                }
            }

            deleteStatement(*it);
        }

        return true;
    }

    bool VisitFieldDecl(FieldDecl *fd)
    {
        SourceLocation start = fd->getLocStart();
        SourceLocation end = fd->getLocEnd();

        if (fieldDeclRange != nullptr)
            delete fieldDeclRange;
        fieldDeclRange = new SourceRange(start, end);

        if (fd->getType().getTypePtr()->isArrayType())
        {
            isInsideArraySize = true;
            firstBinOpAfterArrayDecl = true;

            // if (arrayDeclRange != nullptr)
            //     delete arrayDeclRange;

            arrayDeclRange = new SourceRange(fd->getLocStart(), fd->getLocEnd());
        }
        return true;
    }

    bool VisitVarDecl(VarDecl *vd)
    {
        isInsideEnumDecl = false;

        if (locationIsInRange(vd->getLocStart(), *typedefRange))
            return true;

        if (locationIsInRange(vd->getLocStart(), *functionPrototypeRange))
            return true;

        if (varDeclIsScalar(vd))
        {
            if (vd->isFileVarDecl())    // store global scalar variable
            {
                m_allGlobalScalars.push_back(vd);
            }
            else
            {
                // This is a local variable. m_currentScope vector CANNOT be empty.
                if (m_allLocalScalars.empty())
                {
                    cout << "m_allLocalScalars is empty when meeting a local variable declaration at "; printLocation(vd->getLocStart());
                    exit(1);
                }
                m_allLocalScalars.back().push_back(vd);
            }
        }
        else if (varDeclIsArray(vd))
        {
            if (auto t =  dyn_cast_or_null<ConstantArrayType>(vd->getType().getCanonicalType().getTypePtr())) 
            {  
                isInsideArraySize = true;
                firstBinOpAfterArrayDecl = true;

                arrayDeclRange = new SourceRange(vd->getLocStart(), vd->getLocEnd());
            }

            if (vd->isFileVarDecl())    // global variable
            {
                m_allGlobalArrays.push_back(vd);
            }
            else
            {
                // This is a local variable. m_currentScope vector CANNOT be empty.
                if (m_allLocalArrays.empty())
                {
                    cout << "m_allLocalArrays is empty when meeting a local variable declaration at "; printLocation(vd->getLocStart());
                    exit(1);
                }
                m_allLocalArrays.back().push_back(vd);
            }
        }
        else if (varDeclIsStruct(vd))
        {
            if (vd->isFileVarDecl())    // global variable
            {
                m_allGlobalStructs.push_back(vd);
            }
            else
            {
                // This is a local variable. m_currentScope vector CANNOT be empty.
                if (m_allLocalStructs.empty())
                {
                    cout << "m_allLocalArrays is empty when meeting a local variable declaration at "; printLocation(vd->getLocStart());
                    exit(1);
                }
                m_allLocalStructs.back().push_back(vd);
            }
        }
        else if (varDeclIsPointer(vd))
        {
            if (vd->isFileVarDecl())    // global variable
            {
                m_allGlobalPointers.push_back(vd);
            }
            else
            {
                // This is a local variable. m_currentScope vector CANNOT be empty.
                if (m_allLocalPointers.empty())
                {
                    cout << "m_allLocalPointers is empty when meeting a local variable declaration at "; printLocation(vd->getLocStart());
                    exit(1);
                }
                m_allLocalPointers.back().push_back(vd);
            }
        }

        return true;
    }

    bool VisitSwitchStmt(SwitchStmt *ss)
    {
        if (switchConditionRange != nullptr)
            delete switchConditionRange;

        switchConditionRange = new SourceRange(ss->getSwitchLoc(), ss->getBody()->getLocStart());

        // remove switch statements that are already parsed/passed
        while (!m_switchCaseTracker.empty() && !locationIsInRange(ss->getLocStart(), m_switchCaseTracker.back().first))
            m_switchCaseTracker.pop_back();

        vector<string> cases;
        SwitchCase *sc = ss->getSwitchCaseList();

        // collect all case values' strings
        while (sc != nullptr)
        {
            if (isa<DefaultStmt>(sc))
            {
                sc = sc->getNextSwitchCase();
                continue;
            }

            SourceLocation keywordLoc = sc->getKeywordLoc();    // before 'case'
            SourceLocation colonLoc = sc->getColonLoc();
            string caseStatement{""};

            // retrieve 'case label' without colon
            SourceLocation walk = keywordLoc;
            while (walk != colonLoc)
            {
                caseStatement += *(m_srcmgr.getCharacterData(walk));
                walk = walk.getLocWithOffset(1);
            }

            vector<string> words;
            split_string_into_vector(caseStatement, words, string(" "));

            // retrieve label string
            string caseValue = words.back();

            // avoid trailing spaces between case label and colon
            while (caseValue.empty())
            {
                words.pop_back();
                caseValue = words.back();
            }

            if (caseValue.front() == '\'' && caseValue.back() == '\'')
            {
                caseValue = convertCharStringToIntString(caseValue);
            }

            cases.push_back(caseValue);

            sc = sc->getNextSwitchCase();
        }

        m_switchCaseTracker.push_back(make_pair(SourceRange(ss->getLocStart(), ss->getLocEnd()), cases));

        /*for (auto e: m_switchCaseTracker)
        {
            printRange(e.first);
            print_vec(e.second);
        }*/

        return true;
    }

    bool VisitSwitchCase(SwitchCase *sc)
    {
        if (switchCaseRange != nullptr)
            delete switchCaseRange;

        switchCaseRange = new SourceRange(sc->getLocStart(), sc->getColonLoc());

        return true;
    }

    bool VisitMemberExpr(MemberExpr *me)
    {
        SourceLocation start = me->getLocStart();
        string refName{m_rewriter.ConvertToString(me)};
        SourceLocation end = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                        getLineNumber(&start),
                                        getColNumber(&start) + refName.length());

        if (m_holder->doVSCR)
        {
            if (!isInsideArraySize && !isInsideEnumDecl && targetInMutationRange(&start, &end))
                generateVSCRMutant(me, end);
        }

        //=================================================
        //=== GENERATING Vsrr, Varr, Vtrr, Vprr MUTANTS ===
        //=================================================
        if (m_holder->doVGSR || m_holder->doVLSR || m_holder->doVGAR || m_holder->doVLAR ||
            m_holder->doVGTR || m_holder->doVLTR || m_holder->doVGPR || m_holder->doVLPR)
        {
            if (!isInsideArraySize && !isInsideEnumDecl && targetInMutationRange(&start, &end))
            {
                if (exprIsScalar(cast<Expr>(me)) && !exprIsPointer(cast<Expr>(me)))
                {
                    if (m_holder->doVGSR)
                        generateVGSRMutant(&start, &end, refName);

                    if (m_holder->doVLSR)
                        generateVLSRMutant(&start, &end, refName);
                }
                else if (exprIsArray(cast<Expr>(me)))
                {
                    if (m_holder->doVGAR)
                        generateVGARMutant(&start, &end, refName, me->getType());

                    if (m_holder->doVLAR)
                        generateVLARMutant(&start, &end, refName, me->getType());
                }
                else if (exprIsStruct(cast<Expr>(me)))
                {
                    if (m_holder->doVGTR)
                        generateVGTRMutant(&start, &end, refName, me->getType());

                    if (m_holder->doVLTR)
                        generateVLTRMutant(&start, &end, refName, me->getType());
                }
                else if (exprIsPointer(cast<Expr>(me)))
                {
                    if (m_holder->doVGPR)
                        generateVGPRMutant(&start, &end, refName, me->getType());

                    if (m_holder->doVLPR)
                        generateVLPRMutant(&start, &end, refName, me->getType());
                }
            }
        }

        return true;
    }

    bool VisitStmt(Stmt *s)
    {
        SourceLocation start = s->getLocStart();
        SourceLocation end = s->getLocEnd();

        while (!m_currentScope.empty())
        {
            if (!locationIsInRange(start, m_currentScope.back()))
            {
                m_currentScope.pop_back();
                m_allLocalScalars.pop_back();
                m_allLocalArrays.pop_back();
                m_allLocalStructs.pop_back();
                m_allLocalPointers.pop_back();
            }
            else
            {
                break;
            }
        }

        if (isInsideArraySize && !locationIsInRange(start, *arrayDeclRange))
        {
            isInsideArraySize = false;
        }

        return true;
    }
    
    bool VisitFunctionDecl(FunctionDecl *f) 
    {   
        // Function with nobody, and function declaration within another function is function prototype.
        // no global or local variable mutation will be applied here.
        if (!f->hasBody() || locationIsInRange(f->getLocStart(), *currentFunctionDeclRange))
        {
            functionPrototypeRange = new SourceRange(f->getLocStart(), f->getLocEnd());
        }
        else
        {
            // entering a new local scope
            m_allLocalScalars.clear();
            m_allLocalArrays.clear();
            m_allLocalPointers.clear();
            m_allLocalStructs.clear();
            m_currentScope.clear();

            m_currentScope.push_back(SourceRange(f->getLocStart(), f->getLocEnd()));
            m_allLocalScalars.push_back(DeclInScope());
            m_allLocalArrays.push_back(DeclInScope());
            m_allLocalStructs.push_back(DeclInScope());
            m_allLocalPointers.push_back(DeclInScope());

            isInsideEnumDecl = false;
            // isInsideFunctionDecl = true;

            if (currentFunctionDeclRange != nullptr)
                delete currentFunctionDeclRange;

            currentFunctionDeclRange = new SourceRange(f->getLocStart(), f->getLocEnd());

            // remove local constants appearing before currently parsed function
            auto it = m_allLocalConsts->begin();
            for ( ; it != m_allLocalConsts->end(); ++it)
                if (!locationBeforeRange(it->second.first, *currentFunctionDeclRange))
                    break;
            
            if (it != m_allLocalConsts->begin())
                m_allLocalConsts->erase(m_allLocalConsts->begin(), it);
        }

        return true;
    }
};

class MyASTConsumer : public ASTConsumer
{
public:
    MyASTConsumer(SourceManager &sm, LangOptions &langopt, 
                UserInput *userInput, MutantOperatorHolder *holder,
                CompilerInstance *CI, vector<SourceLocation> *labels,
                LabelUsageMap *usageMap, GlobalScalarConstants *globalConsts,
                LocalScalarConstants *localConsts) 
        : Visitor(sm, langopt, userInput, holder, CI, 
                    labels, usageMap, globalConsts, localConsts) 
    { 
    }

    void setSema(Sema *sema)
    {
        Visitor.setSema(sema);
    }

    /*virtual bool HandleTopLevelDecl(DeclGroupRef DR) 
    {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) 
        {
            // Travel each function declaration using MyASTVisitor
            Visitor.TraverseDecl(*b);
        }
        return true;
    }*/

    virtual void HandleTranslationUnit(ASTContext &Context)
    {
        /* we can use ASTContext to get the TranslationUnitDecl, which is
        a single Decl that collectively represents the entire source file */
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    MyASTVisitor Visitor;
};

class InformationVisitor : public RecursiveASTVisitor<InformationVisitor>
{
public:
    vector<SourceLocation> *m_labels; 

    CompilerInstance *m_compinst;
    MutantOperatorHolder *m_holder;

    SourceManager &m_srcmgr;
    LangOptions &m_langopt;

    Rewriter m_rewriter;

    LabelUsageMap labels;

    GlobalScalarConstants m_allGlobalConsts;
    LocalScalarConstants m_allLocalConsts;

    SourceRange *lastParsedFunctionRange;

    set<string> localConstCache;
    set<string> globalConstCache;

    InformationVisitor(CompilerInstance *CI, MutantOperatorHolder *holder)
        :m_compinst(CI), m_holder(holder), m_srcmgr(CI->getSourceManager()),
        m_langopt(CI->getLangOpts())
    {
        m_labels = new vector<SourceLocation>();
        lastParsedFunctionRange = new SourceRange(m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()),
                                                    m_srcmgr.getLocForStartOfFile(m_srcmgr.getMainFileID()));

        m_rewriter.setSourceMgr(m_srcmgr, m_langopt);
    }

    // Return the line number of a source location.
    int getLineNumber(SourceLocation *loc) 
    {
        return static_cast<int>(m_srcmgr.getExpansionLineNumber(*loc));
    }

    // Return the column number of a source location.
    int getColNumber(SourceLocation *loc) 
    {
        return static_cast<int>(m_srcmgr.getExpansionColumnNumber(*loc));
    }

    // Return True if the location is inside the range
    // Return False otherwise
    bool locationIsInRange(SourceLocation loc, SourceRange range)
    {
        SourceLocation start = range.getBegin();
        SourceLocation end = range.getEnd();

        // line number of loc is out of range
        if (getLineNumber(&loc) < getLineNumber(&start) || getLineNumber(&loc) > getLineNumber(&end))
            return false;

        // line number of loc is same as line number of start but col number of loc is out of range
        if (getLineNumber(&loc) == getLineNumber(&start) && getColNumber(&loc) < getColNumber(&start))
            return false;

        // line number of loc is same as line number of end but col number of loc is out of range
        if (getLineNumber(&loc) == getLineNumber(&end) && getColNumber(&loc) > getColNumber(&end))
            return false;

        return true;
    }

    void printLocation(SourceLocation loc)
    {
        cout << getLineNumber(&loc) << "\t" << getColNumber(&loc) << endl;
    }

    void addLabel(LabelDeclLocation labelLoc, SourceLocation gotoLoc)
    {   
        GotoLocationList newlist{gotoLoc};
        pair<LabelUsageMap::iterator,bool> insertResult = labels.insert(pair<LabelDeclLocation, GotoLocationList>(labelLoc, newlist));
        if (insertResult.second == false)
        {
            ((insertResult.first)->second).push_back(gotoLoc);
        }
    }
    
    bool VisitLabelStmt(LabelStmt *ls)
    {
        string labelName{ls->getName()};
        SourceLocation startloc = ls->getLocStart();

        m_labels->push_back(startloc);

        // Insert into LabelUsageMap the label declaration if its not already in there.
        labels.insert(pair<LabelDeclLocation, GotoLocationList>(LabelDeclLocation(getLineNumber(&startloc),
                                                                                    getColNumber(&startloc)),
                                                                 GotoLocationList()));

        return true;
    }    

    bool VisitGotoStmt(GotoStmt * gs)
    {
        LabelStmt *label = gs->getLabel()->getStmt();
        SourceLocation labelStartLoc = label->getLocStart();

        addLabel(LabelDeclLocation(getLineNumber(&labelStartLoc), getColNumber(&labelStartLoc)), 
                gs->getGotoLoc());
        return true;
    }



    bool VisitExpr(Expr *e)
    {
        if (isa<CharacterLiteral>(e) || isa<FloatingLiteral>(e) || isa<IntegerLiteral>(e))
        {
            // cout << "scalar constant: " << m_rewriter.ConvertToString(e) << endl;

            if (locationIsInRange(e->getLocStart(), *lastParsedFunctionRange))  
            {
                // cout << "local constant\n"; 
                if (localConstCache.find(m_rewriter.ConvertToString(e)) == localConstCache.end())
                {
                    localConstCache.insert(m_rewriter.ConvertToString(e));
                    m_allLocalConsts.push_back(make_pair(m_rewriter.ConvertToString(e),
                                                        make_pair(e->getLocStart(), 
                                                                ((e->getType()).getTypePtr())->isFloatingType())));
                }

                // for (auto constant: m_allLocalConsts)
                //     cout << constant.first << "//" << constant.second.second << endl;
            }
            else
            {
                // cout << "global constant\n";
                if (globalConstCache.find(m_rewriter.ConvertToString(e)) == globalConstCache.end())
                {
                    globalConstCache.insert(m_rewriter.ConvertToString(e));
                    m_allGlobalConsts.push_back(make_pair(m_rewriter.ConvertToString(e),
                                                    ((e->getType()).getTypePtr())->isFloatingType()));
                }
                // print_set(m_allGlobalConsts);
            }
        }

        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *fd)
    {
        if (fd->hasBody() && !locationIsInRange(fd->getLocStart(), *lastParsedFunctionRange))
        {
            if (lastParsedFunctionRange != nullptr)
                delete lastParsedFunctionRange;

            lastParsedFunctionRange = new SourceRange(fd->getLocStart(), fd->getLocEnd());

            // clear the constants of the previously parsed function
            // get ready to contain constants of the new function
            localConstCache.clear(); 
        }

        return true;
    }
};

class InformationGatherer : public ASTConsumer
{
public:
    InformationGatherer(CompilerInstance *CI, MutantOperatorHolder *holder)
        :Visitor(CI, holder)
    {
    }

    // ~InformationGatherer();

    virtual void HandleTranslationUnit(ASTContext &Context)
    {
        /* we can use ASTContext to get the TranslationUnitDecl, which is
        a single Decl that collectively represents the entire source file */
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

    vector<SourceLocation>* getLabels()
    {
        return Visitor.m_labels;
    }

    LabelUsageMap* getLabelUsageMap()
    {
        return &(Visitor.labels);
    }

    GlobalScalarConstants* getAllGlobalConstants()
    {
        return &(Visitor.m_allGlobalConsts);
    }

    LocalScalarConstants* getAllLocalConstants()
    {
        return &(Visitor.m_allLocalConsts);
    }
    
private:
    InformationVisitor Visitor;
};

/** 
    Check if this directory exists

    @param  directory the directory to be checked
    @return True if the directory exists
            False otherwise
*/
bool directoryExists( const std::string &directory )
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

void printUsageErrorMsg()
{
    cout << "Invalid command.\nUsage: tool <filename> [-m <op> [-A \"<domain>\"] ";
    cout << "[-B \"<range>\"]] [-rs <line #> <col #>] [-re <line #> <col #>] [-l <max>] [-o <dir>]" << endl;
}

void printLineColNumberErrorMsg()
{
    cout << "Invalid line/column number\nUsage: [-rs <line #> <col #>] [-re <line #> <col #>]\n";
}

// Called when incrementing iterator variable i in a loop
// When incrementing i in a loop, I expected that i will not exceed max.
// However if user made mistake in input command, it can happen.
void increment_i(int *i, int max)
{
    ++(*i);
    if ((*i) >= max)
    {
        printUsageErrorMsg();
        exit(1);
    }
}

InformationGatherer* gatherInfo(char *filename, MutantOperatorHolder *holder)
{
    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
    CompilerInstance TheCompInst;
    
    // Diagnostics manage problems and issues in compile 
    TheCompInst.createDiagnostics(NULL, false);

    // Set target platform options 
    // Initialize target info with the default triple for our platform.
    TargetOptions *TO = new TargetOptions();
    TO->Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *TI = TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
    TheCompInst.setTarget(TI);

    // FileManager supports for file system lookup, file system caching, and directory search management.
    TheCompInst.createFileManager();
    FileManager &FileMgr = TheCompInst.getFileManager();
    
    // SourceManager handles loading and caching of source files into memory.
    TheCompInst.createSourceManager(FileMgr);
    SourceManager &SourceMgr = TheCompInst.getSourceManager();
    
    // Prreprocessor runs within a single source file
    TheCompInst.createPreprocessor();
    
    // ASTContext holds long-lived AST nodes (such as types and decls) .
    TheCompInst.createASTContext();

    // Enable HeaderSearch option
    llvm::IntrusiveRefCntPtr<clang::HeaderSearchOptions> hso( new HeaderSearchOptions());
    HeaderSearch headerSearch(hso,
                              TheCompInst.getFileManager(),
                              TheCompInst.getDiagnostics(),
                              TheCompInst.getLangOpts(),
                              TI);

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
        hso->AddPath(include_paths[i], 
                    clang::frontend::Angled, 
                    false, 
                    false);
    // </Warning!!> -- End of Platform Specific Code

    InitializePreprocessor(TheCompInst.getPreprocessor(), 
                  TheCompInst.getPreprocessorOpts(),
                  *hso,
                  TheCompInst.getFrontendOpts());

    // Set the main file handled by the source manager to the input file.
    const FileEntry *FileIn = FileMgr.getFile(filename);
    SourceMgr.createMainFileID(FileIn);
    
    // Inform Diagnostics that processing of a source file is beginning. 
    TheCompInst.getDiagnosticClient().BeginSourceFile(TheCompInst.getLangOpts(),&TheCompInst.getPreprocessor());

    // Parse the file to AST, registering our informtion gatherer as the AST consumer.
    InformationGatherer *TheGatherer = new InformationGatherer(&TheCompInst, holder);
    ParseAST(TheCompInst.getPreprocessor(), TheGatherer, TheCompInst.getASTContext());

    return TheGatherer;
}

// Wrap up currently-entering mutant operator (if can)
// before change the state to NONE.
void clearState(State &state, string op, set<string> &domain, set<string> &range, MutantOperatorHolder *holder)
{
    switch (state)
    {
        case State::A_OPTIONAL:
        case State::B_OPTIONAL:
        case State::MUTANT_NAME_OPTIONAL:
        {
            // there is a mutant operator to be wrapped
            validate_replacee(op, domain);
            validate_replacer(op, range);
            MutantOperator newOp{op, domain, range};

            if (!(holder->addOperator(newOp)))
            {
                cout << "Error adding operator " << op << endl;
                exit(1);
            }

            domain.clear();
            range.clear();

            break;
        }
        case State::NONE:
            break;
        default:
            printUsageErrorMsg();
            exit(1);
    };

    state = State::NONE;
}

void handleInput(string input, State &state, string &op, set<string> &domain,
                set<string> &range, MutantOperatorHolder *holder, 
                string &outputDir, int &limit, SourceLocation *start, 
                SourceLocation *end, int &lineNum, int &colNum, SourceManager &sm)
{
    switch (state)
    {
        case State::NONE:
            printUsageErrorMsg();
            exit(1);

        case State::MUTANT_NAME:
            for (int i = 0; i < input.length() ; ++i)
            {
                if (input[i] >= 'a' && input[i] <= 'z')
                    input[i] -= 32;
            }

            op.clear();
            op = input;

            /*if (op.compare("CRCR") == 0)
                state = State::B_OPTIONAL;
            else*/
            state = State::A_OPTIONAL;
            break;

        case State::OUTPUT_DIR:
            outputDir.clear();
            outputDir = input + "/";
            
            if (!directoryExists(outputDir)) 
            {
                cout << "Invalid directory for -o option: " << outputDir << endl;;
                exit(1);
            }

            state = State::NONE;
            break;

        case State::RS_LINE:
            if (!convertStringToInt(input, lineNum))
            {
                printLineColNumberErrorMsg();
                exit(1);
            }

            if (lineNum <= 0)
            {
                cout << "Input line number is not positive. Default to 1.\n";
                lineNum = 1;
            }

            state = State::RS_COL;
            break;

        case State::RS_COL:
            if (!convertStringToInt(input, colNum))
            {
                printLineColNumberErrorMsg();
                exit(1);
            }

            if (colNum <= 0)
            {
                cout << "Input col number is not positive. Default to 1.\n";
                colNum = 1;
            }

            *start = sm.translateLineCol(sm.getMainFileID(), lineNum, colNum);
            state = State::NONE;
            break;

        case State::RE_LINE:
            if (!convertStringToInt(input, lineNum))
            {
                printLineColNumberErrorMsg();
                exit(1);
            }

            if (lineNum <= 0)
            {
                cout << "Input line number is not positive. Default to 1.\n";
                lineNum = 1;
            }

            state = State::RE_COL;
            break;

        case State::RE_COL:
            if (!convertStringToInt(input, colNum))
            {
                printLineColNumberErrorMsg();
                exit(1);
            }

            if (colNum <= 0)
            {
                cout << "Input col number is not positive. Default to 1.\n";
                colNum = 1;
            }

            *end = sm.translateLineCol(sm.getMainFileID(), lineNum, colNum);
            state = State::NONE;
            break;

        case State::LIMIT_MUTANT:
            if (!convertStringToInt(input, limit))
            {
                cout << "Invalid input for -l option, must be an positive integer smaller than 2147483648\nUsage: -l <max>\n";
                exit(1);
            }

            if (limit <= 0)
            {
                cout << "Invalid input for -l option, must be an positive integer smaller than 2147483648\nUsage: -l <max>\n";
                exit(1);
            }            

            state = State::NONE;
            break;

        case State::A_OPTIONAL:
        case State::B_OPTIONAL:
        case State::MUTANT_NAME_OPTIONAL:
        {
            validate_replacee(op, domain);
            validate_replacer(op, range);
            MutantOperator newOp{op, domain, range};
            if (!(holder->addOperator(newOp)))
            {
                cout << "Error adding operator " << op << endl;
                exit(1);
            }

            state = State::MUTANT_NAME;
            domain.clear();
            range.clear();
            handleInput(input, state, op, domain, range, holder, outputDir,
                        limit, start, end, lineNum, colNum, sm);
            break;
        }

        case State::A_OP:
            split_string_into_set(input, domain, string(","));
            validate_replacee(op, domain);
            state = State::B_OPTIONAL;
            break;

        case State::B_OP:
            split_string_into_set(input, range, string(","));
            validate_replacer(op, range);
            state = State::MUTANT_NAME_OPTIONAL;
            break;

        default:
            // cout << "unknown state: " << state << endl;
            exit(1);
    };
}

int main(int argc, char *argv[])
{
    if (argc < 2) 
    {
        printUsageErrorMsg();
        return 1;
    }

    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
    CompilerInstance TheCompInst;
    
    // Diagnostics manage problems and issues in compile 
    TheCompInst.createDiagnostics(NULL, false);

    // Set target platform options 
    // Initialize target info with the default triple for our platform.
    TargetOptions *TO = new TargetOptions();
    TO->Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *TI = TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
    TheCompInst.setTarget(TI);

    // FileManager supports for file system lookup, file system caching, and directory search management.
    TheCompInst.createFileManager();
    FileManager &FileMgr = TheCompInst.getFileManager();
    
    // SourceManager handles loading and caching of source files into memory.
    TheCompInst.createSourceManager(FileMgr);
    SourceManager &SourceMgr = TheCompInst.getSourceManager();
    
    // Prreprocessor runs within a single source file
    TheCompInst.createPreprocessor();
    
    // ASTContext holds long-lived AST nodes (such as types and decls) .
    TheCompInst.createASTContext();

    // Enable HeaderSearch option
    llvm::IntrusiveRefCntPtr<clang::HeaderSearchOptions> hso( new HeaderSearchOptions());
    HeaderSearch headerSearch(hso,
                              TheCompInst.getFileManager(),
                              TheCompInst.getDiagnostics(),
                              TheCompInst.getLangOpts(),
                              TI);

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
        hso->AddPath(include_paths[i], 
                    clang::frontend::Angled, 
                    false, 
                    false);
    // </Warning!!> -- End of Platform Specific Code

    InitializePreprocessor(TheCompInst.getPreprocessor(), 
                  TheCompInst.getPreprocessorOpts(),
                  *hso,
                  TheCompInst.getFrontendOpts());

    // A Rewriter helps us manage the code rewriting task.
    /*Rewriter TheRewriter;
    TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());*/

    // Set the main file handled by the source manager to the input file.
    const FileEntry *FileIn = FileMgr.getFile(argv[1]);
    SourceMgr.createMainFileID(FileIn);
    
    // Inform Diagnostics that processing of a source file is beginning. 
    TheCompInst.getDiagnosticClient().BeginSourceFile(TheCompInst.getLangOpts(),&TheCompInst.getPreprocessor());

    //=======================================================
    //================ USER INPUT ANALYSIS ==================
    //=======================================================
    
    // default output directory is current directory.
    string outputDir = "./";

    // by default, as many mutants will be generated at a location per mutant operator as possible.
    int limit = INT_MAX;

    // start and end of mutation range by default is start and end of file
    SourceLocation startOfRange = SourceMgr.getLocForStartOfFile(SourceMgr.getMainFileID());
    SourceLocation endOfRange = SourceMgr.getLocForEndOfFile(SourceMgr.getMainFileID());

    MutantOperatorHolder *holder = new MutantOperatorHolder();

    // if -m option is specified, apply only specified operators to input file.
    bool useAllOperator = true;

    // Validate and analyze user's inputs.
    State state = State::NONE;
    string mutantOpName{""};
    set<string> replacee;
    set<string> replacer;
    int lineNum{0};
    int colNum{0};

    for (int i = 2; i < argc; ++i)
    {
        string option = argv[i];
        cout << "handling " << option << endl;

        if (option.compare("-m") == 0)
        {
            useAllOperator = false;
            clearState(state, mutantOpName, replacee, replacer, holder);
            
            mutantOpName.clear();
            replacee.clear();
            replacer.clear();
            
            state = State::MUTANT_NAME;
        }
        else if (option.compare("-l") == 0)
        {
            clearState(state, mutantOpName, replacee, replacer, holder);
            state = State::LIMIT_MUTANT;
        }
        else if (option.compare("-rs") == 0)
        {
            clearState(state, mutantOpName, replacee, replacer, holder);
            state = State::RS_LINE;
        }
        else if (option.compare("-re") == 0)
        {
            clearState(state, mutantOpName, replacee, replacer, holder);
            state = State::RE_LINE;
        }
        else if (option.compare("-o") == 0)
        {
            clearState(state, mutantOpName, replacee, replacer, holder);
            state = State::OUTPUT_DIR;
        }
        else if (option.compare("-A") == 0)
        {
            if (state != State::A_OPTIONAL)
            {
                // not expecting -A at this position
                printUsageErrorMsg();
                exit(1);
            }
            else
                state = State::A_OP;
        }
        else if (option.compare("-B") == 0)
        {
            if (state != State::B_OPTIONAL && state != State::A_OPTIONAL)
            {
                // not expecting -B at this position
                printUsageErrorMsg();
                exit(1);
            }
            else
                state = State::B_OP;
        }
        else
        {
            handleInput(option, state, mutantOpName, replacee, replacer, holder,
                        outputDir, limit, &startOfRange, &endOfRange, lineNum, 
                        colNum, SourceMgr);
            // cout << "mutant op name: " << mutantOpName << endl;
            // cout << "domain: "; print_set(replacee);
            // cout << "range "; print_set(replacer);
            // cout << "out dir: " << outputDir << endl;
            // cout << "limit: " << limit << endl;
            // cout << "start: " << startOfRange.printToString(SourceMgr) << endl;
            // cout << "end: " << endOfRange.printToString(SourceMgr) << endl;
            // cout << "line col: " << lineNum << " " << colNum << endl;
            // cout << "==========================================\n\n";
        }
    }

    clearState(state, mutantOpName, replacee, replacer, holder);

    // exit(1);

    if (useAllOperator) 
    {
        holder->useAll();
    }

    // Make mutation database file named <inputfilename>_mut_db.out
    string inputFilename = argv[1];
    string mutDbFilename(outputDir);
    mutDbFilename.append(inputFilename, 0, inputFilename.length()-2);
    mutDbFilename += "_mut_db.out";

    // Open the file with mode TRUNC to create the file if not existed
    // or delete content if existed.
    ofstream out_mutDb(mutDbFilename.data(), ios::trunc);   
    out_mutDb.close();

    // Create UserInput object pointer to pass as attribute for MyASTConsumer
    UserInput *userInput = new UserInput(inputFilename, mutDbFilename, startOfRange, endOfRange, outputDir, limit);

    //=======================================================
    //====================== PARSING ========================
    //=======================================================
    InformationGatherer *TheGatherer = gatherInfo(argv[1], holder);

    // Create an AST consumer instance which is going to get called by ParseAST.
    MyASTConsumer TheConsumer(SourceMgr, TheCompInst.getLangOpts(), userInput, 
                            holder, &TheCompInst, TheGatherer->getLabels(), TheGatherer->getLabelUsageMap(),
                            TheGatherer->getAllGlobalConstants(), TheGatherer->getAllLocalConstants());

    Sema sema(TheCompInst.getPreprocessor(), TheCompInst.getASTContext(), TheConsumer);
    TheConsumer.setSema(&sema);

    // Parse the file to AST, registering our consumer as the AST consumer.
    ParseAST(sema);

    // if (TheGatherer != nullptr)
    //     delete TheGatherer;
    
    // if (userInput != nullptr)
    //     delete userInput;

    return 0;
}
