#include "tree.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <functional>

Tree* Tree::Root = nullptr;
Tree* Tree::Cur = nullptr;
bool Tree::interpretationEnabled = true;
bool Tree::debug = true;
Tree* Tree::currentArea = nullptr;

void Tree::PrintTypeConversionWarning(DATA_TYPE from, DATA_TYPE to, const std::string& context,
    const std::string& expression, int line, int col) {

    if (!debug) return;

    std::cerr << "Предупреждение: неявное преобразование типа ";

    switch (from) {
    case TYPE_SHORT_INT: std::cerr << "short"; break;
    case TYPE_INT: std::cerr << "int"; break;
    case TYPE_LONG_INT: std::cerr << "long"; break;
    case TYPE_BOOL: std::cerr << "bool"; break;
    default: std::cerr << "unknown"; break;
    }

    std::cerr << " к ";

    switch (to) {
    case TYPE_SHORT_INT: std::cerr << "short"; break;
    case TYPE_INT: std::cerr << "int"; break;
    case TYPE_LONG_INT: std::cerr << "long"; break;
    case TYPE_BOOL: std::cerr << "bool"; break;
    default: std::cerr << "unknown"; break;
    }

    std::cerr << " в " << context;
    if (!expression.empty()) {
        std::cerr << " выражения " << expression;
    }
    std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
}

void Tree::SemError(const std::string& msg, const std::string& id, int line, int col) {
    std::cerr << "Семантическая ошибка: " << msg;
    if (!id.empty()) {
        std::cerr << " (около '" << id << "')";
    }
    std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
    std::exit(1);
}

void Tree::InterpError(const std::string& msg, const std::string& id, int line, int col) {
    std::cerr << "Ошибка при интерпретации: " << msg;
    if (!id.empty()) std::cerr << " (около '" + id + "')";
    std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
    std::exit(1);
}

Tree::Tree(SemNode* node, Tree* up) : n(node), Up(up), Left(nullptr), Right(nullptr) {
    if (Root == nullptr) {
        Root = this;
        Cur = this;
    }
}

Tree::~Tree() {
    if (n) {
        delete n;
    }
    if (Right) {
        delete Right; Right = nullptr;
    }
    if (Left) {
        delete Left; Left = nullptr;
    }
}

void Tree::SetRight(SemNode* Data) {
    Tree* newNode = new Tree(Data, this);
    if (this->Right == nullptr) {
        this->Right = newNode;
    }
    else {
        Tree* p = this->Right;
        while (p->Left) {
            p = p->Left;
        }
        p->Left = newNode;
        newNode->Up = this;
    }
}

void Tree::SetLeft(SemNode* Data) {
    Tree* newNode = new Tree(Data, this->Up);
    newNode->Left = this->Left;
    this->Left = newNode;
    newNode->Up = this->Up;
}

Tree* Tree::FindUpOneLevel(Tree* From, const std::string& id) {
    if (From == nullptr) {
        return nullptr;
    }
    Tree* p = From->Right;
    while (p != nullptr) {
        if ((p->n) && (p->n->id == id)) {
            return p;
        }
        p = p->Left;
    }
    return nullptr;
}

Tree* Tree::FindUp(Tree* From, const std::string& id) {
    Tree* cur = From;
    while (cur != nullptr) {
        Tree* found = FindUpOneLevel(cur, id);
        if (found) return found;
        cur = cur->Up;
    }
    return nullptr;
}

bool Tree::DupControl(Tree* Addr, const std::string& a) {
    return FindUpOneLevel(Addr, a) != nullptr;
}

Tree* Tree::SemInclude(const std::string& a, DATA_TYPE t, int line, int col) {
    if (Cur == nullptr) {
        SemError("Внутренняя ошибка: текущая область не установлена при SemInclude", a, line, col);
    }

    if (DupControl(Cur, a)) {
        SemError("Повторное описание идентификатора", a, line, col);
    }

    SemNode* node = new SemNode();
    node->id = a;
    node->DataType = t;
    node->hasValue = false;
    node->FlagConst = 0;
    node->BasicType = TYPE_UNDEFINED;
    node->ArrElemCount = 0;
    node->index = 0;
    node->line = line;
    node->col = col;

    Cur->SetRight(node);
    Tree* added = Cur->Right;
    while (added->Left) added = added->Left;
    return added;
}

