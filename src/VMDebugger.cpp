#include "VMDebugger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>

namespace
{
    class VMQuitException
    {};

    class VMInterruptException
    {};

    class VMBreakPointException
    {};
  
    struct OpcodeInfo
    {
        std::string name;
        size_t operands;
    };

    const std::vector<OpcodeInfo> opcodeTable =
    {
        { "halt", 0 },
        { "set",  2 },
        { "push", 1 },
        { "pop",  1 },
        { "eq",   3 },
        { "gt",   3 },
        { "jmp",  1 },
        { "jt",   2 },
        { "jf",   2 },
        { "add",  3 },
        { "mult", 3 },
        { "mod",  3 },
        { "and",  3 },
        { "or",   3 },
        { "not",  3 },
        { "rmem", 2 },
        { "wmem", 2 },
        { "call", 1 },
        { "ret",  0 },
        { "out",  1 },
        { "in",   1 },
        { "noop", 0 }
    };
}

const VMDebugger::CommandList VMDebugger::commandsList =
{
    { "quit", { "quit", "Quits the interactive debugger.", &VMDebugger::cmdQuit } },
    { "help", { "help [<command>]", "Lists all commands, or shows description of <command>.", &VMDebugger::cmdHelp } },
    { "clear", { "clear", "Clears the VM, wiping all memory.", &VMDebugger::cmdClear } },
    { "reset", { "reset", "Resets the VM, clearing registers and stack, but leaves memory intact.", &VMDebugger::cmdReset} },
    { "load", { "load <filename>", "Loads the binary <filename> at address 0.", &VMDebugger::cmdLoad } },
    { "step", { "step [<count>]", "Executes one or <count> instructions.", &VMDebugger::cmdStep } },
    { "run", { "run", "Executes the program.", &VMDebugger::cmdRun } },
    { "reg", { "reg [<id>] [<value>]", "Shows the value of <id> or all registers, or changes it to <value>.", &VMDebugger::cmdReg } },
    { "mem", { "mem <address> [<value>]", "Shows the value of memory address <address>, or changes it to <value>.", &VMDebugger::cmdMem } },
    { "pc",  { "pc [<address>]", "Shows or changes the program counter to <address>.", &VMDebugger::cmdPC } },
    { "dis", { "dis <address> [<count>]", "Disassembles one or <count> instructions, starting at <address>.", &VMDebugger::cmdDis } },
    { "break", { "break [<address>]", "Adds a breakpoint at <address>, or lists all active breakpoints.", &VMDebugger::cmdBreak } },
    { "unbreak", { "unbreak [<address>]", "Removes a breakpoint at <address>, or removes all active breakpoints.", &VMDebugger::cmdUnbreak } },
    { "dumpasm", { "dumpasm <filename> [<start>] [<end>]", "Dumps the disassembly to <filename>. Optionally starting and ending at <start> and <end>.", &VMDebugger::cmdDumpAsm } },
    { "dump",{ "dump <filename> [<start>] [<end>]", "Dumps the binary to <filename>. Optionally starting and ending at <start> and <end>.", &VMDebugger::cmdDump } },
    { "stack", { "stack", "Shows the current stack.", &VMDebugger::cmdStack } }
};

VMDebugger::VMDebugger()
{
}

void VMDebugger::runShell()
{
    std::cout << "Synacor VM interactive debugger" << std::endl << std::endl;
    std::cout << "For a list of commands, type 'help'." << std::endl;
    std::cout << "To interrupt the VM when running, close stdin (usually Ctrl-Z or Ctrl-D) when the program requests input." << std::endl << std::endl;

    while (true)
    {
        std::string command;
        std::cout << std::hex << "VM> ";
        std::getline(std::cin, command);

        if (checkStdin())
            continue;

        auto cmd = parseCommand(command);        

        if (!cmd.empty())
        {
            auto it = commandsList.find(cmd.front());
            if (it == commandsList.end())
                std::cout << "Unknown command '" << cmd.front() << "'" << std::endl;
            else try
            {
                (this->*(it->second.callback))(cmd);
            }
            catch (const VMQuitException &)
            {
                break;
            }
            catch (const VMInterruptException &)
            {
                std::cout << "VM Interrupted at ";
                printDisassembly(m_vm.instructionPointer());
            }
            catch (const VMBreakPointException &)
            {
                std::cout << "Breakpoint hit at ";
                printDisassembly(m_vm.instructionPointer());
            }
            catch (const std::exception &e)
            {
                std::cout << "Error: " << e.what() << std::endl;
            }            
        }
    }
}

std::vector<std::string> VMDebugger::parseCommand(const std::string& command) const
{
    std::vector<std::string> ret;
    std::istringstream ss(command);

    std::string token;
    while (getline(ss, token, ' '))
        ret.emplace_back(std::move(token));

    return ret;
}

ushort VMDebugger::printDisassembly(ushort ip)
{
    std::cout << std::setfill('0') << std::setw(4) << ip << ": ";
    ushort newIp = disassemble(std::cout, ip);
    std::cout << std::endl;

    return newIp;
}

