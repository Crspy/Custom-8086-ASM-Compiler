#include "COpcode.h"




int8_t COpcode::GetRegID(const char* lineReg)
{
    if (strcmp(lineReg, "ax") == 0)
        return eRegID::AX;
    else if (strcmp(lineReg, "bx") == 0)
        return eRegID::BX;
    else if (strcmp(lineReg, "cx") == 0)
        return eRegID::CX;
    else if (strcmp(lineReg, "dx") == 0)
        return eRegID::DX;
    else
        return -1;
}

void COpcode::EliminateComments(char* line)
{
    char keys[] = "/;";

    char* pch = strpbrk(line, keys);
    while (pch != nullptr)
    {

        strcpy(pch, "\0");
        pch = strpbrk(pch + 1, keys);
    }
}

void COpcode::EliminateTabs(char* line)
{
    char keys[] = "\t";

    char* pch = strpbrk(line, keys);

    while (pch != nullptr)
    {

        strcpy(pch, "\0");
        pch = strpbrk(pch + 1, keys);
    }
}

eOpcodeDir COpcode::GetOpcodeDir(std::string& line)
{
    //std::stringstream ss(line);
    //std::getline(ss, line, ',');
    char* linebuff = static_cast<char*>(malloc(line.capacity())); // new linebuff because the old one is destroyed in tokenization
    strcpy(linebuff, line.c_str());
    char* token = strtok(linebuff,",");

    EliminateComments(token);  

    if (strchr(token,'['))
    {
        free(linebuff);
        return eOpcodeDir::DIR_OUT;
    }

    free(linebuff);
    return eOpcodeDir::DIR_IN;

}


eErrorType COpcode::ProcessMoveIN(tMemAddress* memadd, tInstBlock* currentInst, char* linebuffer,
    std::map<std::string, uint32_t>& constDataMovLabelsMap,uint32_t PC)
{
    currentInst[0].opcode = eOpcode::MOVE_IN;

    currentInst[0].dir_flag = eOpcodeDir::DIR_IN;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);
    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    if (GetRegID(token) >= 0)
    {
        return eErrorType::USING_REGNAME_INSTEAD_OF_ADDRESS;
    }

    if (is_numbers_only(token))
    {
        memadd->m_Address = strtol(token, nullptr, 0);
    
        if (!memadd->InsureMovAddress())
        {
            return eErrorType::MEM_ADDRESS_EXCEEDS;
        }
        if (memadd->m_bNeedLoading)
        {
            currentInst[1].address = memadd->byte0; // lowbyte for mov
            currentInst[1].opcode = currentInst[0].opcode;
            currentInst[1].dir_flag = currentInst[0].dir_flag;
            currentInst[0].opcode = eOpcode::LOAD;
            currentInst[0].dir_flag = eOpcodeDir::DIR_IN; // DirFlag for Loading is 0
            currentInst[0].address = memadd->byte1; // highbyte for load
        }
        else
        {
            currentInst[0].address = memadd->byte0;
        }
    }
    else
    {
        constDataMovLabelsMap.insert(std::pair<std::string,uint32_t>(token,PC));
        memadd->m_bNeedLoading = true; // because the address will be always >= 0x8000
        currentInst[0].opcode = eOpcode::LOAD;
        currentInst[0].dir_flag = eOpcodeDir::DIR_IN; // DirFlag for Loading is 0
        currentInst[1].opcode = currentInst[0].opcode;
        currentInst[1].dir_flag = currentInst[0].dir_flag;
    }


    if (memadd->m_bNeedLoading)
        currentInst[1].reg_id = reg;
    else
        currentInst[0].reg_id = reg;
    return eErrorType::NO_ERROR_DETECTED;
}


