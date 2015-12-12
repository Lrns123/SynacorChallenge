#include "SynacorVM.hpp"
#include <iostream>
#include <fstream>

SynacorVM::SynacorVM()
{
    reset();
}

ushort SynacorVM::loadBinary(std::string filename)
{
    clear();

    std::ifstream fi(filename, std::ios::in | std::ios::binary);
    if (!fi)
        throw std::runtime_error("Could not open binary");

    ushort address = 0;

    // TODO: Endianness
    while (fi)
        fi.read(reinterpret_cast<char *>(&m_memory[address++]), 2);

    return address;
}

ushort SynacorVM::readMemory(ushort address) const
{
    if ((address & 0x8000) == 0)
        return m_memory[address];

    ushort reg = address & 0x7FFF;
    if (reg > 7)
        throw std::out_of_range("Attempted to read from invalid memory address.");

    return m_registers[reg];
}

void SynacorVM::writeMemory(ushort address, ushort value)
{
    if ((address & 0x8000) == 0)
    {
        m_memory[address] = value;
        return;
    }

    ushort reg = address & 0x7FFF;
    if (reg > 7)
        throw std::out_of_range("Attempted to write to invalid memory address.");

    m_registers[reg] = value;
}

ushort SynacorVM::readRegister(ushort reg) const
{
    if (reg > 7)
        throw std::out_of_range("Invalid register index");

    return m_registers[reg];
}

void SynacorVM::writeRegister(ushort reg, ushort value)
{
    if (reg > 7)
        throw std::out_of_range("Invalid register index");

    m_registers[reg] = value;
}

void SynacorVM::clear()
{
    m_memory.fill(0);
    reset();
}

void SynacorVM::reset()
{
    m_registers.fill(0);
    m_stack.clear();

    m_instructionPointer = 0;
}

bool SynacorVM::step()
{
    ushort opcode = readOpcode();

    switch (opcode)
    {
    case 0: /* HALT */
        return false;
    case 1: /* SET */
    {
        ushort address = readOperand();
        ushort value = readValueOperand();

        if (!(address & 0x8000))
            throw std::runtime_error("Operand to SET is not a register");

        writeRegister(address & 0x7FFF, value);
        return true;
    }
    case 2: /* PUSH */
    {
        ushort value = readValueOperand();

        push(value);
        return true;
    }
    case 3: /* POP */
    {
        ushort address = readOperand();
        ushort value = pop();

        writeMemory(address, value);
        return true;
    }
    case 4: /* EQ */
    {
        ushort address = readOperand();
        ushort lhs = readValueOperand();
        ushort rhs = readValueOperand();

        writeMemory(address, lhs == rhs ? 1 : 0);
        return true;
    }
    case 5: /* GT */      
    {
        ushort address = readOperand();
        ushort lhs = readValueOperand();
        ushort rhs = readValueOperand();

        writeMemory(address, lhs > rhs ? 1 : 0);
        return true;
    }
    case 6: /* JMP */
    {
        ushort address = readValueOperand();

        setInstructionPointer(address);
        return true;
    }
    case 7: /* JT */
    {
        ushort value = readValueOperand();
        ushort jumpAddress = readValueOperand();

        if (value)
            setInstructionPointer(jumpAddress);
        return true;
    }
    case 8: /* JF */ 
    {
        ushort value = readValueOperand();
        ushort jumpAddress = readValueOperand();

        if (!value)
            setInstructionPointer(jumpAddress);
        return true;
    }
    case 9: /* ADD */
    {
        ushort address = readOperand();
        ushort lhs = readValueOperand();
        ushort rhs = readValueOperand();

        writeMemory(address, (lhs + rhs) % 32768);
        return true;
    }
    case 10: /* MULT */
    {
        ushort address = readOperand();
        ushort lhs = readValueOperand();
        ushort rhs = readValueOperand();

        // NOTE: May need extension to int to avoid overflow weirdery
        writeMemory(address, (lhs * rhs) % 32768);
        return true;
    }
    case 11: /* MOD */
    {
        ushort address = readOperand();
        ushort lhs = readValueOperand();
        ushort rhs = readValueOperand();

        writeMemory(address, lhs % rhs);
        return true;
    }
    case 12: /* AND */
    {
        ushort address = readOperand();
        ushort lhs = readValueOperand();
        ushort rhs = readValueOperand();

        writeMemory(address, lhs & rhs);
        return true;
    }
    case 13: /* OR */
    {
        ushort address = readOperand();
        ushort lhs = readValueOperand();
        ushort rhs = readValueOperand();

        writeMemory(address, lhs | rhs);
        return true;
    }
    case 14: /* NOT */
    {
        ushort address = readOperand();
        ushort rhs = readValueOperand();

        writeMemory(address, ~rhs & 0x7FFF);
        return true;
    }
    case 15: /* RMEM */
    {
        ushort address = readOperand();
        ushort valueAddress = readValueOperand();

        writeMemory(address, readMemory(valueAddress));
        return true;
    }
    case 16: /* WMEM */
    {
        ushort address = readValueOperand();
        ushort value = readValueOperand();

        writeMemory(address, value);
        return true;
    }
    case 17: /* CALL */
    {
        ushort address = readValueOperand();

        push(instructionPointer());
        setInstructionPointer(address);

        return true;
    }
    case 18: /* RET */
    {
        if (stackEmpty())
            return false;

        setInstructionPointer(pop());
        return true;
    }
    case 19: /* OUT */
    {
        ushort ascii = readValueOperand();

        std::cout << static_cast<char>(ascii);
        return true;
    }
    case 20: /* IN */
    {
        ushort address = readOperand();

        writeMemory(address, std::cin.get());
        return true;
    }
    case 21: /* NOOP */
        return true;
    default:
        throw std::runtime_error("Unknown opcode");
    }

}

void SynacorVM::run()
{
    while (step());
}

void SynacorVM::push(ushort value)
{
    m_stack.push_front(value);
}

ushort SynacorVM::pop()
{
    if (m_stack.empty())
        throw std::underflow_error("Stack underflow");

    ushort val = m_stack.front();
    m_stack.pop_front();

    return val;
}

bool SynacorVM::stackEmpty() const
{
    return m_stack.empty();
}

const std::deque<ushort>& SynacorVM::getStack() const
{
    return m_stack;
}

ushort SynacorVM::instructionPointer() const
{
    return m_instructionPointer;
}

void SynacorVM::setInstructionPointer(ushort address)
{
    m_instructionPointer = address;
}

ushort SynacorVM::readOpcode()
{
    return readMemory(m_instructionPointer++);
}

ushort SynacorVM::readOperand()
{
    return readMemory(m_instructionPointer++);
}

ushort SynacorVM::readValueOperand()
{
    ushort operand = readMemory(m_instructionPointer++);

    if (!(operand & 0x8000))
        return operand; 

    ushort reg = operand & 0x7FFF;
    if (reg > 7)
        throw std::out_of_range("Invalid operand");

    return m_registers[reg];
}
