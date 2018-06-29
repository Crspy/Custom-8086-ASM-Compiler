#pragma once
#include "stdafx.h"

enum eErrorType
{
    NO_ERROR_DETECTED,
    USING_REGNAME_INSTEAD_OF_ADDRESS,
    MEM_ADDRESS_EXCEEDS,
    UNKNOWN_REG_NAME,
    DATA_VALUE_OUTOFBOUNDS,
    UNKNOWN_REG_NAME_OR_NOT_USING_BX_IN_LEFT_OPERAND,
    UNKNOWN_REG_NAME_OR_USING_BX_IN_RIGHT_OPERAND,
    UNKNOWN_REG_NAME_OR_USING_BX_IN_LEFT_OPERAND,
    UNKNOWN_REG_NAME_OR_NOT_USING_BX_IN_RIGHT_OPERAND,
    UNKNOWN_OPCODE,
    ROM_INSTRUCTION_SEGMENT_OVERFLOW,
    ONLY_ADDRESSES_OR_LABELS_ALLOWED,
    LABEL_NAMES_CANT_START_WITH_A_NUMBER,
    USING_SAME_NAME_FOR_TWO_LABELS,
    JUMPING_TO_NON_EXISTANT_LABEL
};

class CErrorHandler
{
public:
    static void PrintErrorMessage(eErrorType errortype, unsigned long linecount, const char* jmpLabel = nullptr);
};