ushort VMDebugger::disassemble(std::ostream &ss, ushort ip) const
{
    ushort opcode = m_vm.readMemory(ip++);
    if (opcode >= opcodeTable.size())
    {
        ss << "dw ";
        disassembleOperand(ss, opcode);
    }
    else
    {
        auto info = opcodeTable[opcode];
        ss << info.name;
        for (int i = 0; i != info.operands; ++i)
        {
            ss << ' '; 
            disassembleOperand(ss, m_vm.readMemory(ip++));
        }   
    }

    return ip;
}

void VMDebugger::disassembleOperand(std::ostream &ss, ushort operand) const
{
    if (operand < 0x8000)
    {
        ss << operand;
        if (operand < 256)
        {
            ss << " '";
            
            if (operand < 0x20)
                switch (operand)
                {
                case '\n':
                    ss << "\\n";
                    break;
                case '\r':
                    ss << "\\r";
                    break;
                case '\t':
                    ss << "\\t";
                    break;
                default:
                    ss << ' ';
                    break;
                }
            else
                ss << static_cast<char>(operand);


            ss << "'";
        }
            
    }
    else if ((operand & 0x7FFF) < 8)
        ss << "R" << (operand & 0x7FFF);
    else
        ss << "Err(" << operand << ')';
}

bool VMDebugger::checkStdin()
{
    if (std::cin.eof()) // Ctrl-C was pressed
    {
        std::cin.clear();
        std::cin.ignore();
        std::cout << std::endl;
        return true;
    }

    return false;
}

void VMDebugger::cmdQuit(const ArgList& args)
{
    throw VMQuitException();
}

void VMDebugger::cmdHelp(const ArgList& args)
{
    if (args.size() < 2)
    {
        std::cout << "Available commands:" << std::endl;
        for (auto it = commandsList.cbegin(), eit = commandsList.cend(); it != eit; ++it)
            std::cout << it->second.usage << std::endl;
    }
    else
    {
        auto it = commandsList.find(args[1]);
        if (it == commandsList.end())
            std::cout << "Unknown command '" << args[1] << "'." << std::endl;
        else
        {
            std::cout << it->second.usage << std::endl;
            std::cout << it->second.description << std::endl;
        }
    }
}

void VMDebugger::cmdClear(const ArgList& args)
{
    m_vm.clear();
    std::cout << "Virtual machine cleared." << std::endl;
}

void VMDebugger::cmdReset(const ArgList& args)
{
    m_vm.reset();
    std::cout << "Virtual machine reset." << std::endl;
}

void VMDebugger::cmdLoad(const ArgList& args)
{
    if (args.size() < 2)
        std::cout << "Please specify a file name to load." << std::endl;
    else
    {
        ushort words = m_vm.loadBinary(args[1]);
        std::cout << "Binary loaded into VM. (Size: " << std::dec << words << std::hex << " words)" << std::endl;
    }
}

void VMDebugger::cmdStep(const ArgList& args)
{    
    size_t ops = args.size() >= 2 ? std::stoi(args[1], nullptr, 0) : 1;

    while (ops-- && m_vm.step())
    {
        if (m_breakpoints.find(m_vm.instructionPointer()) != m_breakpoints.end())
            throw VMBreakPointException();

        if (checkStdin())
            throw VMInterruptException();
    }

    printDisassembly(m_vm.instructionPointer());
}

void VMDebugger::cmdRun(const ArgList& args)
{
    while (m_vm.step())
    {
        if (m_breakpoints.find(m_vm.instructionPointer()) != m_breakpoints.end())
            throw VMBreakPointException();

        if (checkStdin())
            throw VMInterruptException();
    }
}

void VMDebugger::cmdReg(const ArgList& args)
{
    size_t argc = args.size();
    if (argc < 2)
    {
        // List all registers
        for (int i = 0; i != 8; ++i)
            std::cout << "R" << i << " = 0x" << m_vm.readRegister(i) << std::endl;
    }
    else if (argc < 3)
    {
        int regId = std::stoi(args[1], nullptr, 0);
        if (regId < 0 || regId > 7)
            std::cout << "Invalid register." << std::endl;
        else
            std::cout << "R" << regId << " = 0x" << m_vm.readRegister(regId) << std::endl;
    }
    else
    {
        int regId = std::stoi(args[1], nullptr, 0);
        ushort value = std::stoi(args[2], nullptr, 0) & 0x7FFF;

        if (regId < 0 || regId > 7)
            std::cout << "Invalid register." << std::endl;
        else
        {
            std::cout << "R" << regId << " := 0x" << value << std::endl;
            m_vm.writeRegister(regId, value);
        }
    }
}

