//diagram.cpp
#include "diagram.h"
#include <iostream>
#include <sstream>

Diagram::Diagram(Scanner* scanner) : sc(scanner), cur_tok(0), cur_lex(),
current_decl_type(TYPE_UNDEFINED), current_arr_elem_count(0) {
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

void Diagram::semError(const std::string& msg) {
    std::pair<int, int> lc = sc->getLineCol();
    //Tree::SemError(msg, cur_lex, lc.first, lc.second);
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

int Diagram::peekToken() {
    int t = nextToken();
    pushBack(t, cur_lex);
    return t;
}

void Diagram::pushBack(int tok, const std::string& lex) {
    push_tok.push_back(tok);
    push_lex.push_back(lex);
}

void Diagram::ParseProgram() {
    //SemNode* root_node = new SemNode();
    //root_node->id = "<глобальная область видимости>";
    //root_node->DataType = TYPE_SCOPE;
    //root_node->line = 0;
    //root_node->col = 0;

    //Tree* root_tree = new Tree(root_node, nullptr);
    //Tree::SetCur(root_tree);

    Program();

    int t = peekToken();
    if (t != T_END) {
        synError("Лишний текст в конце программы");
    }

    //root_tree->Print();
}

void Diagram::Program() {
    int t = peekToken();
    while (t == KW_INT || t == KW_SHORT || t == KW_LONG ||
        t == KW_BOOL || t == KW_CONST) {
        TopDecl();
        t = peekToken();
    }
}

void Diagram::TopDecl() {
    int t = peekToken();
    if (t == KW_INT) {
        nextToken();
        t = peekToken();
        if (t == IDENT && cur_lex == "main") {
            nextToken();
            MainFunc();
        }
        else {
            current_decl_type = TYPE_INT;
            current_arr_elem_count = 0;
            pushBack(t, cur_lex);
            VarDecl();
        }
    }
    else if (t == KW_CONST) {
        nextToken();
        ConstDecl();
    }
    else {
        if (t == KW_SHORT) {
            current_decl_type = TYPE_SHORT_INT;
            current_arr_elem_count = 0;
        }
        else if (t == KW_LONG) {
            current_decl_type = TYPE_LONG_INT;
            current_arr_elem_count = 0;
        }
        else if (t == KW_BOOL) {
            current_decl_type = TYPE_BOOL;
            current_arr_elem_count = 0;
        }
        nextToken();
        VarDecl();
    }
}

void Diagram::MainFunc() {
    //Tree* saved_cur = Tree::Cur;
    int t = peekToken();
    if (t != LPAREN) synError("Ожидалась '(' после main");
    nextToken();
    t = peekToken();
    if (t != RPAREN) synError("Ожидалась ')' после '('");
    nextToken();
    Block();
    //Tree::SetCur(saved_cur);
}

void Diagram::VarDecl() {
    int t = peekToken();
    if (t != IDENT) {
        synError("Ожидался идентификатор в объявлении переменной");
    }
    nextToken();
    std::string name = cur_lex;
    std::pair<int, int> lc = sc->getLineCol();
    t = peekToken();

    if (t == LBRACKET) {
        nextToken();
        t = peekToken();
        if (t != CONST_DEC && t != CONST_HEX) {
            synError("Ожидалась константа размера массива после '['");
        }
        t = nextToken();
        int base = 10;
        if (t == CONST_HEX) base = 16;
        try {
            current_arr_elem_count = std::stoi(cur_lex, nullptr, base);
        }
        catch (const std::out_of_range& e) {
            semError("Размерность массива не может превышать диапазон типа int");
        }
        if (current_arr_elem_count <= 0) {
            semError("Размерность массива должна быть больше 0");
        }
        t = peekToken();
        if (t != RBRACKET) {
            synError("Ожидалась ']' после размера массива");
        }
        nextToken();
        //Tree* array_node = Tree::Cur->SemInclude(name, TYPE_ARRAY, lc.first, lc.second);
        //array_node->SemSetBasicType(array_node, current_decl_type);
        //array_node->SemSetArrElemCount(array_node, current_arr_elem_count);

        //for (int i = 0; i < current_arr_elem_count; i++) {
            //std::string elem_name = name + "_" + std::to_string(i);
            //Tree* elem_node = Tree::Cur->SemInclude(elem_name, current_decl_type, lc.first, lc.second);
            //elem_node->SemSetIndex(elem_node, i);
        //}

        t = peekToken();
        if (t == ASSIGN) {
            semError("Нельзя инициализировать массив при объявлении");
        }
    }
    else {
        current_arr_elem_count = 0;
        //Tree* var_node = Tree::Cur->SemInclude(name, current_decl_type, lc.first, lc.second);
        t = peekToken();
        if (t == ASSIGN) {
            nextToken();
            DATA_TYPE expr_type = Expr();
            if (expr_type != current_decl_type) {
                semError("Несоответствие типов при инициализации переменной '" + name + "'");
            }
        }
    }

    t = peekToken();
    while (t == COMMA) {
        nextToken();
        t = peekToken();
        if (t != IDENT) {
            synError("Ожидался идентификатор после ','");
        }
        nextToken();
        name = cur_lex;
        lc = sc->getLineCol();

        if (current_arr_elem_count > 0) {
            semError("Нельзя объявлять несколько массивов в одном объявлении");
        }

        //Tree* var_node = Tree::Cur->SemInclude(name, current_decl_type, lc.first, lc.second);

        t = peekToken();
        if (t == ASSIGN) {
            nextToken();
            DATA_TYPE expr_type = Expr();
            if (expr_type != current_decl_type) {
                semError("Несоответствие типов при инициализации переменной '" + name + "'");
            }
        }
        t = peekToken();
    }

    t = peekToken();
    if (t != SEMI) {
        synError("Ожидалась ';' в конце объявления переменных");
    }
    nextToken();
}

void Diagram::ConstDecl() {
    int t = peekToken();
    if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_BOOL) {
        synError("Ожидался тип данных после const");
    }
    if (t == KW_INT) {
        current_decl_type = TYPE_INT;
    }
    else if (t == KW_SHORT) {
        current_decl_type = TYPE_SHORT_INT;
    }
    else if (t == KW_LONG) {
        current_decl_type = TYPE_LONG_INT;
    }
    else if (t == KW_BOOL) {
        current_decl_type = TYPE_BOOL;
    }
    nextToken();

    t = peekToken();
    if (t == LBRACKET) {
        semError("Нельзя объявить константу-массив");
    }

    if (t != IDENT) {
        synError("Ожидался идентификатор константы");
    }
    nextToken();
    std::string name = cur_lex;
    std::pair<int, int> lc = sc->getLineCol();

    //Tree* const_node = Tree::Cur->SemInclude(name, current_decl_type, lc.first, lc.second);
    //const_node->SemSetConst(const_node, 1);

    t = peekToken();
    if (t != ASSIGN) {
        synError("Ожидалось '=' в определении константы");
    }
    nextToken();
    DATA_TYPE expr_type = Expr();
    if (expr_type != current_decl_type) {
        semError("Несоответствие типов при инициализации константы '" + name + "'");
    }

    t = peekToken();
    while (t == COMMA) {
        nextToken();
        t = peekToken();
        if (t != IDENT) {
            synError("Ожидался идентификатор константы после ','");
        }
        nextToken();
        name = cur_lex;
        lc = sc->getLineCol();

        //const_node = Tree::Cur->SemInclude(name, current_decl_type, lc.first, lc.second);
        //const_node->SemSetConst(const_node, 1);

        t = peekToken();
        if (t != ASSIGN) {
            synError("Ожидалось '=' в определении константы");
        }
        nextToken();
        expr_type = Expr();
        if (expr_type != current_decl_type) {
            semError("Несоответствие типов при инициализации константы '" + name + "'");
        }
        t = peekToken();
    }

    t = peekToken();
    if (t != SEMI) {
        synError("Ожидалась ';' в конце объявления констант");
    }
    nextToken();
}

void Diagram::Block() {
    int t = peekToken();
    if (t != LBRACE) synError("Ожидалась '{' для начала блока");
    auto lc = sc->getLineCol();

    //Tree* saved_cur = Tree::Cur;
    //Tree* newBlock = Tree::Cur->SemEnterBlock(lc.first, lc.second);

    t = nextToken();
    BlockItems();

    t = peekToken();
    if (t != RBRACE) synError("Ожидалась '}' для конца блока");

    //Tree::Cur->SemExitBlock();
    //Tree::SetCur(saved_cur);

    nextToken();
}

void Diagram::BlockItems() {
    int t = peekToken();
    while (t != RBRACE && t != T_END) {
        if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
            if (t == KW_INT) {
                current_decl_type = TYPE_INT;
            }
            else if (t == KW_SHORT) {
                current_decl_type = TYPE_SHORT_INT;
            }
            else if (t == KW_LONG) {
                current_decl_type = TYPE_LONG_INT;
            }
            else if (t == KW_BOOL) {
                current_decl_type = TYPE_BOOL;
            }
            nextToken();
            VarDecl();
        }
        else if (t == KW_CONST) {
            nextToken();
            ConstDecl();
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
        nextToken();
        return;
    }
    if (t == LBRACE) {
        Block();
        return;
    }
    if (t == KW_WHILE) {
        nextToken();
        WhileStmt();
        return;
    }
    if (t == IDENT) {
        t = nextToken();
        std::string name = cur_lex;
        std::pair<int, int> lc = sc->getLineCol();
        //Tree* node = Tree::Cur->SemGetVar(name, lc.first, lc.second);

        //if (node->n->FlagConst) {
        //    semError("Именованной константе может быть присвоено значение только при её объявлении");
        //}

        t = peekToken();
        if (t == LBRACKET) {
            //if (node->n->DataType != TYPE_ARRAY) {
            //    semError("Операция индексирования ([]) применима только к идентификаторам, объявленным как массив");
            //}
            nextToken();
            t = peekToken();
            if (t != CONST_DEC && t != CONST_HEX) {
                synError("Ожидалась константа после '['");
            }
            t = nextToken();
            int index;
            int base = 10;
            if (t == CONST_HEX) base = 16;
            try {
                index = std::stoi(cur_lex, nullptr, base);
            }
            catch (const std::out_of_range& e) {
                semError("Индекс при обращении к массиву не может превышать диапазон типа int");
            }
            //if ((index < 0) || (index >= node->n->ArrElemCount)) {
            //    semError("Индекс при обращении к массиву должен быть больше или равен 0 и меньше указанного при объявлении размера");
            //}
            t = peekToken();
            if (t != RBRACKET) synError("Ожидалась ']' после константы");
            name = (name + "_" + std::to_string(index));
            nextToken();
            t = peekToken();
        }
        else {
            //if (node->n->DataType == TYPE_ARRAY) {
            //    semError("Нельзя использовать массив целиком в качестве операнда");
            //}
        }

        if (t == ASSIGN) {
            nextToken();
            DATA_TYPE expr_type = Expr();

            //DATA_TYPE varType = node->n->DataType;
            //if (varType == TYPE_ARRAY) {
            //    varType = node->n->BasicType;
            //}

            //bool node_is_valid = (varType == TYPE_INT || varType == TYPE_SHORT_INT ||
                //varType == TYPE_LONG_INT || varType == TYPE_BOOL);
            //bool expr_is_valid = (expr_type == TYPE_INT || expr_type == TYPE_SHORT_INT ||
                //expr_type == TYPE_LONG_INT || expr_type == TYPE_BOOL);

            //if (!(node_is_valid && expr_is_valid)) {
            //    semError("Несоответствие типов в операторе присваивания");
            //}

            t = peekToken();
            if (t != SEMI) synError("Ожидалась ';' после оператора присваивания");
            nextToken();
            return;
        }
        else {
            synError("Ожидалось '=' после идентификатора (присваивание)");
        }
    }
    synError("Неизвестная форма оператора");
}

void Diagram::WhileStmt() {
    int t = peekToken();
    if (t != LPAREN) synError("Ожидалась '(' после while");
    nextToken();

    DATA_TYPE cond = Expr();
    if (cond != TYPE_BOOL && cond != TYPE_INT && cond != TYPE_SHORT_INT && cond != TYPE_LONG_INT) {
        semError("Тип условия в цикле while должен быть целочисленным или bool");
    }

    t = peekToken();
    if (t != RPAREN) synError("Ожидалась ')' после выражения");
    nextToken();

    Stmt();
}

DATA_TYPE Diagram::Expr() {
    DATA_TYPE left = BitOr();
    int t = peekToken();
    while (t == EQ || t == NEQ) {
        nextToken();
        DATA_TYPE right = BitOr();

        bool is_left_valid = (left == TYPE_INT || left == TYPE_SHORT_INT ||
            left == TYPE_LONG_INT || left == TYPE_BOOL);
        bool is_right_valid = (right == TYPE_INT || right == TYPE_SHORT_INT ||
            right == TYPE_LONG_INT || right == TYPE_BOOL);

        if (!(is_left_valid && is_right_valid)) {
            semError("Операнды для '=='/ '!=' должны быть совместимых типов");
        }

        if (left == TYPE_BOOL && right != TYPE_BOOL) {
            semError("Невозможно сравнить bool с не-bool типом");
        }

        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

DATA_TYPE Diagram::BitOr() {
    DATA_TYPE left = BitXor();
    int t = peekToken();
    while (t == BIT_OR) {
        nextToken();
        DATA_TYPE right = BitXor();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT ||
            left == TYPE_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT ||
            right == TYPE_LONG_INT);

        if (!(is_left_int && is_right_int)) {
            semError("Операнды для '|' должны быть целыми");
        }

        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

DATA_TYPE Diagram::BitXor() {
    DATA_TYPE left = BitAnd();
    int t = peekToken();
    while (t == BIT_XOR) {
        nextToken();
        DATA_TYPE right = BitAnd();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT ||
            left == TYPE_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT ||
            right == TYPE_LONG_INT);

        if (!(is_left_int && is_right_int)) {
            semError("Операнды для '^' должны быть целыми");
        }

        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

DATA_TYPE Diagram::BitAnd() {
    DATA_TYPE left = Rel();
    int t = peekToken();
    while (t == BIT_AND) {
        nextToken();
        DATA_TYPE right = Rel();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT ||
            left == TYPE_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT ||
            right == TYPE_LONG_INT);

        if (!(is_left_int && is_right_int)) {
            semError("Операнды для '&' должны быть целыми");
        }

        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

DATA_TYPE Diagram::Rel() {
    DATA_TYPE left = Add();
    int t = peekToken();
    while (t == LT || t == LE || t == GT || t == GE) {
        nextToken();
        DATA_TYPE right = Add();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT ||
            left == TYPE_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT ||
            right == TYPE_LONG_INT);

        if (!(is_left_int && is_right_int)) {
            semError("Операнды для '<, <=, >, >=' должны быть целыми");
        }

        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

DATA_TYPE Diagram::Add() {
    DATA_TYPE left = Mul();
    int t = peekToken();
    while (t == PLUS || t == MINUS) {
        nextToken();
        DATA_TYPE right = Mul();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT ||
            left == TYPE_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT ||
            right == TYPE_LONG_INT);

        if (!(is_left_int && is_right_int)) {
            semError("Операнды для '+'/'-' должны быть целыми");
        }

        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

DATA_TYPE Diagram::Mul() {
    DATA_TYPE left = Prim();
    int t = peekToken();
    while (t == MULT || t == DIV || t == MOD) {
        nextToken();
        DATA_TYPE right = Prim();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT ||
            left == TYPE_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT ||
            right == TYPE_LONG_INT);

        if (!(is_left_int && is_right_int)) {
            semError("Операнды для '*', '/', '%' должны быть целыми");
        }

        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

DATA_TYPE Diagram::Prim() {
    int t = peekToken();
    if (t == KW_TRUE || t == KW_FALSE) {
        nextToken();
        return TYPE_BOOL;
    }
    if (t == MINUS) {
        nextToken();
        t = peekToken();
        if (t == CONST_DEC || t == CONST_HEX) {
            nextToken();
            DATA_TYPE const_type = TYPE_SHORT_INT;
            try {
                long long val = std::stoll(cur_lex, nullptr, 0);
                if (val > 32767 || val < -32768) const_type = TYPE_INT;
                if (val > 2147483647LL || val < -2147483648LL) const_type = TYPE_LONG_INT;
            }
            catch (...) {}
            return const_type;
        }
        else {
            pushBack(MINUS, "-");
            nextToken();
            t = peekToken();
            if (t != LPAREN) synError("Ожидалась константа или выражение в скобках после '-'");
            nextToken();
            DATA_TYPE dt = Expr();
            t = peekToken();
            if (t != RPAREN) synError("Ожидался ')' после выражения");
            nextToken();
            return dt;
        }
    }
    if (t == CONST_DEC || t == CONST_HEX) {
        nextToken();
        DATA_TYPE const_type = TYPE_SHORT_INT;
        try {
            long long val = std::stoll(cur_lex, nullptr, 0);
            if (val > 32767 || val < -32768) const_type = TYPE_INT;
            if (val > 2147483647LL || val < -2147483648LL) const_type = TYPE_LONG_INT;
        }
        catch (...) {}
        return const_type;
    }
    if (t == LPAREN) {
        nextToken();
        DATA_TYPE dt = Expr();
        t = peekToken();
        if (t != RPAREN) synError("Ожидалась ')' после выражения");
        nextToken();
        return dt;
    }
    if (t == IDENT) {
        nextToken();
        std::string name = cur_lex;
        std::pair<int, int> lc = sc->getLineCol();
        //Tree* node = Tree::Cur->SemGetVar(name, lc.first, lc.second);

        t = peekToken();
        if (t == LBRACKET) {
            //if (node->n->DataType != TYPE_ARRAY) {
            //  semError("Операция индексирования ([]) применима только к массивам");
            //}
            nextToken();
            t = peekToken();
            if (t == CONST_DEC || t == CONST_HEX) {
                t = nextToken();
                int index;
                int base = 10;
                if (t == CONST_HEX) base = 16;
                try {
                    index = std::stoi(cur_lex, nullptr, base);
                }
                catch (const std::out_of_range& e) {
                    semError("Индекс при обращении к массиву не может превышать диапазон типа int");
                }
                //if ((index < 0) || (index >= node->n->ArrElemCount)) {
                //    semError("Индекс при обращении к массиву должен быть больше или равен 0 и меньше указанного при объявлении размера");
                //}
                t = peekToken();
                if (t != RBRACKET) synError("Ожидалась ']' после константы");
                nextToken();
                name = (name + "_" + std::to_string(index));
                //node = Tree::Cur->SemGetVar(name, sc->getLineCol().first, sc->getLineCol().second);
                //return node->n->DataType;
            }
            else {
                synError("Ожидалась константа после '['");
            }
        }
        else {
            //if (node->n->DataType == TYPE_ARRAY) {
            //    semError("Нельзя использовать массив целиком в качестве операнда");
            //}
            //return node->n->DataType;
        }
    }
    synError("Ожидалось первичное выражение (IDENT, константа или скобки)");
    return TYPE_INT;
}
