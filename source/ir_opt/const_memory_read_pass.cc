//
// Created by swift on 1/8/21.
//

#include "const_memory_read_pass.h"

namespace Svm::IR {

    OptConstReadImpl::OptConstReadImpl(BasePageTable *memory) : memory(memory) {}

    bool OptConstReadImpl::IsReadOnly(VAddr addr) {
        auto &pte = memory->GetPTE(addr >> memory->page_bits);
        return pte.Readable() && !pte.Writeable();
    }

    Vector<u8> OptConstReadImpl::ReadMemory(VAddr addr, size_t size) {
        Vector<u8> buffer(size);
        memory->ReadMemory(addr, buffer.data(), size);
        return std::move(buffer);
    }

    void ConstMemoryReadOpt::Optimize(IRBlock *block, OptResult *result) {
        auto opt_const_read = result->GetOptConstRead();
        for (auto &instr : block->Sequence()) {
            if (!instr.Enabled()) {
                continue;
            }

            if (instr.GetOpCode() != OpCode::ReadMemory) {
                continue;
            }

            auto &address = instr.GetParam<Address>(0);
            auto size = instr.GetReturn().Get<Value>().GetSize();

            if (address.IsConst()) {
                auto vaddr = address.ConstAddress().Value<VAddr>();
                if (opt_const_read->IsReadOnly(vaddr)) {
                    TransferToRead(&instr, vaddr, opt_const_read, size);
                }
            } else {
                auto addr_from = address.ValueAddress().Def();
                if (addr_from->GetOpCode() == OpCode::LoadImm) {
                    auto vaddr = addr_from->GetParam<Imm>(0).Value<VAddr>();
                    if (opt_const_read->IsReadOnly(vaddr)) {
                        addr_from->UnUse(&instr);
                        TransferToRead(&instr, vaddr, opt_const_read, size);
                    }
                }
            }
        }
    }

    void ConstMemoryReadOpt::TransferToRead(Instruction *instr, VAddr vaddr, OptConstRead *opt, Size size) {
        instr->SetOpCode(OpCode::LoadImm);
        switch (size) {
            case U8:
                instr->SetParam(0, Imm(opt->Read<u8>(vaddr)));
                break;
            case U16:
                instr->SetParam(0, Imm(opt->Read<u16>(vaddr)));
                break;
            case U32:
                instr->SetParam(0, Imm(opt->Read<u32>(vaddr)));
                break;
            case U64:
                instr->SetParam(0, Imm(opt->Read<u64>(vaddr)));
                break;
            default:
                UNREACHABLE();
        }
        instr->SetParam(1, Void{});
    }
}