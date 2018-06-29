/*
    Custom 8086 ASM Compiler
    Author : Crspy
    Do not delete this comment block. Respect others' work!
*/


#include "8086_Assembler.h"



void ProcessCompile(const wchar_t* filename, BOOL bSortInFolders)
{
    std::ifstream myfile;
    std::string line;
    CROMBlock myrom;
    unsigned long linecount = 0;
    uint32_t PC = 0;

    std::map<std::string, uint32_t> labelsmap;  // storing labels found in jmp instructions and their PC
    std::map<uint32_t,  std::string> jmplabelsmap; // storing labels and their PC
    std::map<std::string, uint32_t> constDataLabelsMap;
    std::map<uint32_t,  std::string> constDataMovLabelsMap;

    if (!DoesFileExist(filename))
    {
        CloseConsole();
        return;
    }
    auto pos = wcslen(filename);
    if (filename[pos - 4] != '.'
        || (filename[pos - 3] != L'a' && filename[pos - 3] != L'A')
        || (filename[pos - 2] != L's' && filename[pos - 2] != L'S')
        || (filename[pos - 1] != L'm' && filename[pos - 1] != L'M'))
    {
        printf("Unknown extension , only .asm is supported\n");
        printf("Press Enter to continue...\n");
        getchar();
        CloseConsole();
        return;
    }
    wprintf(L"%ws\n",filename);
    myfile.open(filename, std::ios::in);


    char* linebuff = nullptr;
    while (std::getline(myfile, line))
    {
        eErrorType errortype = NO_ERROR_DETECTED;
        tInstBlock currentInst[4];     
        linebuff = static_cast<char*>(malloc(line.capacity()));
        strcpy(linebuff, line.c_str());
        char* opToken;
        bool bMovingData = false;
        bool bJmpLabel = false;
        bool bDoubleJmp = false;
        tMemAddress memadd;


        linecount++;
        if (is_blank_line(line.c_str())
            || IsCommentLine(line))   continue;

        // check if it's IN or OUT opcode
        if (COpcode::GetOpcodeDir(line) == eOpcodeDir::DIR_OUT)
        {
            opToken = strtok(linebuff, " [], \t");
            logger(opToken);
            COpcode::EliminateTabs(opToken);
            if (strcmpi(opToken, "mov") == 0)
            {
                errortype = COpcode::ProcessMoveOUT(&memadd, currentInst, linebuff, &bMovingData, &myrom,constDataMovLabelsMap,PC);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }

            }
            else if (strcmpi(opToken, "imov") == 0)
            {
                errortype = COpcode::ProcessIndirectMoveOUT(&memadd, currentInst, linebuff);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }
            }
            else
            {
                CErrorHandler::PrintErrorMessage(eErrorType::UNKNOWN_OPCODE, linecount);
                CloseConsole();
                return;
            }

        }
        else
        {
            opToken = strtok(linebuff, " ,[]/");
            logger(opToken);
            COpcode::EliminateTabs(opToken);

            if (strcmpi(opToken, "mov") == 0)
            {
                logger("MOVE IN");
                errortype = COpcode::ProcessMoveIN(&memadd, currentInst, linebuff,constDataMovLabelsMap,PC);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }
            }
            else if (strcmpi(opToken, "imov") == 0)
            {
                errortype = COpcode::ProcessIndirectMoveIN(&memadd, currentInst, linebuff);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }

            }
            else if (strcmpi(opToken, "iout") == 0)
            {
                errortype = COpcode::ProcessInstOut(currentInst, linebuff);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }

            }
            else if (strcmpi(opToken, "dout") == 0)
            {
                errortype = COpcode::ProcessDataOut(currentInst, linebuff);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }

            }
            else if (strcmpi(opToken, "nop") == 0)
            {
                errortype = COpcode::ProcessNoOperation(currentInst);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }

            }
            else if(CBranching::ProcessBranchingOpcodes(opToken,&memadd,currentInst,linebuff,&errortype,PC,
                jmplabelsmap,&bDoubleJmp))
            {
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }
            }
            else if (COpcode::ProcessALUOpcodes(opToken, currentInst, linebuff, &errortype))
            {
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }
            }
            else if (strchr(opToken, ':')) // it's a label !
            {
                char* labeltr = strchr(opToken, ':');
                *labeltr = '\0'; // terminate the label
                logger (opToken);
                if (DoesStringStartWithNumber(opToken))
                {
                    CErrorHandler::PrintErrorMessage(eErrorType::LABEL_NAMES_CANT_START_WITH_A_NUMBER, linecount);
                    CloseConsole();
                    return;
                }
                if (labelsmap.find(opToken) != labelsmap.end())
                {
                    CErrorHandler::PrintErrorMessage(eErrorType::USING_SAME_NAME_FOR_TWO_LABELS, linecount,opToken);
                    CloseConsole();
                    return;                   
                }
                logger("LABEL FOUND!");
                   
                labelsmap.insert(std::pair<std::string, uint32_t>(opToken, PC));
                bJmpLabel = true;
            }
            else if (strchr(line.c_str(), '='))
            {
                errortype = COpcode::ProcessConstData(&memadd,currentInst,line ,PC,constDataLabelsMap,&bMovingData,&myrom);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }
            }
            else
            {
                CErrorHandler::PrintErrorMessage(eErrorType::UNKNOWN_OPCODE, linecount);
                CloseConsole();
                return;
            }

        }
        //printf("PC COUNT : %d\n", PC);
        //printf("%d\n",memadd.m_Address);

        //printf("%d\n", reg);

        printf("NeedsLoading : %s\n", (memadd.m_bNeedLoading ? "true" : "false"));
        if (memadd.m_bNeedLoading && bDoubleJmp)
        {
            printf("PC INCREMENTED 4\n");
            myrom.Inst[PC].instblock = currentInst[0];
            myrom.Inst[PC+1].instblock = currentInst[1];
            myrom.Inst[PC+2].instblock = currentInst[2];
            myrom.Inst[PC+3].instblock = currentInst[3];
            PC += 4;
        }
        else if (memadd.m_bNeedLoading || bDoubleJmp)
        {
            printf("PC INCREMENTED 2\n");
            myrom.Inst[PC].instblock = currentInst[0];
            myrom.Inst[PC+1].instblock = currentInst[1];
            printf("op: %d , dir_flag: %d, regID: %d, Address: %d\n", currentInst[0].opcode, currentInst[0].dir_flag, currentInst[0].reg_id, currentInst[0].address);
            printf("op: %d , dir_flag: %d, regID: %d, Address: %d\n", currentInst[1].opcode, currentInst[1].dir_flag, currentInst[1].reg_id, currentInst[1].address);
            PC += 2;
        }
        else if (!bMovingData && !bJmpLabel)
        {
            printf("PC INCREMENTED 1\n");
            myrom.Inst[PC].instblock = currentInst[0];
            printf("op: %d , dir_flag: %d, regID: %d, Address: %d\n", currentInst[0].opcode, currentInst[0].dir_flag, currentInst[0].reg_id, currentInst[0].address);
            PC++;
        }

        if (PC > MAX_PC) // 32768 - 1  
        {
            CErrorHandler::PrintErrorMessage(eErrorType::ROM_INSTRUCTION_SEGMENT_OVERFLOW, linecount);
            CloseConsole();
            return;
        }

        printf("PC COUNTER : %d\n", PC);
    }

    if (linebuff)   free(linebuff);

    if (!ProcessAllJmpInst(&myrom, &jmplabelsmap, &labelsmap))
        return;


    if (!ProcessAllConstDataMovInst(&myrom,&constDataMovLabelsMap,&constDataLabelsMap))
        return;

    printf("linecount : %d\n", linecount);
    printf("PC COUNT : %d\n", PC);

    CROMBlockHigh highrom;
    CROMBlockLow lowrom;

    for (int i = 0; i <= 0xFFFF ; i++)
    {
        highrom.RomSeg[i].RomHighByte = myrom.RomSeg[i].RomSegHigh;
        lowrom.RomSeg[i].RomLowByte = myrom.RomSeg[i].RomSegLow;
    }



    wchar_t oldDir[WCHAR_MAX];
    wchar_t OutputFolder[WCHAR_MAX];

    if (bSortInFolders)
    {
        GetCurrentDirectory(WCHAR_MAX, oldDir);

        lstrcpyW(OutputFolder, filename);

        *wcsstr(OutputFolder, L".asm") = L'\0';

        CreateDirectory(OutputFolder, nullptr);
        SetCurrentDirectory(OutputFolder);
    }

    /*
    FILE *highromfile = fopen("high.bin","wb+");  // w for write, b for binary
    fwrite(&highrom,sizeof(CROMBlockHigh),1,highromfile);
    fclose(highromfile);

    FILE *lowromfile = fopen("low.bin","wb+");  // w for write, b for binary
    fwrite(&lowrom,sizeof(CROMBlockLow),1,lowromfile);
    fclose(lowromfile);
    */

    std::ofstream highromfile;
    highromfile.open("high.bin", std::ios::binary | std::ios::out);
    highromfile.write(reinterpret_cast<char*>(&highrom), sizeof(highrom));
    highromfile.close();

    std::ofstream lowromfile;
    lowromfile.open("low.bin", std::ios::binary | std::ios::out);    
    lowromfile.write(reinterpret_cast<char*>(&lowrom), sizeof(lowrom));        
    lowromfile.close(); 

    myfile.close();

    if(bSortInFolders)
        SetCurrentDirectory(oldDir);

    printf("Build Done !\n");
    printf("Press Enter to continue...\n");
    getchar();

    CloseConsole();
}


