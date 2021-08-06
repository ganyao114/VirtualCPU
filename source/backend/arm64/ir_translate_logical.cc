//
// Created by swift on 2021/6/25.
//

#include "ir_translator.h"

#define __ context->Masm().

namespace Svm::A64 {

    void IRCommitA64::AddValue(IR::Value &left, IR::Value &right) {
        auto &left_reg = R(left);
        auto &right_reg = R(left);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ Add(return_reg, left_reg, right_reg);
    }

    void IRCommitA64::Lsl(IR::Value &value, IR::Imm &bits) {
        auto &value_reg = R(value);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ Lsl(return_reg, value_reg, bits.data);
    }

    void IRCommitA64::Lsr(IR::Value &value, IR::Imm &bits) {
        auto &value_reg = R(value);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ Lsr(return_reg, value_reg, bits.data);
    }

    void IRCommitA64::AddImm(IR::Value &left, IR::Imm &right) {
        auto &value_reg = R(left);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        if (IsUint<12>(right.data)) {
            __ Add(return_reg, value_reg, right.data);
        } else {
            auto &tmp = RTemp();
            __ Mov(tmp, right.data);
            __ Add(return_reg, value_reg, right.data);
            Release(tmp);
        }
    }

    void IRCommitA64::SubImm(IR::Value &left, IR::Imm &right) {
        auto &value_reg = R(left);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        if (IsUint<12>(right.data)) {
            __ Add(return_reg, value_reg, right.data);
        } else {
            auto &tmp = RTemp();
            __ Mov(tmp, right.data);
            __ Add(return_reg, value_reg, right.data);
            Release(tmp);
        }
    }


    void IRCommitA64::SubValue(IR::Value &left, IR::Value &right) {
        auto &left_reg = R(left);
        auto &right_reg = R(right);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ Sub(return_reg, left_reg, return_reg);
    }


    void IRCommitA64::AdcImm(IR::Value &left, IR::Imm &right) {
        auto &left_reg = R(left);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        if (IsUint<12>(right.data)) {
            __ Adc(return_reg, left_reg, right.data);
        } else {
            auto &tmp = RTemp();

            __ Mov(tmp, right.data);
            __ Adc(return_reg, left_reg, tmp);

            Release(tmp);
        }
    }

    void IRCommitA64::AdcValue(IR::Value &left, IR::Value &right) {
        auto &left_reg = R(left);
        auto &right_reg = R(right);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ Add(return_reg, left_reg, return_reg);
    }

    void IRCommitA64::SbbImm(IR::Value &left, IR::Imm &right) {

    }

    void IRCommitA64::SbbValue(IR::Value &left, IR::Value &right) {

    }

    void IRCommitA64::AndImm(IR::Value &left, IR::Imm &right) {
        auto &left_reg = R(left);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        if (IsUint<12>(right.data)) {
            __ And(return_reg, left_reg, right.data);
        } else {
            auto &tmp = RTemp();

            __ Mov(tmp, right.data);
            __ And(return_reg, left_reg, tmp);

            Release(tmp);
        }
    }

    void IRCommitA64::AndValue(IR::Value &left, IR::Value &right) {
        auto &left_reg = R(left);
        auto &right_reg = R(right);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ And(return_reg, left_reg, right_reg);
    }

    void IRCommitA64::OrImm(IR::Value &left, IR::Imm &right) {
        auto &left_reg = R(left);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);

        if (return_val.GetSize() != IR::U32 && return_val.GetSize() != IR::U64) {
            Fallback();
            return;
        }

        if (IsUint<12>(right.data)) {
            __ Orr(return_reg, left_reg, right.data);
        } else {
            auto &tmp = RTemp();

            __ Mov(tmp, right.data);
            __ Orr(return_reg, left_reg, tmp);

            Release(tmp);
        }
    }

    void IRCommitA64::OrValue(IR::Value &left, IR::Value &right) {
        auto &left_reg = R(left);
        auto &right_reg = R(right);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ Orr(return_reg, left_reg, right_reg);
    }

    void IRCommitA64::ZeroExtend(IR::Value &value, IR::Size &size) {
        auto &ret_val = current->GetReturn().Get<IR::Value>();
        auto &ret_reg = R(ret_val);
        auto &val_reg = R(value);

        switch (ret_val.GetSize()) {
            case IR::U16:
                __ Ubfx(ret_reg, val_reg, 0, 8);
                break;
            case IR::U32:
            case IR::U64:
                __ Mov(ret_reg, val_reg);
                break;
            default:
                Fallback();
                break;
        }
    }

    void IRCommitA64::Rr(IR::Value &left, IR::Imm &right) {
        auto &left_reg = R(left);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);

        if (return_val.GetSize() != IR::U32 && return_val.GetSize() != IR::U64) {
            Fallback();
            return;
        }

        if (IsUint<12>(right.data)) {
            __ Ror(return_reg, left_reg, right.data);
        } else {
            auto &tmp = RTemp();

            __ Mov(tmp, right.data);
            __ And(return_reg, left_reg, tmp);

            Release(tmp);
        }
    }

    void IRCommitA64::Asr(IR::Value &left, IR::Imm &right) {

    }

    void IRCommitA64::MulImm(IR::Value &left, IR::Imm &right) {

    }

    void IRCommitA64::MulValue(IR::Value &left, IR::Value &right) {
        auto &left_reg = R(left);
        auto &right_reg = R(right);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);

        __ Mul(return_reg, left_reg, right_reg);
    }

    void IRCommitA64::DivImm(IR::Value &left, IR::Imm &right) {
        auto &left_reg = R(left);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);

        if (return_val.GetSize() != IR::U32 && return_val.GetSize() != IR::U64) {
            Fallback();
            return;
        }

        auto &tmp = RTemp();

        __ Mov(tmp, right.data);
        __ And(return_reg, left_reg, tmp);

        Release(tmp);
    }

    void IRCommitA64::DivValue(IR::Value &left, IR::Value &right) {
        auto &left_reg = R(left);
        auto &right_reg = R(right);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);

        __ Udiv(return_reg, left_reg, right_reg);
    }

    void IRCommitA64::TestBit(IR::Value &value, IR::Imm &bit) {
        auto &val_reg = R(value);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        Label end_;
        __ Mov(return_reg, 0);
        __ Tbz(val_reg, bit.Value<u8>(), &end_);
        __ Mov(return_reg, 1);
        __ Bind(&end_);
    }

    void IRCommitA64::BitClear(IR::Value &value, IR::Imm &lsb, IR::Imm &len) {
        auto &val_reg = R(value);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ Bfc(val_reg, lsb.Value<u8>(), len.Value<u8>());
        __ Mov(return_reg, val_reg);
    }

    void IRCommitA64::BitExtract(IR::Value &value, IR::Imm &lsb, IR::Imm &len) {
        auto &val_reg = R(value);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ Ubfx(return_reg, val_reg, lsb.Value<u8>(), len.Value<u8>());
    }

    void IRCommitA64::BitInsert(IR::Value &value, IR::Value &a2, IR::Imm &lsb, IR::Imm &len) {
        auto &val_reg = R(value);
        auto &return_val = current->GetReturn().Get<IR::Value>();
        auto &return_reg = R(return_val);
        __ Bfi(return_reg, val_reg, lsb.Value<u8>(), len.Value<u8>());
    }

}