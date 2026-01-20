// diagram.cpp
#include "diagram.h"
#include <iostream>
#include <sstream>
#include <vector>

Diagram::Diagram(Scanner* scanner) : sc(scanner), cur_tok(0), cur_lex() {
    push_tok.clear();
    push_lex.clear();
}

void Diagram::synError(const std::string& msg) {
    std::pair<int, int> lc = sc->getLineCol();
    std::cerr << "Синтаксическая ошибка: " << msg;
    if (!cur_lex.empty()) std::cerr << " (около '" << cur_lex << "')";
    std::cerr << std::endl << "(строка " << lc.first << ":" << lc.second << ")" << std::endl;
    std::exit(1);
}

void Diagram::lexError() {
    std::pair<int, int> lc = sc->getLineCol();
    std::cerr << "Лексическая ошибка: неизвестная лексема '" << cur_lex << "'";
    std::cerr << std::endl << "(строка " << lc.first << ":" << lc.second << ")" << std::endl;
    std::exit(1);
}

int Diagram::nextToken() {
    if (!push_tok.empty()) {
        int t = push_tok.back();
        push_tok.pop_back();
        std::string lx = push_lex.back();
        push_lex.pop_back();
        cur_tok = t;
        cur_lex = lx;
        if (cur_tok == T_ERR) lexError();
        return cur_tok;
    }
    std::string lex;
    cur_tok = sc->getNextLex(lex);
    cur_lex = lex;
    if (cur_tok == T_ERR) lexError();
    return cur_tok;
}

int Diagram::peekToken(int n = 1) {
    std::vector<int> buf;
    std::vector<std::string> str_buf;
    for (int i = 0; i < n; i++) {
        int t = nextToken();
        buf.push_back(t);
        str_buf.push_back(cur_lex);
    }
    int res;
    res = buf.back();

    for (int i = 0; i < n; i++) {
        int t = buf.back();
        std::string l = str_buf.back();
        pushBack(t, l);
        if (!buf.empty()) buf.pop_back();
        if (!str_buf.empty()) str_buf.pop_back();
    }
    
    return res;
}

void Diagram::pushBack(int tok, const std::string& lex) {
    push_tok.push_back(tok);
    push_lex.push_back(lex);
}

void Diagram::ParseProgram() {
    Program();
    //int t = peekToken();
    //if (t != T_END) {
    //    synError("Лишний текст в конце программы");
    //}
}

void Diagram::Program() {
    int t;
    do {
        t = peekToken();
        if (t == T_END) { std::cout << "Конец файла" << std::endl; break; }
        TopDecl();
    } while (true);
}

void Diagram::TopDecl() {
    int t = peekToken();

    if (t == KW_CONST) {
        //Описание константы
    }

    if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
        //Описание функции или переменной
        int t2 = peekToken(2);
        if (t2 != IDENT) { synError("Ожидался идентификатор после типа"); }

        int t3 = peekToken(3);
        if (t3 == LPAREN) {//Функция
            FuncDecl();
        }
        else { //Данные
            std::cout << "DEBUG: Найдена переменная" << std::endl;
            //Var();
        }
    }
    else {
        synError("Ожидался тип в начале объявления");
    }
}

void Diagram::FuncDecl() {
    nextToken(); //Тип
    nextToken(); //Имя функции
    nextToken(); // '('

    int t = peekToken();
    if (t != RPAREN) {
        Params();
    }

    t = peekToken();
    if (t != RPAREN) {
        synError("Ожидалась ')' после параметров функции");
    }
    nextToken(); // ')'

    Block(); // тело функции
}

void Diagram::Params() {
    int t = peekToken();
    if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
        nextToken(); // тип параметра
        t = peekToken();
        if (t != IDENT) {
            synError("Ожидался идентификатор параметра");
        }
        nextToken(); // имя параметра

        t = peekToken();
        while (t == COMMA) {
            nextToken(); // запятая
            t = peekToken();
            if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_BOOL) {
                synError("Ожидался тип параметра после ','");
            }
            nextToken(); // тип параметра
            t = peekToken();
            if (t != IDENT) {
                synError("Ожидался идентификатор параметра");
            }
            nextToken(); // имя параметра
            t = peekToken();
        }
    }
}

void Diagram::Block() {
    int t = peekToken();
    if (t != LBRACE) {
        synError("Ожидалась '{' для начала блока");
    }
    nextToken(); // '{'

    BlockItems();

    t = peekToken();
    if (t != RBRACE) {
        synError("Ожидалась '}' для конца блока");
    }
    nextToken(); // '}'
}

void Diagram::BlockItems() {
    int t = peekToken();
    while (t != RBRACE && t != T_END) {
        if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
            
            nextToken(); // тип
            while (true) {
                t = peekToken();
                if (t != IDENT) {
                    synError("Ожидался идентификатор переменной");
                }
                nextToken(); // имя

                t = peekToken();
                if (t == LBRACKET) {
                    nextToken(); // '['
                    Expr(); // размер массива (любое выражение)
                    t = peekToken();
                    if (t != RBRACKET) {
                        synError("Ожидалась ']' после размера массива");
                    }
                    nextToken(); // ']'
                }

                t = peekToken();
                if (t == ASSIGN) {
                    nextToken(); // '='
                    Expr(); // инициализация
                }

                t = peekToken();
                if (t != COMMA) {
                    break;
                }
                nextToken(); // ','
            }

            t = peekToken();
            if (t != SEMI) {
                synError("Ожидалась ';' после объявления переменной");
            }
            nextToken(); // ';'
        }
        else {
            Stmt();
        }
        t = peekToken();
    }
}