bool ProcessAllJmpInst(CROMBlock* myrom, std::map<uint32_t , std::string>* jmplabelsmap, 
    std::map<std::string, uint32_t>* labelsmap)
{
    logger("ProcessAllJmpInst");
    for (std::map<uint32_t , std::string>::iterator iter = jmplabelsmap->begin(); iter != jmplabelsmap->end(); ++iter)
    {
        uint32_t instIndex_PC = iter->first;
        std::string value_label = jmplabelsmap->at(instIndex_PC);
        if (labelsmap->find(value_label) != labelsmap->end())
        {
            tMemAddress memadd(labelsmap->at(value_label));// PC * 2 byte to obtain address of label           
            logger(value_label);
            logger(instIndex_PC);
            logger(memadd.m_Address);
            memadd.InsureJmpAddress(); // doesn't matter needs loading or not because we are loading the MSB of the address anyway
            printf(">%d<\n",memadd.byte0);
            printf(">%d<\n",memadd.byte1);
            myrom->Inst[instIndex_PC + 1].instblock.JMPaddress = memadd.byte0; // LSB of address for the jmp opcode
            myrom->Inst[instIndex_PC].instblock.JMPaddress = memadd.byte1;    // MSB of address for the jmp opcode
            // opcodes are already Set correctly in ProcessJump function
        }
        else
        {
            CErrorHandler::PrintErrorMessage(eErrorType::JUMPING_TO_NON_EXISTANT_LABEL, 0, value_label.c_str());
            CloseConsole();
            return false;
        }
    }
    return true;
}


