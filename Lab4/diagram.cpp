// diagram.cpp
#include "diagram.h"
#include <iostream>
#include <sstream>

Diagram::Diagram(Scanner* scanner) : sc(scanner), cur_tok(0), cur_lex("") {
    push_tok.clear();
    push_lex.clear();
}

void Diagram::syntaxError(const std::string& message) {
    std::pair<int, int> pos = sc->getLineCol();
    std::ostringstream oss;
    oss << "Syntax Error at line " << pos.first << ", position " << pos.second
        << ": " << message;
    if (!cur_lex.empty()) {
        oss << " (found: '" << cur_lex << "')";
    }
    throw SyntaxError(oss.str());
}

void Diagram::lexicalError() {
    std::pair<int, int> pos = sc->getLineCol();
    std::ostringstream oss;
    oss << "Lexical Error at line " << pos.first << ", position " << pos.second
        << ": unknown token '" << cur_lex << "'";
    throw SyntaxError(oss.str());
}

int Diagram::nextToken() {
    if (!push_tok.empty()) {
        int t = push_tok.back();
        push_tok.pop_back();
        std::string lx = push_lex.back();
        push_lex.pop_back();
        cur_tok = t;
        cur_lex = lx;
        if (cur_tok == T_ERR) lexicalError();
        return cur_tok;
    }

    std::string lex;
    cur_tok = sc->getNextLex(lex);
    cur_lex = lex;
    if (cur_tok == T_ERR) lexicalError();
    return cur_tok;
}

int Diagram::peekToken(int n) {
    std::vector<int> buf;
    std::vector<std::string> str_buf;

    for (int i = 0; i < n; i++) {
        int t = nextToken();
        buf.push_back(t);
        str_buf.push_back(cur_lex);
    }

    int res = buf.back();

    for (int i = 0; i < n; i++) {
        int t = buf.back();
        std::string l = str_buf.back();
        pushBack(t, l);
        buf.pop_back();
        str_buf.pop_back();
    }

    return res;
}

void Diagram::pushBack(int tok, const std::string& lex) {
    push_tok.push_back(tok);
    push_lex.push_back(lex);
}

void Diagram::parseProgram() {
    try {
        std::cout << "=== Syntax Analysis ===" << std::endl;
        program();
        std::cout << "Syntax analysis completed: no errors found." << std::endl;
        std::cout << "=======================" << std::endl;
    }
    catch (const SyntaxError& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

void Diagram::program() {
    while (true) {
        int t = peekToken();
        if (t == T_END) {
            break;
        }
        topDecl();
    }
}

void Diagram::topDecl() {
    int t = peekToken();

    if (t == KW_CONST) {
        nextToken(); // KW_CONST
        t = peekToken();
        if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_BOOL) {
            syntaxError("Expected type after 'const'");
        }
        nextToken(); // тип
        t = peekToken();
        if (t != IDENT) {
            syntaxError("Expected identifier after type in constant declaration");
        }
        nextToken(); // IDENT
        t = peekToken();
        if (t != ASSIGN) {
            syntaxError("Expected '=' after identifier in constant declaration");
        }
        nextToken(); // ASSIGN
        expr();
        t = peekToken();
        if (t != SEMI) {
            syntaxError("Expected ';' at the end of constant declaration");
        }
        nextToken(); // SEMI
    }
    else if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
        if (peekToken(3) == LPAREN) {
            funcDecl();
        }
        else {
            varDecl();
        }
    }
    else {
        syntaxError("Expected declaration at global scope");
    }
}

void Diagram::funcDecl() {
    nextToken(); // тип возвращаемого значения
    int t = peekToken();
    if (t != IDENT) {
        syntaxError("Expected function name after return type");
    }
    nextToken(); // IDENT

    t = peekToken();
    if (t != LPAREN) {
        syntaxError("Expected '(' after function name");
    }
    nextToken(); // LPAREN

    t = peekToken();
    if (t != RPAREN) {
        params();
    }

    t = peekToken();
    if (t != RPAREN) {
        syntaxError("Expected ')' after function parameters");
    }
    nextToken(); // RPAREN

    block();
}

