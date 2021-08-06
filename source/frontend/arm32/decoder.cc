//
// Created by swift on 1/10/21.
//

#include "decoder.h"

namespace Svm::Decoder {

    constexpr bool IsThumb16(u16 first_part) {
        return (first_part & 0xF800) <= 0xE800;
    }

    A32Decoder::A32Decoder(uint32_t code_addr, IR::Assembler *visitor, Memory::MemoryInterface<u32> *memory, bool thumb)
            : Disassembler(code_addr), visitor(visitor), memory(memory), thumb_mode(thumb), code_start(code_addr) {
        code_address = code_addr;
    }

    std::tuple<u32, A32Decoder::ThumbType> A32Decoder::ReadThumbInstr(u32 arm_pc) {
        u32 first_part = memory->Read<u16>(arm_pc);
        if (IsThumb16(static_cast<u16>(first_part))) {
            return std::make_tuple(first_part, Thumb16);
        }
        u32 second_part = memory->Read<u16>(arm_pc + 2);
        u32 instr_32 = (first_part << 16) | second_part;
        return std::make_tuple(instr_32, Thumb32);
    }

    void A32Decoder::Decode() {
        while (!visitor->Termed()) {
            if (thumb_mode) {
                auto [thumb_instr, thumb_type] = ReadThumbInstr(code_start);
                pc = code_address + 4;
                if (thumb_type == Thumb32) {
                    DecodeT32(thumb_instr);
                    code_address += 4;
                } else {
                    DecodeT32(thumb_instr << 16);
                    code_address += 2;
                }
            } else {
                u32 a32 = memory->Read<u32>(code_address);
                pc = code_address + 8;
                DecodeA32(a32);
                code_address += 4;
            }
        }
    }

    void A32Decoder::UnallocatedT32(u32 instruction) {
        Disassembler::UnallocatedT32(instruction);
    }

    void A32Decoder::UnallocatedA32(u32 instruction) {
        Disassembler::UnallocatedA32(instruction);
    }

    void A32Decoder::UnimplementedT32_16(const char *name, u32 instruction) {
        Disassembler::UnimplementedT32_16(name, instruction);
    }

    void A32Decoder::UnimplementedT32_32(const char *name, u32 instruction) {
        Disassembler::UnimplementedT32_32(name, instruction);
    }

    void A32Decoder::UnimplementedA32(const char *name, u32 instruction) {
        Disassembler::UnimplementedA32(name, instruction);
    }

    void A32Decoder::Unpredictable() {
        Disassembler::Unpredictable();
    }

    void A32Decoder::UnpredictableT32(u32 uint32) {
        Disassembler::UnpredictableT32(uint32);
    }

    void A32Decoder::UnpredictableA32(u32 uint32) {
        Disassembler::UnpredictableA32(uint32);
    }

    void A32Decoder::adc(Condition cond, EncodingSize size, Register rd, Register rn,
                         const Operand &operand) {

    }

    void A32Decoder::adcs(Condition cond, EncodingSize size, Register rd, Register rn,
                          const Operand &operand) {
    }

    void A32Decoder::add(Condition cond, EncodingSize size, Register rd, Register rn,
                         const Operand &operand) {

    }

    void A32Decoder::add(Condition cond, Register rd, const Operand &operand) {
        Disassembler::add(cond, rd, operand);
    }

    void A32Decoder::adds(Condition cond, EncodingSize size, Register rd, Register rn,
                          const Operand &operand) {
        Disassembler::adds(cond, size, rd, rn, operand);
    }

    void A32Decoder::adds(Register rd, const Operand &operand) {
        Disassembler::adds(rd, operand);
    }

    void A32Decoder::addw(Condition cond, Register rd, Register rn, const Operand &operand) {
        Disassembler::addw(cond, rd, rn, operand);
    }

    void A32Decoder::adr(Condition cond, EncodingSize size, Register rd,
                         Disassembler::Location *location) {
        Disassembler::adr(cond, size, rd, location);
    }

}