//
// Created by swift on 1/8/21.
//

#include "const_memory_read_pass.h"

namespace Svm::IR {

    OptConstReadImpl::OptConstReadImpl(PageTable32 *memory32) : memory32(memory32) {}

    OptConstReadImpl::OptConstReadImpl(PageTable64 *memory64) : memory64(memory64) {}

    bool OptConstReadImpl::IsReadOnly(VAddr addr) {
        if (memory64) {
            auto &pte = memory64->GetPTE(addr >> memory64->page_bits);
            return pte.Readable() && !pte.Writeable();
        } else {
            auto &pte = memory32->GetPTE(addr >> memory64->page_bits);
            return pte.Readable() && !pte.Writeable();
        }
    }

    Vector<u8> OptConstReadImpl::ReadMemory(VAddr addr, size_t size) {
        Vector<u8> buffer(size);
        if (memory64) {
            memory64->ReadMemory(addr, buffer.data(), size);
        } else {
            memory32->ReadMemory(addr, buffer.data(), size);
        }
        return std::move(buffer);
    }

    void ConstMemoryReadOpt::Optimize(IRBlock *block, OptResult *result) {
        auto opt_const_read = result->GetOptConstRead();
        for (auto instr : block->Sequence()) {
            if (!result->IsEnable(instr->GetId())) {
                continue;
            }

            if (instr->GetOpCode() != OpCode::ReadMemory) {
                continue;
            }

            auto &address = instr->GetParam<Address>(0);
            auto size = instr->GetReturn().Get<Value>().GetSize();

            if (address.IsConst()) {
                VAddr vaddr = address.ConstAddress().Value<VAddr>();
                if (opt_const_read->IsReadOnly(vaddr)) {
                    TransferToRead(instr, vaddr, opt_const_read, size);
                }
            } else {
                auto &addr_from = block->Instr(address.ValueAddress().GetId());
                if (addr_from.GetOpCode() == OpCode::LoadImm) {
                    VAddr vaddr = addr_from.GetParam<Imm>(0).Value<VAddr>();
                    if (opt_const_read->IsReadOnly(vaddr)) {
                        addr_from.UnUse(instr->GetId());
                        TransferToRead(instr, vaddr, opt_const_read, size);
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