void Diagram::constDecl() {
    nextToken(); // KW_CONST

    // Проверяем тип константы
    int t = peekToken();
    if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_BOOL) {
        syntaxError("Ожидался тип после ключевого слова 'const'");
    }
    nextToken(); // тип

    t = peekToken();
    if (t != IDENT) {
        syntaxError("Ожидался идентификатор после типа в объявлении константы");
    }
    nextToken(); // идентификатор

    t = peekToken();
    if (t != ASSIGN) {
        syntaxError("Ожидался символ '=' после идентификатора в объявлении константы");
    }
    nextToken(); // '='

    expr(); // выражение для инициализации константы

    t = peekToken();
    if (t != SEMI) {
        syntaxError("Ожидалась ';' в конце объявления константы");
    }
    nextToken(); // ';'
}

void Diagram::varDecl() {
    nextToken(); // тип переменной

    while (true) {
        int t = peekToken();
        if (t != IDENT) {
            syntaxError("Ожидался идентификатор переменной");
        }
        nextToken(); // имя переменной

        // Проверка на массивы (необязательно по требованиям, но может быть)
        t = peekToken();
        if (t == LBRACKET) {
            nextToken(); // '['
            expr(); // размер массива
            t = peekToken();
            if (t != RBRACKET) {
                syntaxError("Ожидалась ']' после размера массива");
            }
            nextToken(); // ']'
        }

        // Проверка на инициализацию
        t = peekToken();
        if (t == ASSIGN) {
            nextToken(); // '='
            expr(); // значение для инициализации
        }

        t = peekToken();
        if (t != COMMA) {
            break; // больше нет переменных для объявления
        }
        nextToken(); // ','
    }

    int t = peekToken();
    if (t != SEMI) {
        syntaxError("Ожидалась ';' в конце объявления переменной");
    }
    nextToken(); // ';'
}

void Diagram::params() {
    while (true) {
        int t = peekToken();
        if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_BOOL) {
            syntaxError("Expected parameter type");
        }
        nextToken(); // тип

        t = peekToken();
        if (t != IDENT) {
            syntaxError("Expected parameter name");
        }
        nextToken(); // имя

        t = peekToken();
        if (t != COMMA) {
            break;
        }
        nextToken(); // COMMA
    }
}

void Diagram::block() {
    int t = peekToken();
    if (t != LBRACE) {
        syntaxError("Expected '{' to start block");
    }
    nextToken(); // LBRACE

    blockItems();

    t = peekToken();
    if (t != RBRACE) {
        syntaxError("Expected '}' to end block");
    }
    nextToken(); // RBRACE
}

void Diagram::blockItems() {
    while (true) {
        int t = peekToken();
        if (t == RBRACE || t == T_END) {
            break;
        }

        if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
            nextToken(); // тип

            while (true) {
                t = peekToken();
                if (t != IDENT) {
                    syntaxError("Expected variable name");
                }
                nextToken(); // имя

                t = peekToken();
                if (t == ASSIGN) {
                    nextToken(); // ASSIGN
                    expr();
                }

                t = peekToken();
                if (t != COMMA) {
                    break;
                }
                nextToken(); // COMMA
            }

            t = peekToken();
            if (t != SEMI) {
                syntaxError("Expected ';' after variable declaration");
            }
            nextToken(); // SEMI
        }
        else {
            stmt();
        }
    }
}

void Diagram::stmt() {
    int t = peekToken();

    if (t == SEMI) {
        nextToken(); // пустой оператор
        return;
    }

    if (t == LBRACE) {
        block(); // составной оператор
        return;
    }

    if (t == KW_WHILE) {
        nextToken(); // KW_WHILE
        whileStmt();
        return;
    }

    if (t == KW_RETURN) {
        nextToken(); // KW_RETURN
        returnStmt();
        return;
    }

    if (t == IDENT) {
        nextToken(); // IDENT

        t = peekToken();
        if (t == LPAREN) {
            // Вызов функции
            nextToken(); // LPAREN

            t = peekToken();
            if (t != RPAREN) {
                expr(); // первый аргумент

                t = peekToken();
                while (t == COMMA) {
                    nextToken(); // COMMA
                    expr(); // следующий аргумент
                    t = peekToken();
                }
            }

            if (t != RPAREN) {
                syntaxError("Expected ')' after function arguments");
            }
            nextToken(); // RPAREN

            t = peekToken();
            if (t != SEMI) {
                syntaxError("Expected ';' after function call");
            }
            nextToken(); // SEMI
            return;
        }

        if (t == LBRACKET) {
            // Индексация массива
            nextToken(); // LBRACKET
            expr(); // индекс

            t = peekToken();
            if (t != RBRACKET) {
                syntaxError("Expected ']' after array index");
            }
            nextToken(); // RBRACKET

            t = peekToken();
            if (t != ASSIGN) {
                syntaxError("Expected '=' after array index");
            }
        }

        if (t == ASSIGN || peekToken() == ASSIGN) {
            nextToken(); // ASSIGN
            expr();
            t = peekToken();
            if (t != SEMI) {
                syntaxError("Expected ';' after assignment");
            }
            nextToken(); // SEMI
            return;
        }

        syntaxError("Expected '(' for function call or '=' for assignment after identifier");
    }

    syntaxError("Unexpected token in statement");
}

