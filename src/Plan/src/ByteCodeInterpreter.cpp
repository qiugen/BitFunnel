// The MIT License (MIT)

// Copyright (c) 2016, Microsoft

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <iostream>

#include "BitFunnel/Exceptions.h"
#include "ByteCodeInterpreter.h"
#include "LoggerInterfaces/Check.h"


namespace BitFunnel
{
/*
TODO

Pass results processor callback.
Mock row tables.
For loop for slices.
Unit tests.
Replace std::vector with FixedCapacityVector.
Review zero flag.
Figure out how Rank0 instructions read rows.
Address TODO comments.

*/

    //*************************************************************************
    //
    // ByteCodeInterpreter
    //
    //*************************************************************************
    ByteCodeInterpreter::ByteCodeInterpreter(
        ByteCodeGenerator & code,
        uint64_t const * const * rows)
      : m_code(code.GetCode()),
        m_jumpTable(code.GetJumpTable()),
        m_rows(rows)
    {
    }


    void ByteCodeInterpreter::Run(size_t iterationCount)
    {
        for (size_t i = 0; i < iterationCount; ++i)
        {
            RunOneIteration(i);
        }
    }

    void ByteCodeInterpreter::RunOneIteration(size_t iteration)
    {
        m_ip = m_code.data();
        m_offset = iteration;

        while (m_ip->GetOpcode() != Opcode::End)
        {
            const Opcode opcode = m_ip->GetOpcode();
            const unsigned row = m_ip->GetRow();
            const unsigned delta = m_ip->GetDelta();
            const bool inverted = m_ip->IsInverted();

            switch (opcode)
            {
            case Opcode::AndRow:
                {
                    auto rowPtr = m_rows[row];
                    auto x = *(rowPtr + (m_offset >> delta));
                    m_accumulator &= (inverted ? ~x : x);
                    m_zeroFlag = (m_accumulator == 0);
                    m_ip++;
                }
                break;
            case Opcode::LoadRow:
                {
                    auto rowPtr = m_rows[row];
                    auto x = *(rowPtr + (m_offset >> delta));
                    m_accumulator = (inverted ? ~x : x);
                    m_zeroFlag = (m_accumulator == 0);
                    m_ip++;
                }
                break;
            case Opcode::LeftShiftOffset:
                m_offset <<= row;
                m_ip++;
                break;
            case Opcode::RightShiftOffset:
                m_offset >>= row;
                m_ip++;
                break;
            case Opcode::IncrementOffset:
                m_offset++;
                m_ip++;
                break;
            case Opcode::Push:
                m_valueStack.push_back(m_accumulator);
                m_ip++;
                break;
            case Opcode::Pop:
                m_accumulator = m_valueStack.back();
                m_valueStack.pop_back();
                m_ip++;
                break;
            case Opcode::AndStack:
                {
                    auto left = m_valueStack.back();
                    m_valueStack.pop_back();
                    m_valueStack.back() &= left;
                    m_ip++;
                }
                break;
            case Opcode::Constant:
                throw NotImplemented("Constant opcode not implemented.");
                //m_valueStack.push_back(row);
                //m_ip++;
                //break;
            case Opcode::Not:
                m_valueStack.back() = ~m_valueStack.back();
                m_ip++;
                break;
            case Opcode::OrStack:
                {
                    auto left = m_valueStack.back();
                    m_valueStack.pop_back();
                    m_valueStack.back() |= left;
                    m_ip++;
                }
                break;
            case Opcode::UpdateFlags:
                m_zeroFlag = (m_valueStack.back() == 0);
                m_ip++;
                break;
            case Opcode::Report:
                // TODO: Combine accumulator with value stack.
                std::cout
                    << "Report("
                    << std::hex
                    << m_accumulator
                    << ", "
                    << m_offset
                    << ")"
                    << std::endl;
                m_ip++;
                break;
            case Opcode::Call:
                m_callStack.push_back(m_ip);
                m_ip = m_jumpTable[row];
                break;
            case Opcode::Jmp:
                m_ip = m_jumpTable[row];
                break;
            case Opcode::Jnz:
                if (m_accumulator != 0ull)
                {
                    m_ip = m_jumpTable[row];
                }
                else
                {
                    m_ip++;
                }
                break;
            case Opcode::Jz:
                if (m_accumulator == 0ull)
                {
                    m_ip = m_jumpTable[row];
                }
                else
                {
                    m_ip++;
                }
                break;
            case Opcode::Return:
                m_ip = m_callStack.back();
                m_callStack.pop_back();
                break;
            default:
                RecoverableError error("ByteCodeInterpreter:: bad opcode.");
                throw error;
            }
        }
    }


