#include "pch.h"
#include "CppUnitTest.h"

#include <fstream>   
#include <cstdio>    

#include "../Lab4/scanner.cpp"   // объявление класса Scanner
#include "../Lab4/tree.cpp"      // объявление классов Node, Tree
#include "../Lab4/defines.h"   // коды лексем

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
    // Вспомогательная функция для загрузки строки в сканер.
    // Поскольку в scanner.cpp есть только loadFile, можно либо добавить метод loadFromString
    // в класс Scanner, либо использовать временный файл. Для простоты теста здесь показан вариант
    // с временным файлом (в реальном проекте лучше добавить loadFromString в Scanner).
    //void LoadScannerFromString(Scanner& sc, const std::string& source)
    //{
    //    // Создаём временный файл (путь можно заменить на подходящий для окружения)
    //    std::string tempFileName = "temp_test_source.txt";
    //    std::ofstream out(tempFileName);
    //    out << source;
    //    out.close();

    //    bool ok = sc.loadFile(tempFileName);
    //    Assert::IsTrue(ok, L"Не удалось загрузить временный файл");

    //    // Удаляем файл после загрузки (опционально)
    //    std::remove(tempFileName.c_str());
    //}

    // Тестовый класс для тестов лексера
    TEST_CLASS(ScannerTests)
    {
    public:
        TEST_METHOD(TestKeywordShort)
        {
            Scanner sc;
            sc.loadFromString("short");
            std::string lex;
            int tok = sc.getNextLex(lex);
            Assert::AreEqual(KW_SHORT, tok);
        }

        TEST_METHOD(TestIdentifier)
        {
            Scanner sc;
            sc.loadFromString("_mYvAR52");
            std::string lex;
            int tok = sc.getNextLex(lex);
            Assert::AreEqual(IDENT, tok);
        }

        TEST_METHOD(TestDecConstant)
        {
            Scanner sc;
            sc.loadFromString("9876543210");
            std::string lex;
            int tok = sc.getNextLex(lex);
            // В новом лексере обычная десятичная константа без суффикса L возвращает CONST_DEC
            Assert::AreEqual(CONST_DEC, tok);
        }

        TEST_METHOD(TestHexConstant)
        {
            Scanner sc;
            sc.loadFromString("0xAB6");
            std::string lex;
            int tok = sc.getNextLex(lex);
            Assert::AreEqual(CONST_HEX, tok);
        }

        TEST_METHOD(TestOperators)
        {
            Scanner sc;
            sc.loadFromString("+ - * / % == != < <= > >= =");
            std::string lex;
            int tok;

            tok = sc.getNextLex(lex); Assert::AreEqual(PLUS, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(MINUS, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(MULT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(DIV, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(MOD, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(EQ, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(NEQ, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(LT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(LE, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(GT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(GE, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(ASSIGN, tok);
        }

        TEST_METHOD(TestLongDecimalConstant)
        {
            Scanner sc;
            sc.loadFromString("1234567890L");
            std::string lex;
            int tok = sc.getNextLex(lex);
            Assert::AreEqual(CONST_DEC_LONG, tok);
        }

        TEST_METHOD(TestIdentifierTooLong)
        {
            // MAX_CONST_LEN = 20, идентификатор из 21 символа
            Scanner sc;
            sc.loadFromString("a12345678901234567890"); // 21 символ
            std::string lex;
            int tok = sc.getNextLex(lex);
            Assert::AreEqual(T_ERR, tok);
        }
    };

    // Тестовый класс для тестов семантических операций (работа с деревом)
    TEST_CLASS(SemanticTests)
    {
    public:
        // Перед каждым тестом создаём новое дерево (конструктор автоматически создаёт корневую Scope)
        Tree tree;

        TEST_METHOD(TestIncludeVariable)
        {
            // Добавляем переменную в текущую (глобальную) область
            dataDetails details = { t_short };
            Node* varNode = tree.addSymbol(Var, "s", details, false, 1, 1);

            // Ищем её
            Node* found = tree.findSymbolUp("s");

            Assert::IsNotNull(found);
            Assert::AreEqual(std::string("s"), found->name);
            Assert::AreEqual((int)t_short, (int)found->details.type);
        }

        TEST_METHOD(TestDuplicateVariable)
        {
            dataDetails details = { t_short };
            tree.addSymbol(Var, "s", details, false, 1, 1);

            // Повторное объявление в той же области должно вызвать исключение
            bool exceptionThrown = false;
            try
            {
                tree.addSymbol(Var, "s", details, false, 1, 1);
            }
            catch (const std::runtime_error&)
            {
                exceptionThrown = true;
            }
            Assert::IsTrue(exceptionThrown);
        }

        TEST_METHOD(TestFindVariableInParentBlock)
        {
            // Объявляем переменную в глобальной области
            dataDetails details = { t_short };
            tree.addSymbol(Var, "s", details, false, 1, 1);

            // Входим во вложенный блок
            tree.enterScope(2, 1);

            // Поиск должен подняться в родительскую область
            Node* found = tree.findSymbolUp("s");
            Assert::IsNotNull(found);
            Assert::AreEqual(std::string("s"), found->name);

            // Выходим из блока
            tree.exitScope();
        }

        // Тесты на присваивание значений отсутствуют, так как в новом дереве
        // не хранятся значения – только информация о типах и константности.
        // При необходимости можно добавить интеграционные тесты с классом Diagram.
    };

}