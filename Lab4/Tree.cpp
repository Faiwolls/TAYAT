#include "tree.h"
#include <stdexcept>
#include <iostream>
#include <functional>

using namespace std;

// Конструкторы Node
Node::Node(nodeType k)
    : type(k), left(nullptr), right(nullptr), parent(nullptr),
    name(""), details({ false, t_int, 0 }), isConst(false), line(0), pos(0)
{
}

Node::Node(nodeType k, const std::string& name, dataDetails details,
    bool isConst, int line, int pos)
    : type(k), left(nullptr), right(nullptr), parent(nullptr),
    name(name), details(details), isConst(isConst), line(line), pos(pos)
{
}

Node::~Node() {}

Tree::Tree() {
    root = new Node(Scope);
    root->name = "<root>";
    current = root;
}

Tree::~Tree() {
    // Рекурсивное удаление всех узлов
    function<void(Node*)> deleteTree = [&](Node* node) {
        if (!node) return;
        // Удаляем детей (правую ветвь)
        deleteTree(node->right);
        // Удаляем соседей (левую ветвь)
        deleteTree(node->left);
        delete node;
        };
    deleteTree(root);
    root = nullptr;
    current = nullptr;
}

void Tree::addChild(Node* parent, Node* child) {
    if (!parent || !child) return;
    child->parent = parent;

    // Если у родителя еще нет детей, child становится первым ребенком
    if (!parent->right) {
        parent->right = child;
    }
    else {
        // Иначе добавляем в конец списка детей
        Node* sibling = parent->right;
        while (sibling->left) {
            sibling = sibling->left;
        }
        sibling->left = child;
    }
}

// Вход в новую область видимости
Node* Tree::enterScope(int line, int pos) {
    Node* scope = new Node(Scope);
    scope->line = line;
    scope->pos = pos;
    scope->name = "<scope>";
    addChild(current, scope);
    current = scope;
    return scope;
}

// Выход из текущей области видимости
Node* Tree::exitScope() {
    if (current->parent) {
        current = current->parent;
    }
    return current;
}

// Добавление символа в текущую область видимости
Node* Tree::addSymbol(nodeType type,
    const std::string& name,
    const dataDetails& details,
    bool isConst,
    int line, int pos)
{
    // Проверка на дублирование в текущей области
    Node* child = current->right;
    while (child) {
        if (child->type != Scope && child->name == name) {
            throw runtime_error("Повторяющийся идентификатор '" + name + "'");
        }
        child = child->left;
    }

    Node* node = new Node(type, name, details, isConst, line, pos);
    addChild(current, node);
    return node;
}

// Поиск символа в конкретной области
Node* Tree::findInScope(Node* scope, const std::string& name) const {
    if (!scope) return nullptr;

    Node* child = scope->right;
    while (child) {
        if (child->type != Scope && child->name == name) {
            return child;
        }
        child = child->left;
    }
    return nullptr;
}

// Поиск символа, начиная с текущей области и поднимаясь вверх
Node* Tree::findSymbolUp(const std::string& name) const {
    Node* scope = current;
    while (scope) {
        Node* found = findInScope(scope, name);
        if (found) return found;
        scope = scope->parent;
    }
    return nullptr;
}

// Проверка совместимости базовых типов
bool Tree::isTypeCompatible(dataType type1, dataType type2) {
    // Все числовые типы совместимы между собой
    if ((type1 == t_int || type1 == t_short || type1 == t_long) &&
        (type2 == t_int || type2 == t_short || type2 == t_long)) {
        return true;
    }

    // Bool совместим только с bool
    if (type1 == t_bool && type2 == t_bool) {
        return true;
    }

    return type1 == type2;
}

// Проверка совместимости для присваивания
bool Tree::isAssignCompatible(const dataDetails& lhs, const dataDetails& rhs) {
    // Если массивы, должны совпадать размеры
    if (lhs.isArr != rhs.isArr) return false;
    if (lhs.isArr && lhs.size != rhs.size) return false;

    // Проверка совместимости базовых типов
    return isTypeCompatible(lhs.type, rhs.type);
}

// Рекурсивный вывод дерева
void Tree::printRec(const Node* node, int indent) const {
    if (!node) return;

    string padding(indent, ' ');
    cout << padding;

    switch (node->type) {
    case Scope: cout << "[Scope]"; break;
    case Var: cout << "Variable "; break;
    case Const: cout << "Constant "; break;
    case Func: cout << "Function "; break;
    case Arr: cout << "Array "; break;
    default: cout << "Node "; break;
    }

    if (!node->name.empty()) {
        if (node->type != Scope )cout << node->name;
    }

    // Вывод информации о типе
    if (node->type == Var || node->type == Const || node->type == Func || node->type == Arr) {
        cout << " at [" << node->line << ":" << node->pos << "]";

        cout << " type ";
        switch (node->details.type) {
        case t_int: cout << "int"; break;
        case t_short: cout << "short"; break;
        case t_long: cout << "long"; break;
        case t_bool: cout << "bool"; break;
        default: cout << "unknown"; break;
        }

        if (node->details.isArr) {
            cout << "[" << node->details.size << "]";
        }

        if (node->isConst) {
            cout << " (const)";
        }
    }

    cout << "\n";

    // Вывод детей (правой ветви)
    printRec(node->right, indent + 2);
    // Вывод соседей (левой ветви)
    printRec(node->left, indent);
}

// Основной метод вывода дерева
void Tree::print() const {
    cout << "СЕМАНТИЧЕСКОЕ ДЕРЕВО\n";
    printRec(root, 0);
}