    //*************************************************************************
    //
    // ByteCodeGenerator
    //
    //*************************************************************************
    ByteCodeGenerator::ByteCodeGenerator()
        : m_sealed(false)
    {
    }


    void ByteCodeGenerator::Seal()
    {
        EnsureSealed(false);

        // Mark the end of the generated code.
        m_code.emplace_back(ByteCodeInterpreter::Opcode::End);

        // Verify that all labels have been placed.
        for (size_t i = 0; i < m_jumpTable.size(); ++i)
        {
            CHECK_NE(m_jumpTable[i], nullptr)
                << "Label " << i << " has not been placed.";
        }

        m_sealed = true;
    }


    std::vector<ByteCodeInterpreter::Instruction> const &
        ByteCodeGenerator::GetCode() const
    {
        EnsureSealed(true);
        return m_code;
    }


    std::vector<ByteCodeInterpreter::Instruction const *> const &
        ByteCodeGenerator::GetJumpTable() const
    {
        EnsureSealed(true);
        return m_jumpTable;
    }


    void ByteCodeGenerator::AndRow(size_t row, bool inverted, size_t rankDelta)
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::AndRow, row, rankDelta, inverted);
    }


    void ByteCodeGenerator::LoadRow(size_t row, bool inverted, size_t rankDelta)
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::LoadRow, row, rankDelta, inverted);
    }


    void ByteCodeGenerator::LeftShiftOffset(size_t shift)
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::LeftShiftOffset, shift);
    }


    void ByteCodeGenerator::RightShiftOffset(size_t shift)
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::RightShiftOffset, shift);
    }


    void ByteCodeGenerator::IncrementOffset()
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::IncrementOffset);
    }


    void ByteCodeGenerator::Push()
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::Push);
    }


    void ByteCodeGenerator::Pop()
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::Pop);
    }


    void ByteCodeGenerator::AndStack()
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::AndStack);
    }


    void ByteCodeGenerator::Constant(int /*value*/)
    {
        EnsureSealed(false);
        throw NotImplemented("Constant opcode not implemented.");
    }


    void ByteCodeGenerator::Not()
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::Not);
    }


    void ByteCodeGenerator::OrStack()
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::OrStack);
    }


    void ByteCodeGenerator::UpdateFlags()
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::UpdateFlags);
    }


    void ByteCodeGenerator::Report()
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::Report);
    }


    ICodeGenerator::Label ByteCodeGenerator::AllocateLabel()
    {
        EnsureSealed(false);
        Label label = static_cast<Label>(m_jumpTable.size());
        m_jumpTable.push_back(nullptr);
        return label;
    }


    void ByteCodeGenerator::PlaceLabel(Label label)
    {
        EnsureSealed(false);
        CHECK_NE(m_jumpTable.at(label), nullptr)
            << "Label " << label << " has already been placed.";

        m_jumpTable.at(label) = (&m_code.back()) + 1;
    }


    void ByteCodeGenerator::Call(Label label)
    {
        EnsureSealed(false);
        CHECK_LT(label, m_jumpTable.size())
            << "Call to unknown label " << label;

        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::Call, label);
    }


    void ByteCodeGenerator::Jmp(Label label)
    {
        EnsureSealed(false);
        CHECK_LT(label, m_jumpTable.size())
            << "Jmp to unknown label " << label;

        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::Jmp, label);
    }


    void ByteCodeGenerator::Jnz(Label label)
    {
        EnsureSealed(false);
        CHECK_LT(label, m_jumpTable.size())
            << "Jnz to unknown label " << label;

        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::Jnz, label);
    }


    void ByteCodeGenerator::Jz(Label label)
    {
        EnsureSealed(false);
        CHECK_LT(label, m_jumpTable.size())
            << "Jz to unknown label " << label;

        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::Jz, label);
    }


    void ByteCodeGenerator::Return()
    {
        EnsureSealed(false);
        m_code.emplace_back(
            ByteCodeInterpreter::Opcode::Return);
    }


    void ByteCodeGenerator::EnsureSealed(bool sealed) const
    {
        CHECK_EQ(sealed, m_sealed)
            << (sealed ? "class must be sealed." :
                         "class already sealed.");
    }
}