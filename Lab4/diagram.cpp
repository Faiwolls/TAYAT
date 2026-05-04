// diagram.cpp
#include "diagram.h"
#include <iostream>
#include <sstream>
#include <functional>

class SemanticError : public std::exception {
public:
    SemanticError(const std::string& message) : msg(message) {}
    const char* what() const noexcept override { return msg.c_str(); }
private:
    std::string msg;
};

Diagram::Diagram(Scanner* scanner) : sc(scanner), cur_tok(0), cur_lex(""), inFunction(false), tree(new Tree()) {
    push_tok.clear();
    push_lex.clear();
}

void Diagram::syntaxError(const std::string& message) {
    std::pair<int, int> pos = sc->getLineCol();
    std::ostringstream oss;
    oss << "Синтаксическая ошибка на строке " << pos.first << ", позиция " << pos.second
        << ": " << message;
    if (!cur_lex.empty()) {
        oss << " (найдено: '" << cur_lex << "')";
    }
    throw SyntaxError(oss.str());
}

void Diagram::lexicalError() {
    std::pair<int, int> pos = sc->getLineCol();
    std::ostringstream oss;
    oss << "Лексическая ошибка на строке " << pos.first << ", позиция " << pos.second
        << ": неизвестный токен '" << cur_lex << "'";
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
        program();
        tree->print();
        std::cout << "===============================" << std::endl;
    }
    catch (const std::exception& e) {
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
        constDecl();
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
        syntaxError("Ожидалось объявление на глобальном уровне");
    }
}

void Diagram::funcDecl() {
    // Получаем тип возвращаемого значения
    int type_tok = nextToken();
    dataType retType;
    switch (type_tok) {
    case KW_INT: retType = t_int; break;
    case KW_SHORT: retType = t_short; break;
    case KW_LONG: retType = t_long; break;
    case KW_BOOL: retType = t_bool; break;
    default: syntaxError("Ожидался тип возвращаемого значения функции");
    }

    currentReturnType = retType;

    // Имя функции
    int t = peekToken();
    if (t != IDENT) {
        syntaxError("Ожидалось имя функции после типа возврата");
    }
    nextToken();
    std::string funcName = cur_lex;

    // Сохраняем информацию о функции
    currentFuncName = funcName;
    currentFuncReturnType = retType;
    std::vector<dataDetails> funcParams;

    t = peekToken();
    if (t != LPAREN) {
        syntaxError("Ожидался '(' после имени функции");
    }
    nextToken(); // LPAREN

    // Теперь добавляем функцию в дерево
    dataDetails funcDetails = { retType };
    try {
        Node* funcNode = tree->addSymbol(Func, funcName, funcDetails, false, sc->getLineCol().first, sc->getLineCol().second);
        // Сохраняем количество параметров
        funcNode->paramCount = funcParams.size();
    }
    catch (const std::runtime_error& e) {
        throw SemanticError(std::string("Семантическая ошибка: ") + e.what());
    }

    // Входим в область видимости функции
    tree->enterScope(sc->getLineCol().first, sc->getLineCol().second);

    // Обрабатываем параметры
    t = peekToken();
    if (t != RPAREN) {
        while (true) {
            t = peekToken();
            if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_BOOL) {
                syntaxError("Ожидался тип параметра");
            }

            // Получаем тип параметра
            nextToken();
            dataType paramType;
            switch (t) {
            case KW_INT: paramType = t_int; break;
            case KW_SHORT: paramType = t_short; break;
            case KW_LONG: paramType = t_long; break;
            case KW_BOOL: paramType = t_bool; break;
            }

            t = peekToken();
            if (t != IDENT) {
                syntaxError("Ожидалось имя параметра");
            }
            nextToken();
            std::string paramName = cur_lex;

            // Добавляем параметр в дерево
            dataDetails paramDetails = { paramType };
            funcParams.push_back(paramDetails);

            try {
                tree->addSymbol(Var, paramName, paramDetails, false, sc->getLineCol().first, sc->getLineCol().second);
            }
            catch (const std::runtime_error& e) {
                throw SemanticError(std::string("Семантическая ошибка: ") + e.what());
            }

            t = peekToken();
            if (t != COMMA) {
                break;
            }
            nextToken(); // COMMA
        }
    }

    // Сохраняем информацию о параметрах функции
    functionParams[funcName] = funcParams;

    t = peekToken();
    if (t != RPAREN) {
        syntaxError("Ожидался ')' после параметров функции");
    }
    nextToken(); // RPAREN

    // Устанавливаем флаг, что мы внутри функции
    bool prevInFunction = inFunction;
    inFunction = true;

    block();

    // Выходим из области видимости функции
    tree->exitScope();

    // Восстанавливаем предыдущее состояние флага
    inFunction = prevInFunction;
}