Tree* Tree::SemIncludeConstant(const std::string& a, DATA_TYPE t, const std::string& value, int line, int col) {
    Tree* node = SemInclude(a, t, line, col);
    if (node && node->n) {
        node->n->hasValue = true;
        try {
            long long val = std::stoll(value, nullptr, 0);
            if (t == TYPE_SHORT_INT) {
                node->n->Value.v_int16 = static_cast<int16_t>(val);
            }
            else if (t == TYPE_INT) {
                node->n->Value.v_int32 = static_cast<int32_t>(val);
            }
            else if (t == TYPE_LONG_INT) {
                node->n->Value.v_int32 = static_cast<int32_t>(val);
            }
            else if (t == TYPE_BOOL) {
                node->n->Value.v_bool = (val != 0);
            }
        }
        catch (const std::exception& e) {
            SemError("Неверный формат константы: " + std::string(e.what()), a, line, col);
        }
    }
    return node;
}

void Tree::SemSetConst(Tree* Addr, int value) {
    if (Addr == nullptr || Addr->n == nullptr) {
        SemError("SemSetConst: неверный адрес именованной константы");
    }
    Addr->n->FlagConst = value;
}

void Tree::SemSetBasicType(Tree* Addr, DATA_TYPE bt) {
    if (Addr == nullptr || Addr->n == nullptr) {
        SemError("SemSetBasicType: неверный адрес массива или метки типа");
    }
    Addr->n->BasicType = bt;
}

void Tree::SemSetArrElemCount(Tree* Addr, int aec) {
    if (Addr == nullptr || Addr->n == nullptr) {
        SemError("SemSetArrElemCount: неверный адрес массива");
    }
    Addr->n->ArrElemCount = aec;
}

void Tree::SemSetIndex(Tree* Addr, int index) {
    if (Addr == nullptr || Addr->n == nullptr) {
        SemError("SemSetIndex: неверный адрес элемента массива");
    }
    Addr->n->index = index;
}

Tree* Tree::SemGetVar(const std::string& a, int line, int col) {
    Tree* v = FindUp(Cur, a);
    if (v == nullptr) {
        SemError("Отсутствует описание идентификатора", a, line, col);
    }
    return v;
}

Tree* Tree::SemEnterBlock(int line, int col) {
    if (Cur == nullptr) {
        SemError("SemEnterBlock: текущая область не установлена");
    }
    SemNode* sn = new SemNode();
    sn->id = "{}";
    sn->DataType = TYPE_SCOPE;
    sn->FlagConst = 0;
    sn->BasicType = TYPE_UNDEFINED;
    sn->ArrElemCount = 0;
    sn->index = 0;
    sn->line = line;
    sn->col = col;

    SetRight(sn);

    Tree* created = Cur->Right;
    while (created->Left) {
        created = created->Left;
    }

    Cur = created;
    return created;
}

void Tree::SemExitBlock() {
    if (Cur == nullptr) {
        SemError("SemExitBlock: текущая область не установлена");
    }
    if (Cur->Up == nullptr) {
        SemError("SemExitBlock: попытка выйти из корневой области");
    }
    Cur = Cur->Up;
}

