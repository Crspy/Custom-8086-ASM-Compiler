// 8086_Assembler.cpp : Defines the exported functions for the DLL application.
//

#include "8086_Assembler.h"



void ProcessCompile(const wchar_t* filename, BOOL* pbSortInFolders)
{
    std::ifstream myfile;
    std::string line;
    CROMBlock myrom;
    unsigned long linecount = 0;
    uint32_t PC = 0;

    std::map<std::string, uint32_t> labelsmap;  // storing labels found in jmp instructions and their PC
    std::map<std::string, uint32_t> jmplabelsmap; // storing labels and their PC

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
    wprintf(filename);
    myfile.open(filename, std::ios::in);


    while (std::getline(myfile, line))
    {
        eErrorType errortype = NO_ERROR_DETECTED;
        tInstBlock currentInst[2]; // we use two in case of using an instruction that requires loading address       
        char* linebuff = (char*)malloc(line.capacity());
        strcpy(linebuff, line.c_str());
        char* opToken;
        bool bMovingData = false;
        bool bJmpLabel = false;
        tMemAddress memadd;


        linecount++;
        if (IsBlankLine(line.c_str())
            || IsCommentLine(line))   continue;

        // check if it's IN or OUT opcode
        if (COpcode::GetOpcodeDir(line) == eOpcodeDir::DIR_OUT)
        {
            opToken = strtok(linebuff, " [], \t");
            logger(opToken);
            COpcode::EliminateTabs(opToken);
            if (strcmp(opToken, "mov") == 0)
            {
                errortype = COpcode::ProcessMoveOUT(&memadd, currentInst, linebuff, &bMovingData, &myrom);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }

            }
            else if (strcmp(opToken, "imov") == 0)
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

            if (strcmp(opToken, "mov") == 0)
            {
                errortype = COpcode::ProcessMoveIN(&memadd, currentInst, linebuff);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }
            }
            else if (strcmp(opToken, "imov") == 0)
            {
                errortype = COpcode::ProcessIndirectMoveIN(&memadd, currentInst, linebuff);
                if (errortype != eErrorType::NO_ERROR_DETECTED)
                {
                    CErrorHandler::PrintErrorMessage(errortype, linecount);
                    CloseConsole();
                    return;
                }

            }
            else if (strcmp(opToken, "jmp") == 0)
            {
                logger("FOUND A JUMP");
                errortype = COpcode::ProcessJump(&memadd, currentInst, linebuff, PC, jmplabelsmap, labelsmap);
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
                if (!IsAlphaOnly(opToken))
                {
                    CErrorHandler::PrintErrorMessage(eErrorType::NO_NUMBERS_ALLOWED_IN_A_LABEL, linecount);
                    CloseConsole();
                    return;
                }
                logger("LABEL FOUND!");
                labelsmap.insert(std::pair<std::string, uint32_t>(opToken, PC));
                bJmpLabel = true;
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
        if (memadd.m_bNeedLoading)
        {
            CROMBlock::SetRomInst((tInstBlock*)&myrom.Inst[PC], &currentInst[0]);
            CROMBlock::SetRomInst((tInstBlock*)&myrom.Inst[PC + 1], &currentInst[1]);
            printf("op: %d , dir_flag: %d, regID: %d, Address: %d\n", currentInst[0].opcode, currentInst[0].dir_flag, currentInst[0].reg_id, currentInst[0].address);
            printf("op: %d , dir_flag: %d, regID: %d, Address: %d\n", currentInst[1].opcode, currentInst[1].dir_flag, currentInst[1].reg_id, currentInst[1].address);
            PC += 2;
        }
        else if (!bMovingData && !bJmpLabel)
        {
            CROMBlock::SetRomInst((tInstBlock*)&myrom.Inst[PC], &currentInst[0]);
            printf("op: %d , dir_flag: %d, regID: %d, Address: %d\n", currentInst[0].opcode, currentInst[0].dir_flag, currentInst[0].reg_id, currentInst[0].address);
            PC++;
        }

        if (PC > MAX_PC) // 32768 - 1  
        {
            CErrorHandler::PrintErrorMessage(eErrorType::ROM_INSTRUCTION_SEGMENT_OVERFLOW, linecount);
            CloseConsole();
            return;
        }

    }

    if (!ProcessAllJmpInst(myrom, jmplabelsmap, labelsmap))
        return;

    myfile.close();
    printf("linecount : %d\n", linecount);
    printf("PC COUNT : %d\n", PC);

    CROMBlockHigh highrom;
    CROMBlockLow lowrom;

    for (int i = 0; i < 65536; i++)
    {
        highrom.RomSeg[i].RomHighByte = myrom.RomSeg[i].RomSegHigh;
        lowrom.RomSeg[i].RomLowByte = myrom.RomSeg[i].RomSegLow;
    }



    wchar_t oldDir[WCHAR_MAX];
    wchar_t OutputFolder[WCHAR_MAX];

    if (*pbSortInFolders)
    {
        GetCurrentDirectory(WCHAR_MAX, oldDir);

        lstrcpyW(OutputFolder, filename);

        *wcsstr(OutputFolder, L".asm") = L'\0';

        CreateDirectory(OutputFolder, NULL);
        SetCurrentDirectory(OutputFolder);
    }

    std::ofstream highromfile;
    highromfile.open("high.bin", std::ios::binary | std::ios::out);
    highromfile.write((char*)&highrom, sizeof(highrom));
    highromfile.close();

    std::ofstream lowromfile;
    lowromfile.open("low.bin", std::ios::binary | std::ios::out);    
    lowromfile.write((char*)&lowrom, sizeof(lowrom));        
    lowromfile.close();  

    myfile.close();

    if(*pbSortInFolders)
        SetCurrentDirectory(oldDir);

    printf("Build Done !\n");
    printf("Press Enter to continue...\n");
    getchar();

    CloseConsole();
}


bool ProcessAllJmpInst(CROMBlock& myrom, std::map<std::string, uint32_t>& jmplabelsmap, std::map<std::string, uint32_t>& labelsmap)
{
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
            memadd.InsureJmpAddress(); // doesn't matter needs loading or not because we are loading the MSB of the address anyway
            myrom.Inst[instIndex_PC + 1].address = memadd.byte0; // LSB of address for the jmp opcode
            myrom.Inst[instIndex_PC].address = memadd.byte1;    // MSB of address for the jmp opcode
            // opcodes are already Set correctly in ProcessJump function
        }
        else
        {
            CErrorHandler::PrintErrorMessage(eErrorType::JUMPING_TO_NON_EXISTANT_LABEL, 0, key.c_str());
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
    struct _stat64i32 statbuffer;
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