void Diagram::constDecl() {
    nextToken(); // KW_CONST

    // Получаем тип константы
    int t = peekToken();
    if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_BOOL) {
        syntaxError("Ожидался тип после ключевого слова 'const'");
    }
    nextToken();

    dataType constType;
    switch (t) {
    case KW_INT: constType = t_int; break;
    case KW_SHORT: constType = t_short; break;
    case KW_LONG: constType = t_long; break;
    case KW_BOOL: constType = t_bool; break;
    }

    t = peekToken();
    if (t != IDENT) {
        syntaxError("Ожидался идентификатор после типа в объявлении константы");
    }
    nextToken();
    std::string constName = cur_lex;

    t = peekToken();
    if (t != ASSIGN) {
        syntaxError("Ожидался символ '=' после идентификатора в объявлении константы");
    }
    nextToken(); // '='

    dataDetails exprDetails = expr(); // выражение для инициализации константы

    // Проверка совместимости типов
    if (!Tree::isAssignCompatible({ constType }, exprDetails)) {
        throw SemanticError("Семантическая ошибка: несовместимые типы в инициализации константы '" + constName + "'");
    }

    // Добавляем константу в дерево
    dataDetails constDetails = { constType };
    try {
        tree->addSymbol(Const, constName, constDetails, true, sc->getLineCol().first, sc->getLineCol().second);
    }
    catch (const std::runtime_error& e) {
        throw SemanticError(std::string("Семантическая ошибка: ") + e.what());
    }

    t = peekToken();
    if (t != SEMI) {
        syntaxError("Ожидалась ';' в конце объявления константы");
    }
    nextToken(); // ';'
}