void Tree::SetVarValue(const std::string& name, const SemNode& value, int line, int col) {
    Tree* varNode = Cur->SemGetVar(name, line, col);

    if (value.hasValue) {
        if (CanImplicitCast(value.DataType, varNode->n->DataType)) {
            bool needsTruncationWarning = false;
            long long originalValue = 0;

            switch (value.DataType) {
            case TYPE_SHORT_INT: originalValue = value.Value.v_int16; break;
            case TYPE_INT: originalValue = value.Value.v_int32; break;
            case TYPE_LONG_INT: originalValue = value.Value.v_int32; break;
            case TYPE_BOOL: originalValue = value.Value.v_bool ? 1 : 0; break;
            default: break;
            }

            if (varNode->n->DataType == TYPE_SHORT_INT) {
                if (originalValue < -32768 || originalValue > 32767) {
                    needsTruncationWarning = true;
                }
            }
            else if (varNode->n->DataType == TYPE_INT) {
                if (originalValue < -2147483648LL || originalValue > 2147483647LL) {
                    needsTruncationWarning = true;
                }
            }
            else if (varNode->n->DataType == TYPE_LONG_INT) {
                if (originalValue < -2147483648LL || originalValue > 2147483647LL) {
                    needsTruncationWarning = true;
                }
            }

            if (needsTruncationWarning) {
                std::string text_type = "long";
                if (varNode->n->DataType == TYPE_SHORT_INT) {
                    text_type = "short";
                }
                else if (varNode->n->DataType == TYPE_INT) {
                    text_type = "int";
                }
                else if (varNode->n->DataType == TYPE_BOOL) {
                    text_type = "bool";
                }

                std::cerr << "Предупреждение: значение " << originalValue
                    << " обрезается при преобразовании к "
                    << text_type;
                std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
            }
            else if (value.DataType != varNode->n->DataType && debug) {
                PrintTypeConversionWarning(value.DataType, varNode->n->DataType,
                    "присваивании", name + " = ...", line, col);
            }

            SemNode converted = CastToType(value, varNode->n->DataType, line, col);
            varNode->n->Value = converted.Value;
            varNode->n->hasValue = true;

            PrintAssignment(name, converted, line, col);
        }
        else {
            SemError("Несовместимые типы при присваивании", name, line, col);
        }
    }
    else {
        InterpError("Попытка присвоить NULL", name, line, col);
    }
}

SemNode Tree::GetVarValue(const std::string& name, int line, int col) {
    Tree* varNode = Cur->SemGetVar(name, line, col);
    if (!varNode->n->hasValue) {
        SemError("Использование неинициализированной переменной", name, line, col);
    }
    return *(varNode->n);
}

DATA_TYPE Tree::GetMaxType(DATA_TYPE t1, DATA_TYPE t2) {
    if (t1 == TYPE_LONG_INT || t2 == TYPE_LONG_INT) return TYPE_LONG_INT;
    if (t1 == TYPE_INT || t2 == TYPE_INT) return TYPE_INT;
    if (t1 == TYPE_BOOL && t2 == TYPE_BOOL) return TYPE_BOOL;
    return TYPE_SHORT_INT;
}

bool Tree::CanImplicitCast(DATA_TYPE from, DATA_TYPE to) {
    bool fromIsInt = (from == TYPE_SHORT_INT || from == TYPE_INT ||
        from == TYPE_LONG_INT || from == TYPE_BOOL);
    bool toIsInt = (to == TYPE_SHORT_INT || to == TYPE_INT ||
        to == TYPE_LONG_INT || to == TYPE_BOOL);
    return (fromIsInt && toIsInt);
}

SemNode Tree::CastToType(const SemNode& value, DATA_TYPE targetType, int line, int col, bool showWarning) {
    if (value.DataType == targetType) {
        return value;
    }

    SemNode result;
    result.DataType = targetType;
    result.hasValue = value.hasValue;

    if (!value.hasValue) return result;

    long long originalValue = 0;
    switch (value.DataType) {
    case TYPE_SHORT_INT: originalValue = value.Value.v_int16; break;
    case TYPE_INT: originalValue = value.Value.v_int32; break;
    case TYPE_LONG_INT: originalValue = value.Value.v_int32; break;
    case TYPE_BOOL: originalValue = value.Value.v_bool ? 1 : 0; break;
    default: break;
    }

    switch (targetType) {
    case TYPE_SHORT_INT:
        result.Value.v_int16 = static_cast<int16_t>(originalValue);
        break;

    case TYPE_INT:
        result.Value.v_int32 = static_cast<int32_t>(originalValue);
        break;

    case TYPE_LONG_INT:
        result.Value.v_int32 = static_cast<int32_t>(originalValue);
        break;

    case TYPE_BOOL:
        result.Value.v_bool = (originalValue != 0);
        break;

    default:
        SemError("Неизвестный тип для приведения", "", line, col);
    }

    return result;
}

