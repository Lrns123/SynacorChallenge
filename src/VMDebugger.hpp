#pragma once
#include "SynacorVM.hpp"
#include <map>
#include <set>
#include <vector>

// Interactive VM debugger
class VMDebugger
{
    SynacorVM m_vm;
    std::set<ushort> m_breakpoints;


    using ArgList = std::vector<std::string>;
    struct CommandInfo
    {
        std::string usage;
        std::string description;
        void (VMDebugger::*callback)(const ArgList &);
    };

    using CommandList = std::map<std::string, CommandInfo>;
    static const CommandList commandsList;

public:
    VMDebugger();
    
    void runShell();

private:
    std::vector<std::string> parseCommand(const std::string &command) const;

    ushort printDisassembly(ushort ip);
    ushort disassemble(std::ostream &ss, ushort ip) const;
    void disassembleOperand(std::ostream &ss, ushort operand) const;

    static bool checkStdin();

    // -- Commands --
    void cmdQuit(const ArgList &args);
    void cmdHelp(const ArgList &args);
    void cmdClear(const ArgList &args);
    void cmdReset(const ArgList &args);
    void cmdLoad(const ArgList &args);
    void cmdStep(const ArgList &args);
    void cmdRun(const ArgList &args);
    void cmdReg(const ArgList &args);
    void cmdMem(const ArgList &args);
    void cmdPC(const ArgList &args);
    void cmdDis(const ArgList &args);
    void cmdBreak(const ArgList &args);
    void cmdUnbreak(const ArgList &args);
    void cmdDumpAsm(const ArgList &args);
    void cmdDump(const ArgList &args);
    void cmdStack(const ArgList &args);

};