void Diagram::Stmt() {
    int t = peekToken();
    if (t == SEMI) {
        nextToken(); // пустой оператор
        return;
    }
    if (t == LBRACE) {
        Block(); // составной оператор
        return;
    }
    if (t == KW_WHILE) {
        nextToken(); // while
        WhileStmt();
        return;
    }
    if (t == KW_RETURN) {
        nextToken(); // return
        ReturnStmt();
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
                Expr(); // первый аргумент
                t = peekToken();
                while (t == COMMA) {
                    nextToken(); // ','
                    Expr(); // следующий аргумент
                    t = peekToken();
                }
            }
            if (t != RPAREN) {
                synError("Ожидалась ')' после аргументов функции");
            }
            nextToken(); // ')'
            t = peekToken();
            if (t != SEMI) {
                synError("Ожидалась ';' после вызова функции");
            }
            nextToken(); // ';'
            return;
        }
        else if (t == LBRACKET) {
            // Индексация массива
            nextToken(); // '['
            Expr(); // индекс
            t = peekToken();
            if (t != RBRACKET) {
                synError("Ожидалась ']' после индекса");
            }
            nextToken(); // ']'
            t = peekToken();
        }

        if (t == ASSIGN) {
            // Присваивание
            nextToken(); // '='
            Expr(); // выражение
            t = peekToken();
            if (t != SEMI) {
                synError("Ожидалась ';' после оператора присваивания");
            }
            nextToken(); // ';'
            return;
        }
        else {
            synError("Ожидалось '=' или '(' после идентификатора");
        }
    }
    synError("Неизвестная форма оператора");
}

void Diagram::WhileStmt() {
    int t = peekToken();
    if (t != LPAREN) {
        synError("Ожидалась '(' после while");
    }
    nextToken(); // '('

    Expr(); // условие

    t = peekToken();
    if (t != RPAREN) {
        synError("Ожидалась ')' после условия");
    }
    nextToken(); // ')'

    Stmt(); // тело цикла
}

void Diagram::ReturnStmt() {
    int t = peekToken();
    if (t != SEMI) {
        Expr(); // возвращаемое значение
    }
    t = peekToken();
    if (t != SEMI) {
        synError("Ожидалась ';' после оператора return");
    }
    nextToken(); // ';'
}

void Diagram::Expr() {
    BitOr();
    int t = peekToken();
    while (t == EQ || t == NEQ) {
        nextToken(); // оператор сравнения
        BitOr();
        t = peekToken();
    }
}

void Diagram::BitOr() {
    BitXor();
    int t = peekToken();
    while (t == BIT_OR) {
        nextToken(); // '|'
        BitXor();
        t = peekToken();
    }
}

void Diagram::BitXor() {
    BitAnd();
    int t = peekToken();
    while (t == BIT_XOR) {
        nextToken(); // '^'
        BitAnd();
        t = peekToken();
    }
}

void Diagram::BitAnd() {
    Rel();
    int t = peekToken();
    while (t == BIT_AND) {
        nextToken(); // '&'
        Rel();
        t = peekToken();
    }
}

void Diagram::Rel() {
    Add();
    int t = peekToken();
    while (t == LT || t == LE || t == GT || t == GE) {
        nextToken(); // оператор сравнения
        Add();
        t = peekToken();
    }
}

void Diagram::Add() {
    Mul();
    int t = peekToken();
    while (t == PLUS || t == MINUS) {
        nextToken(); // '+' или '-'
        Mul();
        t = peekToken();
    }
}

void Diagram::Mul() {
    Prim();
    int t = peekToken();
    while (t == MULT || t == DIV || t == MOD) {
        nextToken(); // '*' '/' или '%'
        Prim();
        t = peekToken();
    }
}

void Diagram::Prim() {
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
        Expr();
        t = peekToken();
        if (t != RPAREN) {
            synError("Ожидалась ')' после выражения");
        }
        nextToken(); // ')'
        return;
    }
    if (t == IDENT) {
        nextToken(); // идентификатор
        t = peekToken();
        if (t == LBRACKET) {
            // Индексация массива
            nextToken(); // '['
            Expr(); // индекс
            t = peekToken();
            if (t != RBRACKET) {
                synError("Ожидалась ']' после индекса");
            }
            nextToken(); // ']'
            return;
        }
        else if (t == LPAREN) {
            // Вызов функции
            nextToken(); // '('
            t = peekToken();
            if (t != RPAREN) {
                Expr(); // первый аргумент
                t = peekToken();
                while (t == COMMA) {
                    nextToken(); // ','
                    Expr(); // следующий аргумент
                    t = peekToken();
                }
            }
            if (t != RPAREN) {
                synError("Ожидалась ')' после аргументов");
            }
            nextToken(); // ')'
            return;
        }
        return;
    }
    if (t == MINUS) {
        nextToken(); // унарный минус
        Prim();
        return;
    }
    synError("Ожидалось первичное выражение");
}