SemNode Tree::ExecuteArithmeticOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        SemError("Операция с неинициализированными значениями", "", line, col);
    }

    if (left.DataType == TYPE_BOOL || right.DataType == TYPE_BOOL) {
        SemError("Арифметические операции не применимы к bool", "", line, col);
    }

    if (left.DataType != right.DataType && debug) {
        PrintTypeConversionWarning(left.DataType, right.DataType,
            "арифметической операции", "", line, col);
    }

    DATA_TYPE resultType = GetMaxType(left.DataType, right.DataType);
    SemNode leftConv = CastToType(left, resultType, line, col);
    SemNode rightConv = CastToType(right, resultType, line, col);

    SemNode result;
    result.DataType = resultType;
    result.hasValue = true;

    switch (resultType) {
    case TYPE_SHORT_INT:
        if (op == "+") result.Value.v_int16 = leftConv.Value.v_int16 + rightConv.Value.v_int16;
        else if (op == "-") result.Value.v_int16 = leftConv.Value.v_int16 - rightConv.Value.v_int16;
        else if (op == "*") result.Value.v_int16 = leftConv.Value.v_int16 * rightConv.Value.v_int16;
        else if (op == "/") {
            if (rightConv.Value.v_int16 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int16 = leftConv.Value.v_int16 / rightConv.Value.v_int16;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int16 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int16 = leftConv.Value.v_int16 % rightConv.Value.v_int16;
        }
        break;

    case TYPE_INT:
        if (op == "+") result.Value.v_int32 = leftConv.Value.v_int32 + rightConv.Value.v_int32;
        else if (op == "-") result.Value.v_int32 = leftConv.Value.v_int32 - rightConv.Value.v_int32;
        else if (op == "*") result.Value.v_int32 = leftConv.Value.v_int32 * rightConv.Value.v_int32;
        else if (op == "/") {
            if (rightConv.Value.v_int32 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 / rightConv.Value.v_int32;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int32 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 % rightConv.Value.v_int32;
        }
        break;

    case TYPE_LONG_INT:
        if (op == "+") result.Value.v_int32 = leftConv.Value.v_int32 + rightConv.Value.v_int32;
        else if (op == "-") result.Value.v_int32 = leftConv.Value.v_int32 - rightConv.Value.v_int32;
        else if (op == "*") result.Value.v_int32 = leftConv.Value.v_int32 * rightConv.Value.v_int32;
        else if (op == "/") {
            if (rightConv.Value.v_int32 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 / rightConv.Value.v_int32;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int32 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 % rightConv.Value.v_int32;
        }
        break;

    default:
        SemError("Неподдерживаемый тип для арифметической операции", "", line, col);
    }

    if (debug && interpretationEnabled) {
        PrintArithmeticOp(op, leftConv, rightConv, result, line, col);
    }

    return result;
}

SemNode Tree::ExecuteComparisonOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        SemError("Операция с неинициализированными значениями", "", line, col);
    }

    DATA_TYPE resultType = GetMaxType(left.DataType, right.DataType);
    SemNode leftConv = CastToType(left, resultType, line, col);
    SemNode rightConv = CastToType(right, resultType, line, col);

    SemNode result;
    result.DataType = TYPE_INT;
    result.hasValue = true;

    switch (resultType) {
    case TYPE_SHORT_INT:
        if (op == "<") result.Value.v_int32 = leftConv.Value.v_int16 < rightConv.Value.v_int16;
        else if (op == "<=") result.Value.v_int32 = leftConv.Value.v_int16 <= rightConv.Value.v_int16;
        else if (op == ">") result.Value.v_int32 = leftConv.Value.v_int16 > rightConv.Value.v_int16;
        else if (op == ">=") result.Value.v_int32 = leftConv.Value.v_int16 >= rightConv.Value.v_int16;
        else if (op == "==") result.Value.v_int32 = leftConv.Value.v_int16 == rightConv.Value.v_int16;
        else if (op == "!=") result.Value.v_int32 = leftConv.Value.v_int16 != rightConv.Value.v_int16;
        break;

    case TYPE_INT:
        if (op == "<") result.Value.v_int32 = leftConv.Value.v_int32 < rightConv.Value.v_int32;
        else if (op == "<=") result.Value.v_int32 = leftConv.Value.v_int32 <= rightConv.Value.v_int32;
        else if (op == ">") result.Value.v_int32 = leftConv.Value.v_int32 > rightConv.Value.v_int32;
        else if (op == ">=") result.Value.v_int32 = leftConv.Value.v_int32 >= rightConv.Value.v_int32;
        else if (op == "==") result.Value.v_int32 = leftConv.Value.v_int32 == rightConv.Value.v_int32;
        else if (op == "!=") result.Value.v_int32 = leftConv.Value.v_int32 != rightConv.Value.v_int32;
        break;

    case TYPE_LONG_INT:
        if (op == "<") result.Value.v_int32 = leftConv.Value.v_int32 < rightConv.Value.v_int32;
        else if (op == "<=") result.Value.v_int32 = leftConv.Value.v_int32 <= rightConv.Value.v_int32;
        else if (op == ">") result.Value.v_int32 = leftConv.Value.v_int32 > rightConv.Value.v_int32;
        else if (op == ">=") result.Value.v_int32 = leftConv.Value.v_int32 >= rightConv.Value.v_int32;
        else if (op == "==") result.Value.v_int32 = leftConv.Value.v_int32 == rightConv.Value.v_int32;
        else if (op == "!=") result.Value.v_int32 = leftConv.Value.v_int32 != rightConv.Value.v_int32;
        break;

    case TYPE_BOOL:
        if (op == "==") result.Value.v_int32 = leftConv.Value.v_bool == rightConv.Value.v_bool;
        else if (op == "!=") result.Value.v_int32 = leftConv.Value.v_bool != rightConv.Value.v_bool;
        else SemError("Операции <, <=, >, >= не применимы к bool", "", line, col);
        break;

    default:
        SemError("Неподдерживаемый тип для операции сравнения", "", line, col);
    }

    return result;
}

