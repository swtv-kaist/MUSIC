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

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
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

using namespace clang;
using namespace std;

/*class BinaryOperatorReplace
{
public:
    vector<string> m_arithmeticReplacer;
    vector<string> m_arithAssignReplacer;
    vector<string> m_bitwiseReplacer;
    vector<string> m_bitwiseAssignReplacer;
    vector<string> m_shiftReplacer;
    vector<string> m_shiftAssignReplacer;
    vector<string> m_relationalReplacer;
    vector<string> m_logicalReplacer;    
    vector<string> m_plainAssignReplacer;

    vector<string> m_arithmeticReplacee;
    vector<string> m_arithAssignReplacee;
    vector<string> m_bitwiseReplacee;
    vector<string> m_bitwiseAssignReplacee;
    vector<string> m_shiftReplacee;
    vector<string> m_shiftAssignReplacee;
    vector<string> m_relationalReplacee;
    vector<string> m_logicalReplacee;    
    vector<string> m_plainAssignReplacee;

    BinaryOperatorReplace(){};
    ~BinaryOperatorReplace(){};
    
    void addArithReplacer(string *arith, int len)
    {
        m_arithmeticReplacer.insert(m_arithmeticReplacer.end(), arith, arith+len);
    }

    void addArithAssignReplacer(string *arithAssign, int len) 
    {
        m_arithAssignReplacer.insert(m_arithAssignReplacer.end(), arithAssign, arithAssign+len);
    }

    void addBitwiseReplacer(string *replacer, int len)
    {
        m_bitwiseReplacer.insert(m_bitwiseReplacer.end(), replacer, replacer+len);
    }

    void addBitwiseAssignReplacer(string *replacer, int len)
    {
        m_bitwiseAssignReplacer.insert(m_bitwiseAssignReplacer.end(), replacer, replacer+len);
    }

    void addShiftReplacer(string *replacer, int len)
    {
        m_shiftReplacer.insert(m_shiftReplacer.end(), replacer, replacer+len);
    }

    void addShiftAssignReplacer(string *replacer, int len)
    {
        m_shiftAssignReplacer.insert(m_shiftAssignReplacer.end(), replacer, replacer+len);
    }

    void addLogicalReplacer(string *replacer, int len)
    {
        m_logicalReplacer.insert(m_logicalReplacer.end(), replacer, replacer+len);
    }

    void addRelationalReplacer(string *replacer, int len)
    {
        m_relationalReplacer.insert(m_relationalReplacer.end(), replacer, replacer+len);
    }

    void addPlainAssignReplacer(string *replacer, int len)
    {
        m_plainAssignReplacer.insert(m_plainAssignReplacer.end(), replacer, replacer+len);
    }
};*/

// Print out each elemet of a string vector in a single line.
void print_vec(vector<string> &vec)
{
    for (vector<string>::iterator x = vec.begin(); x != vec.end(); ++x) {
         std::cout << ' ' << *x;
    }
    std::cout << '\n';
}

