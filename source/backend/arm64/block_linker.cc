//
// Created by 甘尧 on 2022/1/27.
//

#include "constants.h"
#include "platform/memory.h"
#include "block_linker.h"

namespace Svm {

    bool LinkBlock(PAddr source, PAddr target, u8 *source_rw, bool pic) {
        constexpr auto _4K = 1 << 12;
        constexpr auto _128MB = 1ULL << 27;
        constexpr auto _4G = 1ULL << 32;
        const auto ip = REG_IP;
        s64 offset = target - source;
        MacroAssembler masm{};
        if (std::abs(offset) >= _4G) {
            if (pic) {
                return false;
            }
            masm.Mov(ip, target);
            masm.Br(ip);
        } else if (std::abs(offset) >= _128MB) {
            auto page_offset = target % _4K;
            Label label{};
            masm.Adrp(ip, &label);
            masm.Add(ip, ip, page_offset);
            masm.Br(ip);
            masm.BindToOffset(&label, offset);
        } else {
            Label label{};
            masm.B(&label);
            masm.BindToOffset(&label, offset);
        }
        masm.FinalizeCode();
        memcpy(source_rw, masm.GetBuffer()->GetStartAddress<void*>(), masm.GetBuffer()->GetSizeInBytes());
        Platform::ClearDCache(reinterpret_cast<VAddr>(source_rw), 4 * 5);
        Platform::ClearDCache(target, 4 * 5);
        Platform::ClearICache(target, 4 * 5);
        return true;
    }

}