SemNode Tree::ExecuteBitwiseOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        SemError("Операция с неинициализированными значениями", "", line, col);
    }

    if (left.DataType == TYPE_BOOL || right.DataType == TYPE_BOOL) {
        SemError("Побитовые операции не применимы к bool", "", line, col);
    }

    DATA_TYPE resultType = GetMaxType(left.DataType, right.DataType);
    SemNode leftConv = CastToType(left, resultType, line, col);
    SemNode rightConv = CastToType(right, resultType, line, col);

    SemNode result;
    result.DataType = resultType;
    result.hasValue = true;

    switch (resultType) {
    case TYPE_INT:
        if (op == "&") result.Value.v_int32 = leftConv.Value.v_int32 & rightConv.Value.v_int32;
        else if (op == "|") result.Value.v_int32 = leftConv.Value.v_int32 | rightConv.Value.v_int32;
        else if (op == "^") result.Value.v_int32 = leftConv.Value.v_int32 ^ rightConv.Value.v_int32;
        break;

    default:
        SemError("Побитовые операции поддерживаются только для int", "", line, col);
    }

    if (debug && interpretationEnabled) {
        PrintBitwiseOp(op, leftConv, rightConv, result, line, col);
    }

    return result;
}

std::string Tree::makeLabel(const Tree* tree) const {
    if (tree == nullptr) return "<null-tree>";
    SemNode* n = tree->n;
    if (!n) return "<null-node>";

    std::ostringstream oss;

    if (!n->id.empty()) {
        oss << n->id;
    }
    else {
        oss << "{}";
    }

    switch (n->DataType) {
    case TYPE_INT: oss << " (int)"; break;
    case TYPE_SHORT_INT: oss << " (short)"; break;
    case TYPE_LONG_INT: oss << " (long)"; break;
    case TYPE_BOOL: oss << " (bool)"; break;
    case TYPE_ARRAY: oss << " (массив)[" << n->ArrElemCount << "] (типа "; break;
    case TYPE_SCOPE: oss << " (область)"; break;
    default: oss << " (?)"; break;
    }

    if (n->FlagConst) {
        oss << " (const)";
    }

    if (n->DataType == TYPE_ARRAY) {
        switch (n->BasicType) {
        case TYPE_INT: oss << "int)"; break;
        case TYPE_SHORT_INT: oss << "short)"; break;
        case TYPE_LONG_INT: oss << "long)"; break;
        case TYPE_BOOL: oss << "bool)"; break;
        default: oss << "?)"; break;
        }
    }

    std::string rname = (tree->Right && tree->Right->n) ?
        (tree->Right->n->DataType == TYPE_SCOPE ? "{}" :
            (tree->Right->n->id.empty() ? "?" : tree->Right->n->id)) : "-";
    std::string lname = (tree->Left && tree->Left->n) ?
        (tree->Left->n->DataType == TYPE_SCOPE ? "{}" :
            (tree->Left->n->id.empty() ? "?" : tree->Left->n->id)) : "-";

    oss << " (L = " << lname << ", R = " << rname << ")";

    return oss.str();
}

