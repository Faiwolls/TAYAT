#include <iostream>
#include <string>
#include <map>
#include <Windows.h>
#include "defines.h"
#include "scanner.h"
#include "diagram.h"

std::map <int, std::string> lex_type_names{
    {10,    "IDENT"},

    { 1,     "KW_SHORT" },
    { 2,     "KW_INT" },
    { 3,     "KW_LONG" },
    { 4,     "KW_BOOL" },
    { 5,     "KW_WHILE" },
    { 6,     "KW_TRUE" },
    { 7,     "KW_FALSE" },
    { 8,     "KW_CONST" },

    { 20,    "CONST_DEC" },
    { 21,    "CONST_HEX" },
    { 22,    "CONST_BOOL" },
    { 23,    "CONST_DEC_LONG" },
    { 24,    "CONST_HEX_LONG" },

    { 30,    "SEMI" },
    { 31,    "COMMA" },
    { 32,    "LPAREN" },
    { 33,    "RPAREN" },
    { 34,    "LBRACE" },
    { 35,    "RBRACE" },
    { 36,    "LBRACKET" },
    { 37,    "RBRACKET" },

    { 40,    "EQ" },
    { 41,    "NEQ" },
    { 42,    "LE" },
    { 43,    "GE" },
    { 44,    "LT" },
    { 45,    "GT" },
    { 46,    "ASSIGN" },

    { 47,    "PLUS" },
    { 48,    "MINUS" },
    { 49,    "MULT" },
    { 50,    "DIV" },
    { 51,    "MOD" },

    { 52,    "BIT_AND" },
    { 53,    "BIT_OR" },
    { 54,    "BIT_XOR" },
    { 57,    "BIT_NOT" },

    { 100,   "T_END" },
    { 200,   "T_ERR" }
};

std::string get_lex_name_by_type(int type) {
    return lex_type_names[type];
}


int main(int argc, char** argv) {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    std::string fname = "input1.txt";
    if (argc > 1) fname = argv[1];

    Scanner sc;
    if (!sc.loadFile(fname)) {
        std::cerr << "Невозможно открыть " << fname << std::endl;
        return -1;
    }

    int xx = 555L;

    Diagram diagram(&sc);

    try {
        diagram.parseProgram();
        std::cout << "Ошибок не обнаружено" << std::endl;
    }
    catch (...) {}

    std::cout << "\nПрограмма завершена. Нажмите Enter для выхода...";
    std::cin.ignore();

    return 0;
}