bool ProcessAllConstDataMovInst(CROMBlock* myrom, std::map<uint32_t , std::string>* constDataMovLabelsMap, 
    std::map<std::string, uint32_t>* constDataLabelsMap)
{
    logger("ProcessAllConstDataMovInst");
    for (std::map<uint32_t , std::string>::iterator iter = constDataMovLabelsMap->begin(); iter != constDataMovLabelsMap->end(); ++iter)
    {
        uint32_t instIndex_PC = iter->first;
        std::string value_label = constDataMovLabelsMap->at(instIndex_PC);
        logger(value_label);
        if (constDataLabelsMap->find(value_label) != constDataLabelsMap->end())
        {
            tMemAddress memadd(constDataLabelsMap->at(value_label)); // memory address of labels
            logger(value_label);
            logger(instIndex_PC);          
            memadd.InsureMovAddress();
            logger(memadd.m_Address);
            myrom->Inst[instIndex_PC + 1].instblock.address = memadd.byte0; // LSB of address for the mov opcode
            myrom->Inst[instIndex_PC].instblock.address = memadd.byte1;    // MSB of address for the mov opcode
            // opcodes are already Set correctly in Process Mov functions
        }
        else
        {
            CErrorHandler::PrintErrorMessage(eErrorType::DATA_LABEL_NOT_FOUND, 0, value_label.c_str());
            CloseConsole();
            return false;
        }
    }
    return true;
}

bool IsCommentLine(std::string& line)
{
    const char* linebuff = line.c_str();
    for (size_t i = 0; i < line.capacity(); i++)
    {
        char first = linebuff[i];
        if (first != ' '
            && first != '/'
            && first != '\t'
            && first != '\n'
            && first != ';')
            return false;

        if (first == '/' || first == ';')
        {
            return true;
        }

    }
    return false;
}

bool DoesFileExist(const wchar_t* filename)
{
    struct _stat statbuffer;

    if (_wstat(filename, &statbuffer) != 0)
    {
        printf("File Not Found\n");
        printf("Press Enter to continue...\n");
        getchar();
        return false;
    }
    return true;
}

void CloseConsole()
{
    fclose(stdin);
    fclose(stdout);
    FreeConsole();
}




