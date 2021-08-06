//
// Created by swift on 2021/6/25.
//

#include "ir_translator.h"

#define __ context->Masm().

namespace Svm::A64 {

    void IRCommitA64::GetReg(IR::Reg &guest) {
        auto reg_mng = &context->RegMng();
        auto &return_value = current->GetReturn().Get<IR::Value>();
        auto &reg = reg_mng->GetValueRegister(return_value);
        if (reg_mng->CastedToHostReg(guest)) {
            context->Get(reg, reg_mng->ToHostRegister(guest), guest.SizeByte(), guest.type);
        } else {
            auto &ctx_ptr = reg_mng->Context();
            context->Load(reg, MemOperand(ctx_ptr, reg_mng->OffsetOf(guest)), guest.SizeByte());
        }
    }

    void IRCommitA64::GetVReg(IR::VReg &guest) {
        auto reg_mng = &context->RegMng();
        auto &return_value = current->GetReturn().Get<IR::Value>();
        auto &reg = reg_mng->GetValueVRegister(return_value);
        if (reg_mng->CastedToHostVReg(guest)) {
            __ Mov(reg, reg_mng->ToHostVRegister(guest));
        } else {
            auto &ctx_ptr = reg_mng->Context();
            context->Load(reg, MemOperand(ctx_ptr, reg_mng->OffsetOf(guest)), guest.SizeByte());
        }
    }

    void IRCommitA64::SetReg(IR::Reg &guest, IR::Value &value) {
        auto reg_mng = &context->RegMng();
        auto &reg = reg_mng->GetValueRegister(value);
        if (reg_mng->CastedToHostReg(guest)) {
            context->Set(reg_mng->ToHostRegister(guest), reg, guest.SizeByte(), guest.type);
        } else {
            auto &ctx_ptr = reg_mng->Context();
            context->Store(reg, MemOperand(ctx_ptr, reg_mng->OffsetOf(guest)), guest.SizeByte());
        }
    }

    void IRCommitA64::SetVReg(IR::VReg &rt, IR::Value &value) {
        auto reg_mng = &context->RegMng();
        auto &reg = reg_mng->GetValueVRegister(const_cast<IR::Value &>(value));
        if (reg_mng->CastedToHostVReg(rt)) {
            __ Mov(reg_mng->ToHostVRegister(rt), reg);
        } else {
            auto &ctx_ptr = reg_mng->Context();
            context->Store(reg, MemOperand(ctx_ptr, reg_mng->OffsetOf(rt)), rt.SizeByte());
        }
    }

}