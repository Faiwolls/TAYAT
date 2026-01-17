#pragma once

#include "scanner.h"
#include "defines.h"
#include "data_type.h"
#include "tree.h"
#include <string>
#include <vector>
#include <stack>

class Diagram {
private:
    Scanner* sc;
    std::vector<int> push_tok;
    std::vector<std::string> push_lex;
    int cur_tok;
    std::string cur_lex;
    DATA_TYPE current_decl_type;
    int current_arr_elem_count;
    std::stack<SemNode> eval_stack;

    int nextToken();
    int peekToken();
    void pushBack(int tok, const std::string& lex);

    void lexError();
    void synError(const std::string& msg);
    void semError(const std::string& msg);
    void interpError(const std::string& msg);

    void Program();
    void TopDecl();
    void MainFunc();
    void VarDecl();
    void ConstDecl();
    void Block();
    void BlockItems();
    void Stmt();
    void WhileStmt();
    DATA_TYPE Expr();
    DATA_TYPE BitOr();
    DATA_TYPE BitXor();
    DATA_TYPE BitAnd();
    DATA_TYPE Rel();
    DATA_TYPE Add();
    DATA_TYPE Mul();
    DATA_TYPE Prim();

    void pushValue(const SemNode& node);
    SemNode popValue();
    SemNode evaluateConstant(const std::string& value, DATA_TYPE type);
    void executeAssignment(const std::string& varName, DATA_TYPE exprType, int line, int col);

public:
    Diagram(Scanner* scanner);
    void ParseProgram(bool isInterp = true, bool isDebug = false);
};