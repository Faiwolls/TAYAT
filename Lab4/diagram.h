//diagram.h
#pragma once
#include "scanner.h"
#include "defines.h"
//#include "data_type.h"
//#include "tree.h"
#include <string>
#include <vector>

enum DATA_TYPE {
    TYPE_INT = 1,
    TYPE_SHORT_INT,
    TYPE_LONG_INT,
    TYPE_BOOL,
    TYPE_ARRAY,
    TYPE_SCOPE,
    TYPE_UNDEFINED
};

class Diagram {
private:
    Scanner* sc;
    std::vector<int> push_tok;
    std::vector<std::string> push_lex;
    int cur_tok;
    std::string cur_lex;
    DATA_TYPE current_decl_type;
    int current_arr_elem_count;

    int nextToken();
    int peekToken();
    void pushBack(int tok, const std::string& lex);
    void lexError();
    void synError(const std::string& msg);
    void semError(const std::string& msg);

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

public:
    Diagram(Scanner* scanner);
    void ParseProgram();
};

