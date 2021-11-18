//
// Created by SwiftGan on 2021/1/2.
//

#pragma once

#include <base/marco.h>
#include <variant>

namespace Svm::IR {

    enum Size : u8 {
        VOID    = 0,
        U8      = 1 << 0,
        U16     = 1 << 1,
        U32     = 1 << 2,
        U64     = 1 << 3,
        U128    = 1 << 4,
        U256    = 1 << 5
    };

    struct Imm {

        constexpr Imm(u64 value, Size size) : data{value}, size{size} {}

        constexpr Imm(bool value) : data{value}, size{U8} {}

        constexpr Imm(u8 value) : data{value}, size{U8} {}

        constexpr Imm(u16 value) : data{value}, size{U16} {}

        constexpr Imm(u32 value) : data{value}, size{U32} {}

        constexpr Imm(u64 value) : data{value}, size{U64} {}

        constexpr Size GetSize() {
            return size;
        }

        template<typename T>
        constexpr T Value() {
            return static_cast<T>(data);
        }
        
        template<typename T>
        constexpr T Value() const {
            return static_cast<T>(data);
        }

        u64 data{};
        Size size{};
    };

    struct Value {

        explicit Value() = default;

        constexpr u32 Valid() {
            return id != 0;
        }

        constexpr u32 GetId() {
            return id;
        }

        constexpr Size GetSize() {
            return size;
        }

        constexpr Value &SetSize(Size size) {
            this->size = size;
            return *this;
        }

        constexpr u8 SizeByte() {
            return size;
        }

        constexpr bool IsFloat() {
            return is_float;
        }

        // value id
        u32 id{};
        Size size{};
        bool is_float{false};
    };

    struct Null : Value {
        explicit Null() {
            size = VOID;
        }
    };

    struct Void {
        explicit Void() {};
    };

    struct Reg {

        explicit Reg() {};
        
        explicit Reg(u8 code) : code{code} {}
        
        explicit Reg(u8 code, Size size) : code{code}, size{size} {}

        explicit Reg(u8 code, Size size, u8 type) : code{code}, size{size}, type{type} {}

        constexpr u8 GetCode() {
            return code;
        }

        constexpr u8 GetType() {
            return type;
        }

        constexpr Reg &SetSize(Size size) {
            this->size = size;
            return *this;
        }

        constexpr u8 SizeByte() {
            return size;
        }

        [[nodiscard]] constexpr u8 SizeByte() const {
            return size;
        }

        constexpr Size GetSize() {
            return size;
        }

        u8 code{};
        u8 type{};
        Size size{};
    };

    struct VReg {
        
        explicit VReg() {};
        
        explicit VReg(u8 code) : code{code} {}
        
        explicit VReg(u8 code, Size size) : code{code}, size{size} {}

        explicit VReg(u8 code, Size size, u8 type) : code{code}, size{size}, type{type} {}

        constexpr u8 GetCode() {
            return code;
        }

        constexpr u8 GetType() {
            return type;
        }

        constexpr VReg &SetSize(Size size) {
            this->size = size;
            return *this;
        }

        constexpr Size GetSize() {
            return size;
        }

        constexpr u8 SizeByte() {
            return size;
        }

        [[nodiscard]] constexpr u8 SizeByte() const {
            return size;
        }

        u8 code{};
        u8 type{};
        Size size{};
    };

    enum class Cond : u8 {
        EQ = 0, NE, CS, CC, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE, AL, NV,
        AT, AE, BT, BE, SN, NS, PA, NP, HS = CS, LO = CC, OF = VS, NO = VC
    };

    struct Flags {

        enum FlagValue : u16 {
            Carry       = 1 << 0,
            Overflow    = 1 << 1,
            Signed      = 1 << 2,
            Zero        = 1 << 3,
            FlagAll     = Carry | Overflow | Signed | Zero
        };

        explicit Flags() : flag() {}

        constexpr Flags(u32 value) : flag(value) {}

        u32 flag;
    };

    struct Address {
        explicit Address() = default;
        constexpr Address(const Imm &addr) : address{addr} {};
        constexpr Address(const Value &addr) : address{addr} {};

        constexpr bool IsConst() {
            return std::holds_alternative<Imm>(address);
        }

        constexpr Imm &ConstAddress() {
            return std::get<Imm>(address);
        }

        constexpr Value &ValueAddress() {
            return std::get<Value>(address);
        }

        std::variant<Imm, Value> address{Null{}};
    };

    struct Label {

        constexpr u32 GetId() {
            return id;
        }

        u32 id{};
    };

    struct Opr {

        constexpr Opr(const Value &value) : v{value} {}

        constexpr Value &GetValue() {
            return v;
        }

        Value v{};
    };

    struct CalAct {
        bool overflow{};
    };

    struct CalActRes {
        bool overflow{};
        bool carry{};
    };

    using OperandData = std::variant<Void, Imm, Value, Reg, VReg, Cond, Address, Label, Flags, Size, Opr, CalAct, CalActRes>;

    class Operand {
    public:

        explicit Operand() : data{Void{}} {}

        constexpr Operand(const Void &v) : data{v} {}

        constexpr Operand(const Size &size) : data{size} {}

        constexpr Operand(const Imm &imm) : data{imm} {}

        constexpr Operand(const Value &value) : data{value} {}

        constexpr Operand(const Reg &reg) : data{reg} {}

        constexpr Operand(const VReg &reg) : data{reg} {}

        constexpr Operand(const Cond &cond) : data{cond} {}

        constexpr Operand(const Address &address) : data{address} {}

        constexpr Operand(const Label &label) : data{label} {}

        constexpr Operand(const Flags &flags) : data{flags} {}

        constexpr Operand(const CalAct &act) : data{act} {}

        constexpr Operand(const CalActRes &act) : data{act} {}

        template<typename T>
        constexpr T &Get() {
            return std::get<T>(data);
        }

        constexpr bool IsVoid() {
            return std::holds_alternative<Void>(data);
        }

        constexpr bool IsImm() {
            return std::holds_alternative<Imm>(data);
        }

        constexpr bool IsValue() {
            return std::holds_alternative<Value>(data);
        }

        constexpr bool IsCond() {
            return std::holds_alternative<Cond>(data);
        }

        constexpr bool IsLabel() {
            return std::holds_alternative<Label>(data);
        }

        constexpr bool IsReg() {
            return std::holds_alternative<Reg>(data);
        }

        constexpr bool IsVReg() {
            return std::holds_alternative<VReg>(data);
        }

        constexpr bool IsAddress() {
            return std::holds_alternative<Address>(data);
        }

        constexpr bool IsFlags() {
            return std::holds_alternative<Flags>(data);
        }

        constexpr bool IsSize() {
            return std::holds_alternative<Size>(data);
        }

        constexpr bool IsCalAct() {
            return std::holds_alternative<CalAct>(data);
        }

        constexpr bool IsCalActRes() {
            return std::holds_alternative<CalActRes>(data);
        }

    private:
        OperandData data;
    };

    class GuestRegInterface {
    public:
        virtual u32 OffsetOf(const Reg &reg) = 0;
        virtual u32 OffsetOf(const VReg &reg) = 0;
        virtual u32 PCOffset() = 0;
    };

}
