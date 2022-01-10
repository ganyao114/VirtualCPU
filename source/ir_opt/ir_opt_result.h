//
// Created by SwiftGan on 2021/1/4.
//

#pragma once

#include <base/marco.h>
#include <ir/operand.h>
#include <ir/instruction.h>

namespace Svm::IR {

    class OptHostReg : public BaseObject, CopyDisable {
    public:
        virtual bool CastHostReg(Reg &reg);

        virtual bool CastHostReg(VReg &reg);

        virtual bool DirectSetHostReg(Instruction *instr);

        virtual bool DirectGetHostReg(Instruction *instr, u8 op_index);

        virtual void MarkDirectSetHostReg(Instruction *instr, IR::Reg &reg);

        virtual void MarkDirectSetHostReg(Instruction *instr, IR::VReg &reg);

        virtual void MarkDirectGetHostReg(Instruction *instr, IR::Reg &reg);

        virtual void MarkDirectGetHostReg(Instruction *instr, IR::VReg &reg);
    };

    class OptValueFold : public BaseObject, CopyDisable {
    public:

        struct Op {
            enum Operate : u8 {
                Add,
                Sub,
                Mul,
                Div
            };
            Operate op;
            Value left{Null{}};
            std::variant<Imm, Value> right{Null{}};
        };
        
        virtual bool CouldFold(Instruction *dest, Instruction *src);
        
        virtual void MarkFold(Instruction *value_src, Set<Instruction *> &dest_instr_set);

        virtual Op *GetFoldOperand(Instruction *value_src);

        Map<u32, Op> folded_ops{};
    };

    class OptConstRead : public BaseObject, CopyDisable {
    public:

        virtual bool IsReadOnly(VAddr addr);

        virtual Vector<u8> ReadMemory(VAddr addr, size_t size);

        template <typename T>
        T Read(VAddr vaddr) {
            T t;
            auto buffer = ReadMemory(vaddr, sizeof(T));
            memcpy(&t, buffer.data(), sizeof(T));
            return std::move(t);
        }

    };

    class OptFlagsSync : public BaseObject, CopyDisable {
    public:

        virtual bool CanSyncFlagSet(IR::Flags &flag, Instruction *instr_flag_from);

        virtual bool CanSyncFlagGet(IR::Flags &flag, Instruction *instr_flag_consume);

        virtual void SyncFlagSet(Instruction *instr_flag_from, IR::Flags &flag);

        virtual void SyncFlagGet(Instruction *instr_flag_consume, IR::Flags &flag);

        virtual void FlagsCanNotSync(IR::Flags &flag);

    };

    class RegAllocPass : public BaseObject, CopyDisable {
    public:

        virtual void DefineValue(IR::Value &value) {};

        virtual void DefineFloatValue(IR::Value &value) {};

        virtual void UseValue(Instruction *instr, IR::Value &value) {};

        virtual void UseFloatValue(Instruction *instr, IR::Value &value) {};

        virtual void AllocateForBlock() {};

    };

    class OptResult : public BaseObject, CopyDisable {
    public:
        
        explicit OptResult() {}

        virtual OptHostReg *GetOptHostReg();

        virtual OptValueFold *GetOptValueFold();

        virtual OptConstRead *GetOptConstRead();

        virtual OptFlagsSync *GetOptFlagsGetSet();

        virtual RegAllocPass *GetRegAllocPass();
    };

}