/*
void ProcessAllJmpInst(CROMBlock* myrom, std::map<std::string, uint32_t>& jmplabelsmap, std::map<std::string, uint32_t>& labelsmap,
    uint32_t* PC , unsigned long linecount, std::map<uint32_t, bool>& jmpAddressesMap)
{
    uint32_t extraPC = 0;
    for (std::map<std::string, uint32_t>::iterator iter = jmplabelsmap.begin(); iter != jmplabelsmap.end(); ++iter)
    {
        std::string key = iter->first;
        if (labelsmap.find(key) != labelsmap.end())
        {
            
            tMemAddress memadd(labelsmap[key] * 2);// PC * 2 byte to obtain address of label
            uint32_t instIndex_PC = jmplabelsmap[key];
            logger(key);
            logger(instIndex_PC);
            logger(memadd.m_Address);
            memadd.InsureAddress();
            if (memadd.m_bNeedLoading) // :(
            {
                jmpAddressesMap.insert(std::pair<uint32_t, bool>(instIndex_PC, true));
                // pushing all instructions starting from that jmp inst by one inst !          
                for (unsigned int i = *PC; i >= instIndex_PC; i--)
                {
                    tInstBlock* instblock = (tInstBlock*)&myrom->Inst[i];
                    *reinterpret_cast<tInstBlock*>(&myrom->Inst[i + 1]) = *instblock;
                }
                if (labelsmap[key] > instIndex_PC)
                {
                    // add 2 bytes as our instructions has been shifted forward by 2 bytes and that label exist after among them
                    memadd.m_Address += 2; 
                }
                myrom->Inst[instIndex_PC + extraPC + 1].address = memadd.byte0; // lowbyte of address for the jmp opcode
                myrom->Inst[instIndex_PC + extraPC].opcode = eOpcode::LOAD;
                myrom->Inst[instIndex_PC + extraPC].address = memadd.byte1;
                for (std::map<std::string, uint32_t>::iterator _iter = jmplabelsmap.begin(); _iter != jmplabelsmap.end(); ++_iter)
                {
                    std::string _key = _iter->first;
                    uint32_t instIndex = jmplabelsmap[_key]; // without the extra :3
                    uint32_t labelindex = labelsmap[_key];
                    if (labelindex > instIndex_PC) // it was after that inst , so we have to edit the inst that do the jumping
                    {
                        if (instIndex > instIndex_PC) // was it after that inst ? so we have to add 1
                        {
                            if (jmpAddressesMap.find(instIndex) != jmpAddressesMap.end()) // does it exist in the map? then it needed loading
                            {
                                if (myrom->Inst[instIndex + extraPC + 1].address > 0xFD)
                                {
                                    tMemAddress tempMemaddress;
                                    tempMemaddress.byte0 = myrom->Inst[instIndex + extraPC + 1].address;
                                    tempMemaddress.byte1 = myrom->Inst[instIndex + extraPC].address;
                                    tempMemaddress.m_Address += 2;
                                    myrom->Inst[instIndex + extraPC + 1].address = tempMemaddress.byte0;
                                    myrom->Inst[instIndex + extraPC].address = tempMemaddress.byte1;
                                }
                                else
                                    myrom->Inst[instIndex + extraPC + 1].address += 2;

                            }
                            else // didn't need loading :3
                            {
                                if (myrom->Inst[instIndex + extraPC].address <= 0xFD)
                                {
                                    myrom->Inst[instIndex + extraPC].address += 2;
                                }
                                else // damn it , now it needs loading
                                {
                                    printf("Compiler Error, Can't Determine Label \"%s\" memory address,\nplease add an instruction before the label as a workaround to fix this problem", _key);
                                    printf("Press Enter To continue....");
                                    std::cin.get();
                                }

                            }
                        }
                    }

                }
                extraPC++;
                if ((*PC + extraPC) > MAX_PC) // 32768 - 1  
                {
                    CErrorHandler::PrintErrorMessage(eErrorType::ROM_INSTRUCTION_SEGMENT_OVERFLOW, linecount);
                    CloseConsole();
                }
            }
            else // doesn't need loading
            {
                myrom->Inst[instIndex_PC].address = memadd.byte0;
            }
        }
        else
        {
            CErrorHandler::PrintErrorMessage(eErrorType::JUMPING_TO_NON_EXISTANT_LABEL, 0, key.c_str());
            CloseConsole();
        }
    }

    PC += extraPC;
}
*/