eErrorType COpcode::ProcessMoveOUT(tMemAddress* memadd, tInstBlock* currentInst, char* linebuffer,
    bool* bMovingData, CROMBlock* myrom,std::map<std::string, uint32_t>& constDataMovLabelsMap,uint32_t PC)
{
    currentInst[0].opcode = eOpcode::MOVE_OUT;
    currentInst[0].dir_flag = eOpcodeDir::DIR_OUT;

    char * token = strtok(nullptr, " [], \t");
    logger(token);
    EliminateTabs(token);
    if (GetRegID(token) >= 0)
    {
        return eErrorType::USING_REGNAME_INSTEAD_OF_ADDRESS;

    }

    memadd->m_Address = strtol(token, nullptr, 0);
    logger(memadd->m_Address);

    token = strtok(nullptr, " [], \t");
    
    EliminateComments(token); EliminateTabs(token);

    int8_t reg = GetRegID(token);

    if (reg != -1)
    {
        if (is_numbers_only(token))
        {
            memadd->m_Address = strtol(token, nullptr, 0);
        
            if (!memadd->InsureMovAddress())
            {
                return eErrorType::MEM_ADDRESS_EXCEEDS;
            }
            if (memadd->m_bNeedLoading)
            {
                currentInst[1].address = memadd->byte0; // lowbyte for mov
                currentInst[1].opcode = currentInst[0].opcode;
                currentInst[1].dir_flag = currentInst[0].dir_flag;
                currentInst[0].opcode = eOpcode::LOAD;
                currentInst[0].dir_flag = eOpcodeDir::DIR_IN; // DirFlag for Loading is 0
                currentInst[0].address = memadd->byte1; // highbyte for load
            }
            else
            {
                currentInst[0].address = memadd->byte0;
            }
        }
        else
        {
            constDataMovLabelsMap.insert(std::pair<std::string,uint32_t>(token,PC));
            memadd->m_bNeedLoading = true; // because the address will be always >= 0x8000
            currentInst[1].opcode = currentInst[0].opcode;
            currentInst[1].dir_flag = currentInst[0].dir_flag;
            currentInst[0].opcode = eOpcode::LOAD;
            currentInst[0].dir_flag = eOpcodeDir::DIR_IN; // DirFlag for Loading is 0
        }
    }
    else // then it's moving value to dataSeg
    {
        if (!memadd->InsureDataAddress())
        {
            return eErrorType::MEM_ADDRESS_EXCEEDS;
        }

        long lvalue;

        try
        {
            lvalue = strtol(token, nullptr, 0);
            logger("VALUE");
            logger(lvalue);
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << "\n";
            return eErrorType::UNKNOWN_REG_NAME;
        }

        if (lvalue > 0xFFFF)
        {
            return eErrorType::DATA_VALUE_OUTOFBOUNDS;

        }
        else
        {
            *bMovingData = true;
            memadd->m_bNeedLoading = false;

            if (lvalue < 0) // negative number
            {
                if (lvalue >= -32768)
                {
                    const int16_t svalue = lvalue;
                    // swapping endianess
                    *(reinterpret_cast<char*>(&myrom->DataSeg[memadd->m_Address].value) + 1) = *((char*)&svalue);
                    *reinterpret_cast<char*>(&myrom->DataSeg[memadd->m_Address].value) = *(((char*)&svalue + 1));
                }
                else
                    return eErrorType::DATA_VALUE_OUTOFBOUNDS;
            }
            else // zero or positive number 
            {
                const uint16_t usvalue = lvalue;
                // swapping endianess
                *(reinterpret_cast<char*>(&myrom->DataSeg[memadd->m_Address].value) + 1) = *(((char*)&usvalue));
                *reinterpret_cast<char*>(&myrom->DataSeg[memadd->m_Address].value) = *(((char*)&usvalue + 1));
            }

        }

    }
    if (memadd->m_bNeedLoading)
        currentInst[1].reg_id = reg;
    else if (!bMovingData)
        currentInst[0].reg_id = reg;
    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessIndirectMoveOUT(tMemAddress* memadd, tInstBlock* currentInst,
    char* linebuffer)
{
    currentInst[0].opcode = eOpcode::INDIRECT_OUT;

    currentInst[0].dir_flag = eOpcodeDir::DIR_OUT;

    char * token = strtok(nullptr, " [], \t");
    logger(token);
    EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1 || reg2 != eRegID::BX) // then it's moving value to dataSeg
    {
        return eErrorType::UNKNOWN_REG_NAME_OR_NOT_USING_BX_IN_LEFT_OPERAND;
    }

    token = strtok(nullptr, " [], \t");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg = GetRegID(token);
    if (reg == -1 || reg == eRegID::BX) // then it's moving value to dataSeg
    {
        return eErrorType::UNKNOWN_REG_NAME_OR_USING_BX_IN_RIGHT_OPERAND;
    }
    currentInst[0].reg_id = reg;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessIndirectMoveIN(tMemAddress* memadd, tInstBlock* currentInst,
    char* linebuffer)
{
    currentInst[0].opcode = eOpcode::INDIRECT_IN;
    currentInst[0].dir_flag = eOpcodeDir::DIR_IN;

    char * token = strtok(nullptr, " [], \t");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);
    if (reg == -1 || reg == eRegID::BX) // then it's moving value to dataSeg
    {
        return eErrorType::UNKNOWN_REG_NAME_OR_USING_BX_IN_LEFT_OPERAND;
    }

    token = strtok(nullptr, " [], \t");
    logger(token);

    int8_t reg2 = GetRegID(token);
    if (reg2 == -1 || reg2 != eRegID::BX) // then it's moving value to dataSeg
    {
        return eErrorType::UNKNOWN_REG_NAME_OR_NOT_USING_BX_IN_RIGHT_OPERAND;
    }
    currentInst[0].reg_id = reg;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessJump(tMemAddress* memadd, tInstBlock* currentInst, char* linebuffer, uint32_t PC,
    std::map<std::string, uint32_t>& jmplabelsmap, std::map<std::string, uint32_t>& labelsmap/*, std::map<uint32_t, tMemAddress>& memaddressesMap*/)
{
    currentInst[0].opcode = eOpcode::JUMP;


    char * token = strtok(nullptr, " [], \t");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    logger(token);
    if (is_numbers_only(token))
    {
        //logger("Numbers ONLY");
        memadd->m_Address = strtol(token, nullptr, 0);
        logger(token);
        if (!memadd->InsureJmpAddress())
        {
            return eErrorType::MEM_ADDRESS_EXCEEDS;
        }
        if (memadd->m_bNeedLoading)
        {
            currentInst[1].address = memadd->byte0; // lowbyte for mov
            currentInst[1].opcode = currentInst[0].opcode;
           // currentInst[1].dir_flag = currentInst[0].dir_flag;
            currentInst[0].opcode = eOpcode::LOAD;
            //currentInst[0].dir_flag = 0;  
            currentInst[0].address = memadd->byte1; // highbyte for load
        }
        else
        {
            
            currentInst[0].address = memadd->byte0;           
        }
    }
    else if (is_alpha_only(token)) 
    {
        //logger("ALPHA ONLY");
        //std::cin.get();
        memadd->m_bNeedLoading = true; // needs loading by default 
        currentInst[1].opcode = currentInst[0].opcode;
        currentInst[0].opcode = eOpcode::LOAD;
        std::string jmplabel(token);
        jmplabelsmap.insert(std::pair<std::string, uint32_t>(jmplabel, PC));
    }
    else
    {
        return eErrorType::ONLY_ADDRESSES_OR_LABELS_ALLOWED;

    }
    return eErrorType::NO_ERROR_DETECTED;
}


eErrorType COpcode::ProcessConstData(tMemAddress* memadd, tInstBlock* currentInst, std::string& line, uint32_t PC,
        std::map<std::string, uint32_t>& constDataLabelsMap,bool* bMovingData, CROMBlock* myrom)
{

    static uint32_t DataCounter = 0;

    /*
    auto PutStringInDataSeg = [](char* token,CROMBlock* myrom)
    {
        auto toklen = strlen(token);
        for(int i = 0 ; i != toklen; i++)
        {
            const uint16_t usvalue = token[i];
            // swapping endianess
            *(reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) + 1) = *(((char*)&usvalue));
            *reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) = *(((char*)& usvalue + 1));
            DataCounter++;
        }
    };
    */
    *bMovingData = true;
    char* linebuff = static_cast<char*>(malloc(line.capacity())); // new linebuff because the old one is destroyed in tokenization
    strcpy(linebuff, line.c_str());

    char * token = strtok(linebuff, "= \t");
    EliminateTabs(token);
    constDataLabelsMap.insert(std::pair<std::string,uint32_t>(token,DataCounter));

    char * token2 = strtok(nullptr, "= \t");
    bool bShiftPhase = false;
    char* pch = strtok(token2,",");
    while (pch != NULL && *pch != ' ')
    {    
        if (strchr(pch,'\"'))
        {
            char* token3 = strtok(pch, "\"\"");
            auto toklen = strlen(token3); // string length without the nullterm
            logger(toklen);
            for(int i = 0 ; i <= toklen; i++)
            {
                int16_t usvalue = token3[i];
                logger(usvalue);
                // swapping endianess
                printf("DataCount : %d\n",DataCounter);
                *(reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) + 1) = *(((char*)&usvalue));
                *reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) = *(((char*)& usvalue + 1));
                DataCounter++;
            }
            logger(token3);
        }
        else if (strchr(pch,'\''))
        {
            char* token3 = strtok(pch, "''");
            auto toklen = strlen(token3);
            logger(toklen);
            for(int i = 0 ; i < toklen; i++)
            {
                int16_t usvalue = token3[i];
                logger(usvalue);
                // swapping endianess
                printf("DataCount : %d\n",DataCounter);
                *(reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) + 1) = *(char*)&usvalue;
                *reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) = *((char*)& usvalue + 1);
                DataCounter++;
            }
            logger(token3);
        }
        else if (EliminateComments(pch),EliminateTabs(pch),is_numbers_only(pch))
        {
            
            long lvalue = strtol(pch, nullptr, 0);
            if (lvalue < 0) // negative number
            {
                if (lvalue >= -32768)
                {
                    const int16_t svalue = lvalue;
                    // swapping endianess
                    *(reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) + 1) = *(char*)&svalue;
                    *reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) = *((char*)& svalue + 1);
                }
                else
                    return eErrorType::DATA_VALUE_OUTOFBOUNDS;
            }
            else if (lvalue <= 0xFFFF)  // zero or positive number 
            {
                
                const uint16_t usvalue = lvalue;
                // swapping endianess
                printf("%d\n",lvalue);
                *(reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) + 1) = *(char*)&usvalue;
                *reinterpret_cast<char*>(&myrom->DataSeg[DataCounter].value) = *((char*)& usvalue + 1);
            }
            else
                return eErrorType::DATA_VALUE_OUTOFBOUNDS;
            DataCounter++;
        }
        else
        {   // don't add blank data declarations to the map
            constDataLabelsMap.erase(token);
        }

        if (bShiftPhase)
        {
            pch = strtok(pch,",");
            bShiftPhase = false;
        }
        else
        {
            pch = strtok(nullptr,",");
            bShiftPhase = true;
        }
    }

    
    free(linebuff);
    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessNot(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::NOT;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " [], \t");
    printf(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        printf("%d\n",reg);
        return eErrorType::UNKNOWN_REG_NAME;
    }

    currentInst[0].regALU_id = reg;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessTransfer(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::TRANSFER;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    currentInst[0].regALU_id = reg;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessInc(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::INC;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    currentInst[0].regALU_id = reg;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessDec(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::DEC;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    currentInst[0].regALU_id = reg;

    return eErrorType::NO_ERROR_DETECTED;
}


eErrorType COpcode::ProcessXor(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::XOR;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessOR(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::OR;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}


eErrorType COpcode::ProcessAND(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::AND;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessModulus(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::MODULUS;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}


eErrorType COpcode::ProcessAdd(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::ADD;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}


eErrorType COpcode::ProcessSub(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::SUB;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessMul(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::MUL;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}


eErrorType COpcode::ProcessDiv(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::DIV;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}

/*
eErrorType COpcode::ProcessiAdd(tInstBlock* currentInst, char* linebuffer)
{
    int8_t reg;
    int8_t reg2;
    char * token;
    currentInst[0].opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::iADD;


    token = strtok(NULL, " ,[]/");
    logger(token);
    EliminateTabs(token);
    reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(NULL, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}
*/


eErrorType COpcode::ProcessiSub(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::iSUB;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}

/*
eErrorType COpcode::ProcessiMul(tInstBlock* currentInst, char* linebuffer)
{
    int8_t reg;
    int8_t reg2;
    char * token;
    currentInst[0].opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::iMUL;


    token = strtok(NULL, " ,[]/");
    logger(token);
    EliminateTabs(token);
    reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(NULL, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}
*/

eErrorType COpcode::ProcessiDiv(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::iDIV;
    currentInst[0].always_1 = 1;

    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}


eErrorType COpcode::ProcessCompare(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::COMPARE;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg2 = GetRegID(token);
    if (reg2 == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }
    currentInst[0].regALU_id = reg;
    currentInst[0].reg2ALU_id = reg2;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessShiftLeft(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::SHIFT_LEFT;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    currentInst[0].regALU_id = reg;

    return eErrorType::NO_ERROR_DETECTED;
}

eErrorType COpcode::ProcessShiftRight(tInstBlock* currentInst, char* linebuffer)
{
    currentInst[0].const_ALU_opcode = eOpcode::ALU;
    currentInst[0].alu_op = eALUOpcode::SHIFT_RIGHT;
    currentInst[0].always_1 = 1;


    char * token = strtok(nullptr, " ,[]/");
    logger(token);
    EliminateComments(token); EliminateTabs(token);
    int8_t reg = GetRegID(token);

    if (reg == -1)
    {
        return eErrorType::UNKNOWN_REG_NAME;
    }

    currentInst[0].regALU_id = reg;


    return eErrorType::NO_ERROR_DETECTED;
}

bool COpcode::ProcessALUOpcodes(char* opToken,tInstBlock* currentInst, char* linebuff,eErrorType* errortype)
{
    if (strcmp(opToken, "add") == 0)
    {
        *errortype = COpcode::ProcessAdd(currentInst, linebuff);
    }
    else if (strcmp(opToken, "not") == 0)
    {
        *errortype = COpcode::ProcessNot(currentInst, linebuff);
    }
    /*
    else if (strcmp(opToken, "trans") == 0)
    {
        errortype = COpcode::ProcessTransfer(currentInst, linebuff);
    }
    */
    else if (strcmp(opToken, "inc") == 0)
    {
        *errortype = COpcode::ProcessInc(currentInst, linebuff);
    }
    else if (strcmp(opToken, "dec") == 0)
    {
        *errortype = COpcode::ProcessDec(currentInst, linebuff);
    }
    else if (strcmp(opToken, "xor") == 0)
    {
        *errortype = COpcode::ProcessXor(currentInst, linebuff);
    }
    else if (strcmp(opToken, "or") == 0)
    {
        *errortype = COpcode::ProcessOR(currentInst, linebuff);
    }
    else if (strcmp(opToken, "and") == 0)
    {
        *errortype = COpcode::ProcessAND(currentInst, linebuff);
    }
    else if (strcmp(opToken, "mod") == 0)
    {
        *errortype = COpcode::ProcessModulus(currentInst, linebuff);
    }
    else if (strcmp(opToken, "sub") == 0)
    {
        *errortype = COpcode::ProcessSub(currentInst, linebuff);
    }
    else if (strcmp(opToken, "mul") == 0)
    {
        *errortype = COpcode::ProcessMul(currentInst, linebuff);
    }
    else if (strcmp(opToken, "div") == 0)
    {
        *errortype = COpcode::ProcessDiv(currentInst, linebuff);
    }
    /*
    else if (strcmp(opToken, "iadd") == 0)
    {
        errortype = COpcode::ProcessiAdd(currentInst, linebuff);
    }
    }
    */
    else if (strcmp(opToken, "isub") == 0)
    {
        *errortype = COpcode::ProcessiSub(currentInst, linebuff);
    }
    /*
    else if (strcmp(opToken, "imul") == 0)
    {
        errortype = COpcode::ProcessiMul(currentInst, linebuff);
    }
    }
    */
    else if (strcmp(opToken, "idiv") == 0)
    {
        *errortype = COpcode::ProcessiDiv(currentInst, linebuff);
    }
    else if (strcmp(opToken, "shr") == 0)
    {
        *errortype = COpcode::ProcessShiftRight(currentInst, linebuff);
    }
    else if (strcmp(opToken, "shl") == 0)
    {
        *errortype = COpcode::ProcessShiftLeft(currentInst, linebuff);
    }
    else if (strcmp(opToken, "cmp") == 0)
    {
        *errortype = COpcode::ProcessCompare(currentInst, linebuff);
    }
    else
        return false; // not an ALU opcode

    

    return true;
}