void Diagram::whileStmt() {
    int t = peekToken();
    if (t != LPAREN) {
        syntaxError("Expected '(' after 'while'");
    }
    nextToken(); // LPAREN

    expr(); // условие

    t = peekToken();
    if (t != RPAREN) {
        syntaxError("Expected ')' after while condition");
    }
    nextToken(); // RPAREN

    stmt(); // тело цикла
}

void Diagram::returnStmt() {
    int t = peekToken();
    if (t != SEMI) {
        expr(); // возвращаемое значение
    }

    t = peekToken();
    if (t != SEMI) {
        syntaxError("Expected ';' after return statement");
    }
    nextToken(); // SEMI
}

void Diagram::expr() {
    rel();
    int t = peekToken();
    while (t == EQ || t == NEQ) {
        nextToken(); // оператор сравнения
        rel();
        t = peekToken();
    }
}

void Diagram::rel() {
    add();
    int t = peekToken();
    while (t == LT || t == LE || t == GT || t == GE) {
        nextToken(); // оператор отношения
        add();
        t = peekToken();
    }
}

void Diagram::add() {
    mul();
    int t = peekToken();
    while (t == PLUS || t == MINUS) {
        nextToken(); // '+' или '-'
        mul();
        t = peekToken();
    }
}

void Diagram::mul() {
    bitOr();
    int t = peekToken();
    while (t == MULT || t == DIV || t == MOD) {
        nextToken(); // '*' '/' или '%'
        bitOr();
        t = peekToken();
    }
}

void Diagram::bitOr() {
    bitXor();
    int t = peekToken();
    while (t == BIT_OR) {
        nextToken(); // '|'
        bitXor();
        t = peekToken();
    }
}

void Diagram::bitXor() {
    bitAnd();
    int t = peekToken();
    while (t == BIT_XOR) {
        nextToken(); // '^'
        bitAnd();
        t = peekToken();
    }
}

void Diagram::bitAnd() {
    prim();
    int t = peekToken();
    while (t == BIT_AND) {
        nextToken(); // '&'
        prim();
        t = peekToken();
    }
}

void Diagram::prim() {
    int t = peekToken();

    if (t == KW_TRUE || t == KW_FALSE) {
        nextToken(); // логическая константа
        return;
    }

    if (t == CONST_DEC || t == CONST_HEX) {
        nextToken(); // числовая константа
        return;
    }

    if (t == LPAREN) {
        nextToken(); // '('
        expr();

        t = peekToken();
        if (t != RPAREN) {
            syntaxError("Expected ')' after expression");
        }
        nextToken(); // ')'
        return;
    }

    if (t == IDENT) {
        nextToken(); // идентификатор

        t = peekToken();
        if (t == LPAREN) {
            // Вызов функции
            nextToken(); // '('

            t = peekToken();
            if (t != RPAREN) {
                expr(); // первый аргумент

                t = peekToken();
                while (t == COMMA) {
                    nextToken(); // ','
                    expr(); // следующий аргумент
                    t = peekToken();
                }
            }

            if (t != RPAREN) {
                syntaxError("Expected ')' after function arguments");
            }
            nextToken(); // ')'
            return;
        }

        if (t == LBRACKET) {
            // Индексация массива
            nextToken(); // '['
            expr(); // индекс

            t = peekToken();
            if (t != RBRACKET) {
                syntaxError("Expected ']' after array index");
            }
            nextToken(); // ']'
            return;
        }

        return;
    }

    if (t == MINUS || t == PLUS) {
        nextToken(); // унарный минус или плюс
        prim();
        return;
    }

    syntaxError("Expected primary expression");
}