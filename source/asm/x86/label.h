#pragma once

#include <base/marco.h>

namespace Svm::X86 {

    class Assembler;

    class AssemblerBuffer;

    class AssemblerFixup;

    class X86Assembler;
    class X86_64Assembler;

    namespace x32 {
        class NearLabel;
    }

    namespace x64 {
        class NearLabel;
    }

    class ExternalLabel {
    public:
        ExternalLabel(const char *name_in, uintptr_t address_in)
                : name_(name_in), address_(address_in) {
        }

        const char *name() const { return name_; }

        uintptr_t address() const {
            return address_;
        }

    private:
        const char *name_;
        const uintptr_t address_;
    };

    class Label {
    public:
        Label() : position_(0) {}

        Label(Label &&src)
                : position_(src.position_) {
            // We must unlink/unbind the src label when moving; if not, calling the destructor on
            // the src label would fail.
            src.position_ = 0;
        }

        ~Label() {
            // Assert if label is being destroyed with unresolved branches pending.
            CHECK(!IsLinked());
        }

        // Returns the position for bound and linked labels. Cannot be used
        // for unused labels.
        int Position() const {
            CHECK(!IsUnused());
            return IsBound() ? -position_ - sizeof(void *) : position_ - sizeof(void *);
        }

        int LinkPosition() const {
            CHECK(IsLinked());
            return position_ - sizeof(void *);
        }

        bool IsBound() const { return position_ < 0; }

        bool IsUnused() const { return position_ == 0; }

        bool IsLinked() const { return position_ > 0; }

        void BindTo(int position) {
            position_ = -position - sizeof(void *);
        }

        void LinkTo(int position) {
            position_ = position + sizeof(void *);
        }

    private:

        int position_;

        void Reinitialize() {
            position_ = 0;
        }

        friend class X86Assembler;

        friend class x32::NearLabel;

        friend class X86_64Assembler;

        friend class x64::NearLabel;
    };


}
