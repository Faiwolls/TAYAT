#pragma once
#include <string>
#include "data_type.h"

struct SemNode {
    std::string id;
    DATA_TYPE DataType;
    bool hasValue = false;

    union {
        int16_t v_int16;
        int32_t v_int32;
        int64_t v_int64;
        bool v_bool;
    } Value;

    int FlagConst;
    DATA_TYPE BasicType;
    int ArrElemCount;
    int index;
    int line;
    int col;
};