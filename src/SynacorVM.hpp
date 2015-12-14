#pragma once

#include <deque>
#include <array>

using ushort = unsigned short;

class SynacorVM final
{

    std::array<ushort, 32768> m_memory;
    std::array<ushort, 8> m_registers;
    unsigned short m_instructionPointer;
    std::deque<ushort> m_stack;
    char m_escapeChar;

public:
    SynacorVM();

    // Resets the VM and wipes memory.
    void clear();

    // Resets the VM without wiping the memory.
    void reset();

    // Executes the next opcode. Returns whether the program should continue running (i.e. the opcode was not HALT).
    bool step();

    // Executes the program until a halt is encountered.
    void run();

    ushort loadBinary(std::string filename);

    // Reads the specified memory address.
    ushort readMemory(ushort address) const;

    // Writes to the specified memory address.
    void writeMemory(ushort address, ushort value);

    // Reads the specified register.
    ushort readRegister(ushort reg) const;

    // Writes to the specified register.
    void writeRegister(ushort reg, ushort value);

    // Returns the instruction pointer.
    ushort instructionPointer() const;

    // Changes the instruction pointer to the specified address.
    void setInstructionPointer(ushort address);

    // Returns the stack.
    const std::deque<ushort> &getStack() const;

    // Returns the escape character. Interrupts execution if typed.
    char escapeChar() const;

    // Sets the escape character. Interrupts execution if typed. Set to 0 to disable.
    void setEscapeChar(char escapeChar);

    // Thrown if the escape character is typed.
    class EscapeCharacterException
    {};

private:
    // Reads the next opcode.
    ushort readOpcode();    

    // Reads the next operand.
    ushort readOperand();

    // Reads the next operand and automatically read register values if necessary.
    ushort readValueOperand();

    // Pushes a value to the stack.
    void push(ushort value);

    // Pops a value from the stack.
    ushort pop();

    // Returns whether the stack is empty.
    bool stackEmpty() const;    
};