//
// Created by swift on 1/10/21.
//

#include <base/marco.h>
#include <include/memory_interface.h>
#include <frontend/decoder.h>
#include <externals/vixl/aarch32/disasm-aarch32.h>

namespace Svm::Decoder {

    using namespace vixl::aarch32;

    class A32Decoder : public Decoder::BaseDecoder, vixl::aarch32::Disassembler {
    public:

        explicit A32Decoder(u32 code_addr, IR::Assembler *visitor, Memory::MemoryInterface *memory, bool thumb_mode);

        void Decode() override;

    private:

        enum ThumbType {
            Thumb16, Thumb32
        };

        void adc(Condition cond, EncodingSize size, Register rd, Register rn,
                 const Operand &operand) override;

        void adcs(Condition cond, EncodingSize size, Register rd, Register rn,
                  const Operand &operand) override;

        void add(Condition cond, EncodingSize size, Register rd, Register rn,
                 const Operand &operand) override;

        void add(Condition cond, Register rd, const Operand &operand) override;

        void adds(Condition cond, EncodingSize size, Register rd, Register rn,
                  const Operand &operand) override;

        void adds(Register rd, const Operand &operand) override;

        void addw(Condition cond, Register rd, Register rn, const Operand &operand) override;

        void adr(Condition cond, EncodingSize size, Register rd, Location *location) override;

        void UnallocatedT32(u32 instruction) override;

        void UnallocatedA32(u32 instruction) override;

        void UnimplementedT32_16(const char *name, u32 instruction) override;

        void UnimplementedT32_32(const char *name, u32 instruction) override;

        void UnimplementedA32(const char *name, u32 instruction) override;

        void Unpredictable() override;

        void UnpredictableT32(u32 uint32) override;

        void UnpredictableA32(u32 uint32) override;

        std::tuple<u32, ThumbType> ReadThumbInstr(u32 arm_pc);

        u32 code_start{};
        u32 code_address{};
        IR::Assembler *visitor{};
        bool thumb_mode{false};
        u32 pc{};
        Memory::MemoryInterface *memory{};
    };

}
