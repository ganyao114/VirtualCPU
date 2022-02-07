//
// Created by swift on 1/8/21.
//

#pragma once

#include <base/marco.h>
#include <ir/block.h>

namespace Svm {

    class Block : public BaseObject, CopyDisable {
    public:

        explicit Block(ObjectPool<IR::Instruction> *pool, VAddr pc);

        void AddPredecessor(Block* block);

        void AddSuccessor(Block* block);

        constexpr std::list<Block*> &Predecessors() {
            return predecessors;
        }

        constexpr std::list<Block*> &Successors() {
            return predecessors;
        }

        constexpr IR::IRBlock &GetIRBlock() {
            return ir_block;
        }
        
        constexpr VAddr StartPC() const {
            return ir_block.StartPC();
        }
        
        constexpr bool Overlap(VAddr pc) const {
            return ir_block.Overlap(pc);
        }

        constexpr void SetId(u32 id_) {
            this->id = id_;
        }

        [[nodiscard]] constexpr u32 GetId() const {
            return id;
        }

        constexpr bool operator==(const Block &rhs) const { return ir_block.StartPC() == rhs.ir_block.StartPC(); }

        constexpr bool operator!=(const Block &rhs) const { return ir_block.StartPC() != rhs.ir_block.StartPC(); }

        constexpr bool operator<(const Block &rhs) const { return ir_block.StartPC() < rhs.ir_block.StartPC(); }

        constexpr bool operator>(const Block &rhs) const { return ir_block.StartPC() > rhs.ir_block.StartPC(); }

        constexpr bool operator<=(const Block &rhs) const { return ir_block.StartPC() <= rhs.ir_block.StartPC(); }

        constexpr bool operator>=(const Block &rhs) const { return ir_block.StartPC() >= rhs.ir_block.StartPC(); }

    private:
        u32 id;
        IR::IRBlock ir_block;
        std::list<Block*> predecessors;
        std::list<Block*> successors;
        std::list<Block*> dominated_blocks;
    };

}