// Print out each element of a string set in a single line.
void print_set(set<string> &set)
{
    for (auto e: set)
    {
        cout << ' ' << e;
    }
    cout << "\n";
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
        cout << "new operator made. replacee and replacer:\n";
        print_set(m_targets);
        print_set(m_replaceOptions);
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
    Currently can only hold Binary Operator Mutant Operators (Obom).
    Operators that mutate the same token are in the same vector.
    There are 9 different vectors (for now).

    @param  vectors containing MutantOperator mutating arithmetic op,
            arithmetic assignment, bitwise op, bitwise assignment,
            shift op, shift assignment, logical op, relational op,
            plain assignment.
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

    MutantOperatorHolder()
    {
    }
    ~MutantOperatorHolder(){}

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

        // Check which set does the operator belongs to.
        // Add the operator to the corresponding vector.
        if (logical.find(op.getName()) != logical.end())
        {
            bin_logical.push_back(op);
            cout << "added " << (--(bin_logical.end()))->m_name << endl;
        }
        else if (relational.find(op.getName()) != relational.end())
        {
            bin_relational.push_back(op);
        }
        else if (arithmetic.find(op.getName()) != arithmetic.end())
        {
            bin_arith.push_back(op);
        }
        else {
            // The operator does not belong to any of the supported categories
            return false;
        }

        return true;
    }

    /**
        Add to each Mutant Operator vector all the MutantOperators that it
        can receive, with tokens before and after mutation of each token
        same as in Agrawal's Design of Mutation Operators for C.        
        Called when user does not specify any specific operator(s) to use.
    */
    void useAll()
    {
        ;
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

    // mutant operators to be applied to input file.
    MutantOperatorHolder *m_holder;
    // int countOp;

    MyASTVisitor(SourceManager &sm, LangOptions &langopt, 
                UserInput *userInput, MutantOperatorHolder *holder) 
        : m_srcmgr(sm), m_langopt(langopt), m_holder(holder),
        m_userinput(userInput), m_mutFileNum(1) 
    {
        m_outFilename.assign(m_userinput->getInputFilename(), 0, (m_userinput->getInputFilename()).length()-2);
        m_outFilename += ".MUT";
    }

    // Return True if given operator is an arithmetic operator.
    bool isArithmeticOperator(const string &op) 
    {
        return op == "+" || op == "-" || 
                op == "*" || op == "/" || op == "%";
    }

    /*bool isRelationalOperator(const string &op) 
    {
        return op == ">" || op == "<" || 
                op == ">=" || op == "<=" ||
                op == "==" || op == "!=";
    }*/

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

    // Return the name of the next mutated code file
    string generateMutFilename() {
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
    void generateMutant(string &op, SourceLocation *startloc, SourceLocation *endloc, string &token, string &replacingToken) 
    {
        // Create a Rewriter objected needed for edit input file
        Rewriter theRewriter;
        theRewriter.setSourceMgr(m_srcmgr, m_langopt);
        
        // Replace the string started at startloc, length equal to token's length
        // with the replacingToken. 
        theRewriter.ReplaceText(*startloc, token.length(), replacingToken);

        string mutFilename;
        mutFilename = generateMutFilename();

        // Make and write mutated code to output file.
        const RewriteBuffer *RewriteBuf = theRewriter.getRewriteBufferFor(m_srcmgr.getMainFileID());
        ofstream output(mutFilename.data());
        output << string(RewriteBuf->begin(), RewriteBuf->end());
        output.close();

        SourceLocation newendloc = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                                            getLineNumber(startloc),
                                                            getColNumber(startloc)+replacingToken.length());

        // Write information of the new mutated file into mutation database file
        writeToMutationDb(op, startloc, endloc, startloc, &newendloc, token, replacingToken);

        // each output mutated code file needs to have different name.
        ++m_mutFileNum;
    }

    /*bool applyLogicalMutantOp() 
    {
        return true;
    }*/

    // Return True if the given range is within the mutation range (mutation is permitted)
    bool targetInRange(SourceLocation *startloc, SourceLocation *endloc)
    {   
        // If line is out of bound, then this token is not mutated
        if ((getLineNumber(startloc) < getLineNumber(m_userinput->getStartOfRange())) ||
            (getLineNumber(endloc) > getLineNumber(m_userinput->getEndOfRange())))
        {
            return false;
        }

        // If column is out of bound, then this token is not mutated
        if ((getLineNumber(startloc) == getLineNumber(m_userinput->getStartOfRange())) &&
            ((getColNumber(startloc) < getColNumber(m_userinput->getStartOfRange())) ||
                (getColNumber(endloc) > getColNumber(m_userinput->getEndOfRange()))))
        {
            return false;
        }

        /*else if ((getLineNumber(endloc) == getLineNumber(m_userinput->getEndOfRange())) &&
            (getColNumber(endloc) > getColNumber(m_userinput->getEndOfRange())))
        {
            return false;
        }*/

        return true;
    }


    bool VisitStmt(Stmt *s) 
    {
        if (isa<Expr>(s))   // This is an expression
        {
            if (isa<BinaryOperator>(s)) 
            {
                // Retrieve the location and the operator of the expression
                BinaryOperator *binOp_s = cast<BinaryOperator>(s);
                string op = static_cast<string>(binOp_s->getOpcodeStr());
                SourceLocation startloc = binOp_s->getOperatorLoc();
                SourceLocation endloc = m_srcmgr.translateLineCol(m_srcmgr.getMainFileID(),
                                                                    getLineNumber(&startloc),
                                                                    getColNumber(&startloc)+op.length());

                // No mutation needs to be done if the token is not in the mutation range
                if (!(targetInRange(&startloc, &endloc)))
                {
                    return true;
                }

                vector<MutantOperator>::iterator startIt;
                vector<MutantOperator>::iterator endIt;

                // Classify the type of operator to traverse the correct MutantOperator vector
                if (isArithmeticOperator(op)) 
                {
                    startIt = (m_holder->bin_arith).begin();
                    endIt = (m_holder->bin_arith).end();
                }
                else if (binOp_s->isLogicalOp())
                {
                    startIt = (m_holder->bin_logical).begin();
                    endIt = (m_holder->bin_logical).end();
                }
                else if (binOp_s->isRelationalOp())
                {
                    startIt = (m_holder->bin_relational).begin();
                    endIt = (m_holder->bin_relational).end();
                }
                else
                {

                }

                for (auto it = startIt; it != endIt; ++it)
                {
                    // Generate mutants only for operators that can mutate this operator
                    if ((it->m_targets).find(op) != (it->m_targets).end())
                    {
                        // Getting the max # of mutants per mutation location.
                        int cap;
                        if (m_userinput->getLimit() != 0)
                        {
                            cap = m_userinput->getLimit();
                        }
                        else
                        {
                            // some random large number instead of MAX_INT
                            // meaning generate as many mutants as possible.
                            cap = 2017; 
                        }

                        for (auto replacingToken: it->m_replaceOptions)
                        {
                            // Do not replace the token with itself
                            if (replacingToken.compare(op) == 0)
                            {
                                continue;
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

                /*if (isArithmeticOperator(op)) 
                {
                    // Iterate through each mutant operator applicable on binary logical operator.
                    // Generate mutant if the operator is in the replacee list.
                    for (auto it: m_holder->bin_arith)
                    {
                        // if the operator is in the set of replacees...
                        if ((it.m_targets).find(op) != (it.m_targets).end())
                        {
                            // Getting the max # of mutants per mutation location.
                            int cap;
                            if (m_userinput->getLimit() != 0)
                            {
                                cap = m_userinput->getLimit();
                            }
                            else
                            {
                                // some random large number instead of MAX_INT
                                // meaning generate as many mutants as possible.
                                cap = 2017; 
                            }

                            for (auto replacingToken: it.m_replaceOptions)
                            {
                                if (replacingToken.compare(op) == 0)
                                {
                                    continue;
                                }

                                generateMutant(it.getName(), &startloc, &endloc, op, replacingToken);
                                --cap;
                                if (cap == 0)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (binOp_s->isLogicalOp()) 
                {
                    // Iterate through each mutant operator applicable on binary logical operator.
                    // Generate mutant if the operator is in the replacee list.
                    for (auto it: m_holder->bin_logical)
                    {
                        // if the operator is in the set of replacees...
                        if ((it.m_targets).find(op) != (it.m_targets).end())
                        {
                            // Getting the max # of mutants per mutation location.
                            int cap;
                            if (m_userinput->getLimit() != 0)
                            {
                                cap = m_userinput->getLimit();
                            }
                            else
                            {
                                // some random large number instead of MAX_INT
                                // meaning generate as many mutants as possible.
                                cap = 2017; 
                            }

                            for (auto replacingToken: it.m_replaceOptions)
                            {
                                if (replacingToken.compare(op) == 0)
                                {
                                    continue;
                                }

                                generateMutant(it.getName(), &startloc, &endloc, op, replacingToken);
                                --cap;
                                if (cap == 0)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (binOp_s->isRelationalOp())
                {
                    // Iterate through each mutant operator applicable on binary logical operator.
                    // Generate mutant if the operator is in the replacee list.
                    for (auto it: m_holder->bin_relational)
                    {
                        // if the operator is in the set of replacees...
                        if ((it.m_targets).find(op) != (it.m_targets).end())
                        {
                            // Getting the max # of mutants per mutation location.
                            int cap;
                            if (m_userinput->getLimit() != 0)
                            {
                                cap = m_userinput->getLimit();
                            }
                            else
                            {
                                // some random large number instead of MAX_INT
                                // meaning generate as many mutants as possible.
                                cap = 2017; 
                            }

                            for (auto replacingToken: it.m_replaceOptions)
                            {
                                if (replacingToken.compare(op) == 0)
                                {
                                    continue;
                                }

                                generateMutant(it.getName(), &startloc, &endloc, op, replacingToken);
                                --cap;
                                if (cap == 0)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
                else 
                {
                    // cout << "NOT arithmetic op: " << op << endl;
                    ;
                }*/
            }         
        }	
        return true;
    }
    
    bool VisitFunctionDecl(FunctionDecl *f) 
    {
        // Fill out this function for your homework
    	// if (f->hasBody()) {
    	    // cout << "function: " << f->getName() << endl;
        //printf("function: %s\n", f->getName());
    	// }
        return true;
    }
};

class MyASTConsumer : public ASTConsumer
{
public:
    MyASTConsumer(SourceManager &sm, LangOptions &langopt, 
                UserInput *userInput, MutantOperatorHolder *holder) 
        : Visitor(sm, langopt, userInput, holder) 
    { 
        //Visitor = MyASTVisitor(sm);
    }

    virtual bool HandleTopLevelDecl(DeclGroupRef DR) 
    {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) 
        {
            // Travel each function declaration using MyASTVisitor
            Visitor.TraverseDecl(*b);
        }
        return true;
    }

private:
    MyASTVisitor Visitor;
};

/** 
    Check if this directory exists

    @param  directory the directory to be checked
    @return True if the directory exists
            False otherwise
*/
bool directory_exists( const std::string &directory )
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

void print_usage_error_msg()
{
    cout << "Invalid command.\nUsage: tool <filename> [-m <op> [-A \"<domain>\"] ";
    cout << "[-B \"<range>\"]] [-rs <line #> <col #>] [-re <line #> <col #>] [-l <max>] [-o <dir>]" << endl;
}

// Called when incrementing iterator variable i in a loop
// When incrementing i in a loop, I expected that i will not exceed max.
// However if user made mistake in input command, it can happen.
void increment_i(int *i, int max)
{
    ++(*i);
    if ((*i) >= max)
    {
        print_usage_error_msg();
        exit(1);
    }
}

// Divide string into elements of a string set with delimiter comma (,)
void split_string(string target, set<string> &output)
{
    size_t pos = 0;
    string delimiter = ",";

    string token;
    while ((pos = target.find(delimiter)) != string::npos) {
        token = target.substr(0, pos);
        output.insert(token);
        target.erase(0, pos + delimiter.length());
    }
    output.insert(target);
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
    else
    {
        cout << "Error in validate_replacee\n";
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
    else
    {
        cout << "Error in validate_replacee\n";
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

int main(int argc, char *argv[])
{
    if (argc < 2) 
    {
        print_usage_error_msg();
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
    
    // default output directory is current directory.
    string outputDir = "./";

    // by default, as many mutants will be generated at a location per mutant operator as possible.
    int limit = 0;

    // start and end of mutation range by default is start and end of file
    SourceLocation startOfRange = SourceMgr.getLocForStartOfFile(SourceMgr.getMainFileID());
    SourceLocation endOfRange = SourceMgr.getLocForEndOfFile(SourceMgr.getMainFileID());

    /*String Array of each type of binary operator
    string arith[]          = {"+", "-", "*", "/", "%"};
    string arithAssign[]    = {"+=", "-=", "*=", "/=", "%="};
    string bitwise[]        = {"&", "|", "^"};
    string bitwiseAssign[]  = {"&=", "|=", "^="};
    string shift[]          = {">>", "<<"};
    string shiftAssign[]    = {">>=", "<<="};
    string logical[]        = {"&&", "||"};
    string relational[]     = {"==", "!=", ">", "<", ">=", "<="};
    string plainAssign[]    = {"="};*/

    MutantOperatorHolder *holder = new MutantOperatorHolder();

    // if -m option is specified, apply only specified operators to input file.
    bool useAllOperator = true;

    // Validate and analyze user's inputs.
    for (int i = 2; i < argc; ++i) 
    {
        string option = argv[i];

        if (option.compare("-m") == 0) 
        {
            useAllOperator = false;
            increment_i(&i, argc);
            string opName = argv[i];
            set<string> replacee;
            set<string> replacer;

            if (i + 1 != argc)
            {
                ++i;
                string nextOption = argv[i];

                if (nextOption.compare("-A") == 0) 
                {
                    increment_i(&i, argc);
                    string a_argument = argv[i];
                    split_string(a_argument, replacee);
                    validate_replacee(opName, replacee);

                    // Check if there is option -B following -A.
                    if (i + 1 != argc)  
                    {
                        ++i;
                        string nextnextOption = argv[i];
                        if (nextnextOption.compare("-B") == 0)
                        {
                            increment_i(&i, argc);
                            string b_argument = argv[i];
                            split_string(b_argument, replacer);
                            validate_replacer(opName, replacer);
                        }
                        // no option -B following -A.
                        else
                        {
                            --i;
                            // Set default replacer
                            validate_replacer(opName, replacer);
                            // print_set(replacer);
                        }
                    }
                    // this is the end of user command. no option -B.
                    else    
                    {
                        // Set default replacer
                        validate_replacer(opName, replacer);
                        // print_set(replacer);
                    }
                }
                else if (nextOption.compare("-B") == 0)
                {
                    // Set default replacee
                    validate_replacee(opName, replacee);
                    // print_set(replacee);

                    increment_i(&i, argc);
                    string b_argument = argv[i];
                    split_string(b_argument, replacer);
                    validate_replacer(opName, replacer);
                }
                else {  // next option is not -A or -B options
                    --i;
                    // Set default replacer and replacee for that operator
                    validate_replacee(opName, replacee);
                    validate_replacer(opName, replacer);
                }
            }
            // this is the end of user command.
            else {
                validate_replacee(opName, replacee);
                validate_replacer(opName, replacer);
            }

            // Create and insert specified mutant operator into appropriate vector of holder
            MutantOperator op{opName, replacee, replacer};
            if (!(holder->addOperator(op)))
            {
                cout << "Error adding operator.\n";
                return 1;
            }
        }
        else if (option.compare("-l") == 0) 
        {
            increment_i(&i, argc);
            stringstream convert(argv[i]);
            if (!(convert >> limit)) 
            {
                cout << "Invalid input for -l option, must be an integer\nUsage: -l <max>\n";
                return 1;
            }
            // cout << limit << endl;
        }
        else if (option.compare("-rs") == 0 || option.compare("-re") == 0) 
        {
            increment_i(&i, argc);
            stringstream convert(argv[i]);
            int lineNum;
            if (!(convert >> lineNum)) 
            {
                cout << "Invalid line/column number\nUsage: [-rs <line #> <col #>] [-re <line #> <col #>]\n";
                return 1;
            }

            // clearing stringstream for reuse
            convert.str("");
            convert.clear();

            increment_i(&i, argc);
            convert << argv[i];
            int colNum;
            if (!(convert >> colNum)) 
            {
                cout << "Invalid line/column number\nUsage: [-rs <line #> <col #>] [-re <line #> <col #>]\n";
                return 1;
            }

            // Set start or end of mutation range based on option
            if (option.compare("-rs") == 0) 
            {
                startOfRange = SourceMgr.translateLineCol(SourceMgr.getMainFileID(), lineNum, colNum);
            }
            else 
            {
                endOfRange = SourceMgr.translateLineCol(SourceMgr.getMainFileID(), lineNum, colNum);
            }
        } 
        else if (option.compare("-o") == 0) 
        {
            increment_i(&i, argc);
            outputDir = argv[i];
            if (!directory_exists(outputDir)) 
            {
                cout << "Invalid directory for -o option: " << outputDir << endl;;
                return 1;
            }
        }
        else 
        {
            cout << "Invalid option: " << argv[i] << "\n";
            print_usage_error_msg();
            return 1;
        }
    }

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

    // Create an AST consumer instance which is going to get called by ParseAST.
    MyASTConsumer TheConsumer(SourceMgr, TheCompInst.getLangOpts(), userInput, holder);

    // Parse the file to AST, registering our consumer as the AST consumer.
    ParseAST(TheCompInst.getPreprocessor(), &TheConsumer, TheCompInst.getASTContext());
    return 0;
}
