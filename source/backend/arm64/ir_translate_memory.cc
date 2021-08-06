//
// Created by swift on 2021/6/25.
//

#include "ir_translator.h"

#define __ context->Masm().

namespace Svm::A64 {

    void IRCommitA64::ReadMemory(IR::Address &address) {
        auto &reg_mng = context->RegMng();
        auto &ret_val = current->GetReturn().Get<IR::Value>();
        auto &ret_reg = reg_mng.GetValueRegister(ret_val);
        if (address.IsConst()) {
            context->ReadMemory(ret_reg, {address.ConstAddress().data, ret_val.SizeByte()});
        } else {
            auto &va_reg = reg_mng.GetValueRegister(address.ValueAddress());
            context->ReadMemory(ret_reg, {va_reg, ret_val.SizeByte()});
        }
    }

    void IRCommitA64::WriteMemory(IR::Address &address, IR::Value &value) {
        auto &reg_mng = context->RegMng();
        auto &value_reg = reg_mng.GetValueRegister(value);
        if (address.IsConst()) {
            context->WriteMemory(value_reg, {address.ConstAddress().data, value.SizeByte()});
        } else {
            auto &va_reg = reg_mng.GetValueRegister(address.ValueAddress());
            context->WriteMemory(value_reg, {va_reg, value.SizeByte()});
        }
    }

    void IRCommitA64::CompareAndSwap(IR::Address &va, IR::Value &exp, IR::Value &update) {
        Register address{};
        if (va.IsConst()) {
            address = RTemp();
            __ Mov(address, va.ConstAddress().data);
        } else {
            address = R(va.ValueAddress());
        }
        auto &exp_reg = R(exp);
        auto &update_reg = R(update);
        auto &return_reg = R(current->GetReturn().Get<IR::Value>());
        address = R(va.ValueAddress());
        auto &pa = RTemp();
        context->VALookup(pa, address, Memory::PageEntry::Read | Memory::PageEntry::Write);
        context->CompareAndSwap(pa, exp_reg, update_reg, update.SizeByte());
        __ Mov(return_reg, exp_reg);
        Release(pa);
    }

}