void Tree::Print(int depth) {
    if (this == nullptr) return;

    std::string label = makeLabel(this);
    for (int i = 0; i < depth; ++i) {
        std::cout << ' ';
    }
    std::cout << label << '\n';

    Tree* child = this->Right;
    while (child) {
        child->Print(depth + 4);
        child = child->Left;
    }
}

void Tree::Print() {
    Print(0);
}

void Tree::PrintDebugInfo(const std::string& message, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::string context = "глобальная область";
    if (currentArea && currentArea->n) {
        if (currentArea->n->DataType == TYPE_SCOPE) {
            if (currentArea->n->id == "{}") {
                context = "{}";
            }
            else if (currentArea->n->id == "<глобальная область видимости>") {
                context = "глобальная область";
            }
            else if (!currentArea->n->id.empty()) {
                context = currentArea->n->id;
            }
        }
    }

    std::cout << "DEBUG: [" << context << "]";
    if (line > 0) {
        std::cout << " (строка " << line << ":" << col << ")";
    }
    std::cout << " " << message << std::endl;
}

void Tree::PrintAssignment(const std::string& varName, const SemNode& value, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Присваивание: " << varName << " = ";

    if (value.hasValue) {
        switch (value.DataType) {
        case TYPE_SHORT_INT:
            oss << value.Value.v_int16 << " (short)";
            break;
        case TYPE_INT:
            oss << value.Value.v_int32 << " (int)";
            break;
        case TYPE_LONG_INT:
            oss << value.Value.v_int32 << " (long)";
            break;
        case TYPE_BOOL:
            oss << (value.Value.v_bool ? "true" : "false") << " (bool)";
            break;
        default:
            oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    PrintDebugInfo(oss.str(), line, col);
}

void Tree::PrintArithmeticOp(const std::string& op, const SemNode& left, const SemNode& right, const SemNode& result, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Арифметическая операция: ";

    if (left.hasValue) {
        switch (left.DataType) {
        case TYPE_SHORT_INT: oss << left.Value.v_int16 << " (short)"; break;
        case TYPE_INT: oss << left.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT: oss << left.Value.v_int32 << " (long)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    oss << " " << op << " ";

    if (right.hasValue) {
        switch (right.DataType) {
        case TYPE_SHORT_INT: oss << right.Value.v_int16 << " (short)"; break;
        case TYPE_INT: oss << right.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT: oss << right.Value.v_int32 << " (long)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    oss << " = ";

    if (result.hasValue) {
        switch (result.DataType) {
        case TYPE_SHORT_INT: oss << result.Value.v_int16 << " (short)"; break;
        case TYPE_INT: oss << result.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT: oss << result.Value.v_int32 << " (long)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    PrintDebugInfo(oss.str(), line, col);
}

void Tree::PrintBitwiseOp(const std::string& op, const SemNode& left, const SemNode& right, const SemNode& result, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Побитовая операция: ";

    if (left.hasValue) {
        switch (left.DataType) {
        case TYPE_INT: oss << left.Value.v_int32 << " (int)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    oss << " " << op << " ";

    if (right.hasValue) {
        switch (right.DataType) {
        case TYPE_INT: oss << right.Value.v_int32 << " (int)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    oss << " = ";

    if (result.hasValue) {
        switch (result.DataType) {
        case TYPE_INT: oss << result.Value.v_int32 << " (int)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    PrintDebugInfo(oss.str(), line, col);
}