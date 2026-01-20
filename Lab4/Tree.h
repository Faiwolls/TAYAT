#pragma once
#include <string>

enum nodeType {
	Scope,
	Var,
	Const,
	Func,
	Arr
};

enum dataType {
	t_int,
	t_short,
	t_long,
	t_bool
};

struct dataDetails {
	bool isArr;
	dataType type;
	int size;
};

class Node
{
public:
    nodeType type;

    // Дерево: left - соседний узел на том же уровне, right - первый дочерний узел
    Node* left;
    Node* right;
    Node* parent; // родительский узел (область видимости)

    // Семантическая информация
    std::string name; // идентификатор
    dataDetails details; // детали типа данных

    bool isConst;
    int line; // строка в исходном коде
    int pos;  // позиция в строке
    int paramCount = 0;

    // Конструкторы
    Node(nodeType k);
    Node(nodeType k, const std::string& name, dataDetails details, bool isConst, int line, int pos);
    ~Node();
};

class Tree {
private:
    Node* root;    // корень дерева (глобальная область видимости)
    Node* current; // текущая область видимости

    

    // Вспомогательные методы
    void addChild(Node* parent, Node* child);
    Node* findInScope(Node* scope, const std::string& name) const;
    void printRec(const Node* node, int indent) const;

public:
    Tree();
    ~Tree();

    // Управление областями видимости
    Node* enterScope(int line = 0, int pos = 0);
    Node* exitScope();

    // Добавление символов
    Node* addSymbol(nodeType type,
        const std::string& name,
        const dataDetails& details,
        bool isConst,
        int line, int pos);

    // Поиск символов
    Node* findSymbolUp(const std::string& name) const;

    // Проверка совместимости типов
    static bool isTypeCompatible(dataType type1, dataType type2);
    static bool isAssignCompatible(const dataDetails& lhs, const dataDetails& rhs);

    // Отладочный вывод
    void print() const;

    // Геттеры
    Node* getCurrentScope() const { return current; }
    Node* getRoot() const { return root; }
};