#pragma once
#include "sem_node.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

class Tree {
public:
    SemNode* n;
    Tree* Up;
    Tree* Left;
    Tree* Right;

    static Tree* Root;
    static Tree* Cur;
    static bool interpretationEnabled;
    static bool debug;
    static Tree* currentArea;

    Tree(SemNode* node = nullptr, Tree* up = nullptr);
    ~Tree();

    void SetLeft(SemNode* Data);
    void SetRight(SemNode* Data);

    Tree* FindUp(Tree* From, const std::string& id);
    Tree* FindUpOneLevel(Tree* From, const std::string& id);

    Tree* SemInclude(const std::string& a, DATA_TYPE t, int line, int col);
    Tree* SemIncludeConstant(const std::string& a, DATA_TYPE t, const std::string& value, int line, int col);
    void SemSetConst(Tree* Addr, int value);
    void SemSetBasicType(Tree* Addr, DATA_TYPE bt);
    void SemSetArrElemCount(Tree* Addr, int aec);
    void SemSetIndex(Tree* Addr, int index);
    Tree* SemGetVar(const std::string& a, int line, int col);
    bool DupControl(Tree* Addr, const std::string& a);
    Tree* SemEnterBlock(int line, int col);
    void SemExitBlock();

    static void SetCur(Tree* a) { Cur = a; }
    static Tree* GetCur() { return Cur; }

    void Print();
    static void SemError(const std::string& msg, const std::string& id = "", int line = -1, int col = -1);
    static void InterpError(const std::string& msg, const std::string& id = "", int line = -1, int col = -1);
    static void SetVarValue(const std::string& name, const SemNode& value, int line, int col);
    static SemNode GetVarValue(const std::string& name, int line, int col);
    static SemNode ExecuteArithmeticOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col);
    static SemNode ExecuteComparisonOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col);
    static SemNode ExecuteBitwiseOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col);
    static DATA_TYPE GetMaxType(DATA_TYPE t1, DATA_TYPE t2);
    static SemNode CastToType(const SemNode& value, DATA_TYPE targetType, int line, int col, bool showWarning = false);
    static bool CanImplicitCast(DATA_TYPE from, DATA_TYPE to);

    static void EnableInterpretation() { interpretationEnabled = true; }
    static void DisableInterpretation() { interpretationEnabled = false; }
    static bool IsInterpretationEnabled() { return interpretationEnabled; }

    static void EnableDebug() { debug = true; }
    static void DisableDebug() { debug = false; }
    static bool IsDebugEnabled() { return debug; }

    static void PrintDebugInfo(const std::string& message, int line = 0, int col = 0);
    static void PrintAssignment(const std::string& varName, const SemNode& value, int line, int col);
    static void PrintArithmeticOp(const std::string& op, const SemNode& left, const SemNode& right, const SemNode& result, int line, int col);
    static void PrintBitwiseOp(const std::string& op, const SemNode& left, const SemNode& right, const SemNode& result, int line, int col);
    static void PrintTypeConversionWarning(DATA_TYPE from, DATA_TYPE to, const std::string& context, const std::string& expression, int line, int col);

    static void SetCurrentArea(Tree* area) { currentArea = area; }
    static Tree* GetCurrentArea() { return currentArea; }

private:
    void Print(int depth);
    std::string makeLabel(const Tree* tree) const;
};