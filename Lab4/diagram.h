// diagram.h
#pragma once
#include "scanner.h"
#include "defines.h"
#include <string>
#include <vector>
#include <stdexcept>

class SyntaxError : public std::runtime_error {
public:
    SyntaxError(const std::string& message) : std::runtime_error(message) {}
};

class Diagram {
private:
    Scanner* sc;
    std::vector<int> push_tok;
    std::vector<std::string> push_lex;
    int cur_tok;
    std::string cur_lex;

    int nextToken();
    int peekToken(int n = 1);
    void pushBack(int tok, const std::string& lex);

    void syntaxError(const std::string& message);
    void lexicalError();

    // Синтаксические правила
    void program();
    void topDecl();
    void funcDecl();
    void constDecl();
    void varDecl();
    void params();
    void block();
    void blockItems();
    void stmt();
    void whileStmt();
    void returnStmt();
    void expr();
    void rel();
    void add();
    void mul();
    void bitOr();
    void bitXor();
    void bitAnd();
    void prim();

public:
    Diagram(Scanner* scanner);
    void parseProgram();
};