void Diagram::varDecl() {
    // Получаем тип переменной
    int type_tok = nextToken();
    dataType varType;
    switch (type_tok) {
    case KW_INT: varType = t_int; break;
    case KW_SHORT: varType = t_short; break;
    case KW_LONG: varType = t_long; break;
    case KW_BOOL: varType = t_bool; break;
    default: syntaxError("Ожидался тип переменной");
    }

    while (true) {
        int t = peekToken();
        if (t != IDENT) {
            syntaxError("Ожидался идентификатор переменной");
        }
        nextToken();
        std::string varName = cur_lex;

        dataDetails varDetails = { varType };

        // Проверка на инициализацию
        t = peekToken();
        if (t == ASSIGN) {
            nextToken(); // '='
            dataDetails initDetails = expr(); // значение для инициализации

            if (!Tree::isAssignCompatible(varDetails, initDetails)) {
                throw SemanticError("Семантическая ошибка: несовместимые типы в инициализации переменной '" + varName + "'");
            }
        }

        // Добавляем переменную в дерево
        try {
            tree->addSymbol(Var, varName, varDetails, false, sc->getLineCol().first, sc->getLineCol().second);
        }
        catch (const std::runtime_error& e) {
            throw SemanticError(std::string("Семантическая ошибка: ") + e.what());
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

void Diagram::block() {
    int t = peekToken();
    if (t != LBRACE) {
        syntaxError("Ожидался '{' для начала блока");
    }
    nextToken(); // LBRACE

    // Входим в новую область видимости
    tree->enterScope(sc->getLineCol().first, sc->getLineCol().second);

    blockItems();

    // Выходим из области видимости
    tree->exitScope();

    t = peekToken();
    if (t != RBRACE) {
        syntaxError("Ожидался '}' для завершения блока");
    }
    nextToken(); // RBRACE
}

void Diagram::blockItems() {
    while (true) {
        int t = peekToken();
        if (t == RBRACE || t == T_END) {
            break;
        }
        else if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
            varDecl();
        }
        else if (t == KW_CONST) {
            constDecl();
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
        std::string identName = cur_lex;
        nextToken(); // IDENT

        // Ищем идентификатор в дереве
        Node* identNode = tree->findSymbolUp(identName);
        if (!identNode) {
            throw SemanticError("Семантическая ошибка: необъявленный идентификатор '" + identName + "'");
        }

        else if (identNode->type == Func) {
            throw SemanticError("Семантическая ошибка: функция в качестве левого операнда '" + identName + "'");
        }

        t = peekToken();
        if (t == LPAREN) {
            // Вызов функции
            if (identNode->type != Func) {
                throw SemanticError("Семантическая ошибка: '" + identName + "' не является функцией");
            }

            nextToken(); // LPAREN

            // Собираем аргументы вызова
            std::vector<dataDetails> args;
            t = peekToken();
            if (t != RPAREN) {
                args.push_back(expr()); // первый аргумент
                t = peekToken();
                while (t == COMMA) {
                    nextToken(); // COMMA
                    args.push_back(expr()); // следующий аргумент
                    t = peekToken();
                }
            }

            // Проверяем соответствие количества и типов параметров
            if (functionParams.find(identName) != functionParams.end()) {
                const std::vector<dataDetails>& expectedParams = functionParams[identName];

                if (expectedParams.size() != args.size()) {
                    throw SemanticError("Семантическая ошибка: неверное количество аргументов в вызове функции '" +
                        identName + "'. Ожидается " + std::to_string(expectedParams.size()) +
                        ", получено " + std::to_string(args.size()));
                }
                else {
                    // Проверяем типы аргументов
                    for (size_t i = 0; i < args.size(); i++) {
                        if (!Tree::isAssignCompatible(expectedParams[i], args[i])) {
                            throw SemanticError("Семантическая ошибка: несовместимый тип аргумента " +
                                std::to_string(i + 1) + " в вызове функции '" + identName + "'");
                        }
                    }
                }
            }
            else if (identNode->type == Func) {
                // Функция найдена, но нет информации о параметрах
                throw SemanticError("Семантическая ошибка: отсутствует информация о параметрах функции '" + identName + "'");
            }

            if (t != RPAREN) {
                syntaxError("Ожидался ')' после аргументов функции");
            }
            nextToken(); // RPAREN

            t = peekToken();
            if (t != SEMI) {
                syntaxError("Ожидалась ';' после вызова функции");
            }
            nextToken(); // SEMI

            return;
        }

        if (t == ASSIGN) {
            if (identNode && identNode->type == Const) {
                throw SemanticError("Семантическая ошибка: нельзя присваивать значение константе '" + identName + "'");
            }

            nextToken(); // ASSIGN
            dataDetails rhsDetails = expr();

            if (identNode) {
                dataDetails lhsDetails = identNode->details;

                if (!Tree::isAssignCompatible(lhsDetails, rhsDetails)) {
                    throw SemanticError("Семантическая ошибка: несовместимые типы в присваивании");
                }
            }

            t = peekToken();
            if (t != SEMI) {
                syntaxError("Ожидалась ';' после присваивания");
            }
            nextToken(); // SEMI

            return;
        }

        // Если это не вызов функции и не присваивание, то это выражение
        if (t != SEMI) {
            syntaxError("Ожидалась ';' после выражения");
        }
        nextToken(); // SEMI

        return;
    }

    // Если это не идентификатор, то это выражение
    expr();
    t = peekToken();
    if (t != SEMI) {
        syntaxError("Ожидалась ';' после выражения");
    }
    nextToken(); // SEMI
}

void Diagram::whileStmt() {
    int t = peekToken();
    if (t != LPAREN) {
        syntaxError("Ожидался '(' после 'while'");
    }
    nextToken(); // LPAREN

    dataDetails condDetails = expr(); // условие

    // Проверка типа условия
    if (condDetails.type != t_bool) {
        throw SemanticError("Семантическая ошибка: условие цикла while должно быть логическим");
    }

    t = peekToken();
    if (t != RPAREN) {
        syntaxError("Ожидался ')' после условия цикла while");
    }
    nextToken(); // RPAREN

    stmt(); // тело цикла
}

void Diagram::returnStmt() {
    // Проверка, что return используется только внутри функции
    if (!inFunction) {
        syntaxError("Оператор 'return' может использоваться только внутри функции");
    }

    int t = peekToken();
    if (t != SEMI) {
        dataDetails retDetails = expr(); // возвращаемое значение

        // Проверка совместимости с типом возврата функции
        if (!Tree::isAssignCompatible({ currentReturnType }, retDetails)) {
            throw SemanticError("Семантическая ошибка: несовместимый тип возвращаемого значения");
        }
    }
    t = peekToken();
    if (t != SEMI) {
        syntaxError("Ожидалась ';' после оператора return");
    }
    nextToken(); // SEMI
}

dataDetails Diagram::expr() {
    dataDetails left = rel();
    int t = peekToken();
    while (t == EQ || t == NEQ) {
        nextToken(); // оператор сравнения
        dataDetails right = rel();

        // Проверка совместимости типов для сравнения
        if (!Tree::isTypeCompatible(left.type, right.type)) {
            throw SemanticError("Семантическая ошибка: несовместимые типы в операции сравнения");
        }

        // Результат сравнения - bool
        left.type = t_bool;

        t = peekToken();
    }
    return left;
}

dataDetails Diagram::rel() {
    dataDetails left = shift();
    int t = peekToken();
    while (t == LT || t == LE || t == GT || t == GE) {
        nextToken(); // оператор отношения
        dataDetails right = shift();

        // Проверка совместимости типов для отношений
        if (!Tree::isTypeCompatible(left.type, right.type)) {
            throw SemanticError("Семантическая ошибка: несовместимые типы в операции отношения");
        }

        // Результат отношения - bool
        left.type = t_bool;

        t = peekToken();
    }
    return left;
}

dataDetails Diagram::shift() {
    dataDetails left = add();
    int t = peekToken();
    while (t == BIT_LEFT || t == BIT_RIGHT) {
        nextToken(); // '<<' или '>>'
        dataDetails right = add();

        // Проверка типов для битовых сдвигов
        if (left.type != t_int && left.type != t_short && left.type != t_long) {
            throw SemanticError("Семантическая ошибка: левый операнд сдвига должен быть целочисленным");
        }
        if (right.type != t_int && right.type != t_short && right.type != t_long) {
            throw SemanticError("Семантическая ошибка: правый операнд сдвига должен быть целочисленным");
        }

        // Результат сдвига - тип левого операнда
        // left.type остается без изменений

        t = peekToken();
    }
    return left;
}

dataDetails Diagram::add() {
    dataDetails left = mul();
    int t = peekToken();
    while (t == PLUS || t == MINUS) {
        nextToken(); // '+' или '-'
        dataDetails right = mul();

        // Проверка типов для арифметических операций
        if (left.type == t_bool || right.type == t_bool) {
            throw SemanticError("Семантическая ошибка: булевский тип не поддерживает арифметические операции");
        }

        // Определение результирующего типа
        if (!Tree::isTypeCompatible(left.type, right.type)) {
            throw SemanticError("Семантическая ошибка: несовместимые типы в арифметической операции");
        }

        // Для арифметики используем больший тип (оставляем тип левого операнда)

        t = peekToken();
    }
    return left;
}

dataDetails Diagram::mul() {
    dataDetails left = bitOr();
    int t = peekToken();
    while (t == MULT || t == DIV || t == MOD) {
        nextToken(); // '*' '/' или '%'
        dataDetails right = bitOr();

        // Проверка типов для арифметических операций
        if (left.type == t_bool || right.type == t_bool) {
            throw SemanticError("Семантическая ошибка: булевский тип не поддерживает арифметические операции");
        }

        // Определение результирующего типа
        if (!Tree::isTypeCompatible(left.type, right.type)) {
            throw SemanticError("Семантическая ошибка: несовместимые типы в арифметической операции");
        }

        t = peekToken();
    }
    return left;
}

dataDetails Diagram::bitOr() {
    dataDetails left = bitXor();
    int t = peekToken();
    while (t == BIT_OR) {
        nextToken(); // '|'
        dataDetails right = bitXor();

        // Проверка типов для битовых операций
        if (left.type == t_bool || right.type == t_bool) {
            throw SemanticError("Семантическая ошибка: булевский тип не поддерживает битовые операции");
        }

        // Определение результирующего типа
        if (!Tree::isTypeCompatible(left.type, right.type)) {
            throw SemanticError("Семантическая ошибка: несовместимые типы в битовой операции");
        }

        t = peekToken();
    }
    return left;
}

dataDetails Diagram::bitXor() {
    dataDetails left = bitAnd();
    int t = peekToken();
    while (t == BIT_XOR) {
        nextToken(); // '^'
        dataDetails right = bitAnd();

        // Проверка типов для битовых операций
        if (left.type == t_bool || right.type == t_bool) {
            throw SemanticError("Семантическая ошибка: булевский тип не поддерживает битовые операции");
        }

        // Определение результирующего типа
        if (!Tree::isTypeCompatible(left.type, right.type)) {
            throw SemanticError("Семантическая ошибка: несовместимые типы в битовой операции");
        }

        t = peekToken();
    }
    return left;
}

dataDetails Diagram::bitAnd() {
    dataDetails left = prim();
    int t = peekToken();
    while (t == BIT_AND) {
        nextToken(); // '&'
        dataDetails right = prim();

        // Проверка типов для битовых операций
        if (left.type == t_bool || right.type == t_bool) {
            throw SemanticError("Семантическая ошибка: булевский тип не поддерживает битовые операции");
        }

        // Определение результирующего типа
        if (!Tree::isTypeCompatible(left.type, right.type)) {
            throw SemanticError("Семантическая ошибка: несовместимые типы в битовой операции");
        }

        t = peekToken();
    }
    return left;
}

dataDetails Diagram::prim() {
    int t = peekToken();

    // Логические константы
    if (t == KW_TRUE || t == KW_FALSE) {
        nextToken(); // логическая константа
        return { t_bool };
    }

    // Числовые константы
    if (t == CONST_DEC || t == CONST_HEX) {
        nextToken(); // числовая константа
        return { t_int };
    }
    if (t == CONST_DEC_LONG || t == CONST_HEX_LONG) {
        nextToken(); // числовая константа
        return { t_long };
    }

    // Выражение в скобках
    if (t == LPAREN) {
        nextToken(); // '('
        dataDetails details = expr();
        t = peekToken();
        if (t != RPAREN) {
            syntaxError("Ожидался ')' после выражения");
        }
        nextToken(); // ')'
        return details;
    }

    // Идентификатор
    if (t == IDENT) {
        std::string identName = cur_lex;
        nextToken(); // идентификатор

        // Ищем идентификатор в дереве
        Node* identNode = tree->findSymbolUp(identName);
        if (!identNode) {
            throw SemanticError("Семантическая ошибка: необъявленный идентификатор '" + identName + "'");
        }

        t = peekToken();

        // Вызов функции
        if (t == LPAREN) {
            if (identNode->type != Func) {
                throw SemanticError("Семантическая ошибка: '" + identName + "' не является функцией");
            }

            nextToken(); // '('

            // Собираем аргументы вызова
            std::vector<dataDetails> args;
            t = peekToken();
            if (t != RPAREN) {
                args.push_back(expr()); // первый аргумент
                t = peekToken();
                while (t == COMMA) {
                    nextToken(); // ','
                    args.push_back(expr()); // следующий аргумент
                    t = peekToken();
                }
            }

            // Проверяем соответствие количества и типов параметров
            if (functionParams.find(identName) != functionParams.end()) {
                const std::vector<dataDetails>& expectedParams = functionParams[identName];

                if (expectedParams.size() != args.size()) {
                    throw SemanticError("Семантическая ошибка: неверное количество аргументов в вызове функции '" +
                        identName + "'. Ожидается " + std::to_string(expectedParams.size()) +
                        ", получено " + std::to_string(args.size()));
                }
                else {
                    // Проверяем типы аргументов
                    for (size_t i = 0; i < args.size(); i++) {
                        if (!Tree::isAssignCompatible(expectedParams[i], args[i])) {
                            throw SemanticError("Семантическая ошибка: несовместимый тип аргумента " +
                                std::to_string(i + 1) + " в вызове функции '" + identName + "'");
                        }
                    }
                }
            }
            else {
                throw SemanticError("Семантическая ошибка: отсутствует информация о параметрах функции '" + identName + "'");
            }

            if (t != RPAREN) {
                syntaxError("Ожидался ')' после аргументов функции");
            }
            nextToken(); // ')'

            // Возвращаем тип функции
            dataDetails result = identNode->details;
            return result;
        }

        // Простой идентификатор
        return identNode->details;
    }

    // Унарный минус или плюс
    if (t == MINUS || t == PLUS) {
        nextToken(); // унарный минус или плюс
        dataDetails details = prim();

        // Проверка типа для унарной операции
        if (details.type == t_bool) {
            throw SemanticError("Семантическая ошибка: булевский тип не поддерживает унарные операции");
        }

        return details;
    }

    syntaxError("Ожидалось первичное выражение");
    return { t_int }; // Заглушка для компилятора
}