// diagram.h
#pragma once
#include "scanner.h"
#include "defines.h"
#include <string>
#include <vector>

class Diagram {
private:
    Scanner* sc;
    std::vector<int> push_tok;
    std::vector<std::string> push_lex;
    int cur_tok;
    std::string cur_lex;

    int nextToken();
    int peekToken(int n);
    void pushBack(int tok, const std::string& lex);
    void lexError();
    void synError(const std::string& msg);

    void Program();
    void TopDecl();
    void FuncDecl();
    void Params();
    void Block();
    void BlockItems();
    void Stmt();
    void WhileStmt();
    void ReturnStmt();
    void Expr();
    void BitOr();
    void BitXor();
    void BitAnd();
    void Rel();
    void Add();
    void Mul();
    void Prim();

public:
    Diagram(Scanner* scanner);
    void ParseProgram();
};