void VMDebugger::cmdMem(const ArgList& args)
{
    size_t argc = args.size();
    if (argc < 2)
    {
        std::cout << "Missing address" << std::endl;
    }
    else if (argc < 3)
    {
        int address = std::stoi(args[1], nullptr, 16);
        if (address < 0 || address > 32767)
            std::cout << "Invalid address." << std::endl;
        else
            std::cout << "M[0x" << address << "] = 0x" << m_vm.readMemory(address) << std::endl;
    }
    else
    {
        int address = std::stoi(args[1], nullptr, 16);
        ushort value = std::stoi(args[2], nullptr, 0);

        if (address < 0 || address > 32767)
            std::cout << "Invalid address." << std::endl;
        else
        {
            std::cout << "M[0x" << address << "] := 0x" << value << std::endl;
            m_vm.writeMemory(address, value);
        }
    }
}

void VMDebugger::cmdPC(const ArgList& args)
{
    if (args.size() < 2)
        std::cout << "PC = 0x" << m_vm.instructionPointer() << std::endl;
    else
    {
        ushort address = std::stoi(args[1], nullptr, 16) & 0x7FFF;
        if (address < 0 || address > 32767)
            std::cout << "Invalid address." << std::endl;
        else
        {
            std::cout << "PC := 0x" << address << std::endl;
            m_vm.setInstructionPointer(address);
        }
    }
}

void VMDebugger::cmdDis(const ArgList& args)
{
    if (args.size() < 2)
        std::cout << "Missing address" << std::endl;
    else
    {
        ushort ip = std::stoi(args[1], nullptr, 16);
        int count = args.size() < 3 ? 1 : std::stoi(args[2], nullptr, 0);

        do
        {
            ip = printDisassembly(ip);
            if (ip > 32767)
                break;
        }
        while (--count);
    }
    
}

void VMDebugger::cmdBreak(const ArgList& args)
{
    if (args.size() < 2)
    {
        if (m_breakpoints.empty())
            std::cout << "No breakpoints" << std::endl;
        else
        {
            std::cout << "Breakpoints:" << std::endl;
            for (ushort ip : m_breakpoints)
                printDisassembly(ip);
        }
        
    }
    else
    {
        ushort address = std::stoi(args[1], nullptr, 16) & 0x7FFF;
        if (address < 0 || address > 32767)
            std::cout << "Invalid address." << std::endl;
        else
        {
            m_breakpoints.insert(address);
            std::cout << "Added breakpoint at ";
            printDisassembly(address);
        }
    }
}

void VMDebugger::cmdUnbreak(const ArgList& args)
{
    if (args.size() < 2)
    {
        m_breakpoints.clear();
        std::cout << "Removed all breakpoints" << std::endl;
    }
    else
    {
        ushort address = std::stoi(args[1], nullptr, 16) & 0x7FFF;
        if (address < 0 || address > 32767)
            std::cout << "Invalid address." << std::endl;
        else
        {
            auto it = m_breakpoints.find(address);
            if (it == m_breakpoints.end())
                std::cout << "No breakpoint on address " << address << std::endl;
            else
            {
                std::cout << "Removed breakpoint at " << address << std::endl;
                m_breakpoints.erase(it);
            }
        }
    }
}

void VMDebugger::cmdDumpAsm(const ArgList& args)
{
    if (args.size() < 2)
        std::cout << "Missing filename" << std::endl;
    else
    {
        std::ofstream fs(args[1], std::ios::out);

        if (!fs)
        {
            std::cout << "Cannot open " << args[1] << " for writing" << std::endl;
            return;
        }

        fs << "Synacor VM Disassembly" << std::endl << std::endl;

        ushort start = args.size() < 3 ? 0 : stoi(args[2], nullptr, 16);
        ushort end = args.size() < 4 ? 32768 : stoi(args[3], nullptr, 16);

       
        fs << std::hex << std::setfill('0');

        for (ushort ip = start; ip < end; )
        {
            fs << std::setw(4) << ip << ": ";
            ip = disassemble(fs, ip);
            fs << std::endl;
        }

        fs.close();

        std::cout << "Disassembly dumped to " << args[1] << std::endl;
    }
}

void VMDebugger::cmdDump(const ArgList& args)
{
    if (args.size() < 2)
        std::cout << "Missing filename" << std::endl;
    else
    {
        std::ofstream fs(args[1], std::ios::out | std::ios::binary);

        if (!fs)
        {
            std::cout << "Cannot open " << args[1] << " for writing" << std::endl;
            return;
        }

        ushort start = args.size() < 3 ? 0 : stoi(args[2], nullptr, 16);
        ushort end = args.size() < 4 ? 32768 : stoi(args[3], nullptr, 16);


        for (ushort addr = start; addr < end; ++addr)
        {
            ushort val = m_vm.readMemory(addr);
            fs.write(reinterpret_cast<const char *>(&val), 2);
        }        

        fs.close();

        std::cout << "Binary dumped to " << args[1] << std::endl;
    }
}

void VMDebugger::cmdStack(const ArgList& args)
{
    const auto &stack = m_vm.getStack();

    unsigned int pos = stack.size();
    for (auto it = stack.cbegin(), eit = stack.cend(); it != eit; ++it)
        std::cout << '[' << std::setw(4) << --pos << "] = " << *it << std::endl;
}
