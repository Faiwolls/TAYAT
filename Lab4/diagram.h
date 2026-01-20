// diagram.h
#pragma once
#include "scanner.h"
#include "defines.h"
#include "tree.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_map>
class SyntaxError : public std::runtime_error {
public:
    SyntaxError(const std::string& message) : std::runtime_error(message) {}
};
class Diagram {
private:
    Scanner* sc;
    Tree* tree;
    std::vector<int> push_tok;
    std::vector<std::string> push_lex;
    int cur_tok;
    std::string cur_lex;
    bool inFunction;
    dataType currentReturnType;
    std::string currentFuncName;
    dataType currentFuncReturnType;

    // Хранение информации о параметрах функций
    std::unordered_map<std::string, std::vector<dataDetails>> functionParams;

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

    dataDetails expr();
    dataDetails rel();
    dataDetails add();
    dataDetails mul();
    dataDetails bitOr();
    dataDetails bitXor();
    dataDetails bitAnd();
    dataDetails shift();
    dataDetails prim();

public:
    Diagram(Scanner* scanner);
    void parseProgram();
};