//
// Created by swift on 1/9/21.
//

#include "function_disam.h"
#include <ir/assembler.h>
#include <frontend/x64/decoder.h>

namespace Svm {

    class DummyMemory : public MemoryInterface<VAddr> {
    public:

        void ReadMemory(const VAddr src_addr, void *dest_buffer, std::size_t size) override {
            UNREACHABLE();
        }

        void WriteMemory(const VAddr dest_addr, const void *src_buffer, std::size_t size) override {
            UNREACHABLE();
        }

        std::optional<void *> GetPointer(VAddr vaddr) override {
            return std::optional<void *>(reinterpret_cast<void*>(vaddr));
        }

    };

    SharedPtr<Ast::Graph> BuildFunction(VAddr pc, MemoryInterface<VAddr> *memory) {

        struct NearFunctionCall {
            Ast::BasicBlock *block;
            VAddr target;
        };

        auto func_graph = MakeShared<Ast::Graph>(pc);

        DummyMemory dummy{};

        if (!memory) {
            memory = &dummy;
        }

        Map<VAddr, Ast::BasicBlock*> blocks;
        List<NearFunctionCall> near_func_calls;
        List<VAddr> basic_block_start;
        
        auto add_block = [&] (SharedPtr<Ast::BasicBlock> b) -> void {
            func_graph->AddBlock(b);
            blocks[b->StartPC()] = b.get();
        };
        
        auto find_block = [&] (VAddr vaddr) -> Ast::BasicBlock* {
            auto find = blocks.find(vaddr);
            if (find != blocks.end()) {
                return find->second;
            }
            for (auto itr : blocks) {
                if (itr.second->InBlock(vaddr)) {
                    return itr.second;
                }
            }
            return {};
        };
        
        basic_block_start.push_back(pc);
        bool first_block{true};
        while (!basic_block_start.empty()) {
            auto block_start = basic_block_start.begin();
            basic_block_start.erase(block_start);
            if (auto found = find_block(*block_start)) {
                if (found->StartPC() == *block_start) {
                    continue;
                } else {
                    auto split_offset = *block_start - found->StartPC();
                    auto new_ir_block = MakeShared<IR::IRBlock>();
                    found->GetIRBlock()->Split(new_ir_block, (u32)split_offset);
                    found->SetSize(found->GetIRBlock()->BlockCodeSize());
                    auto new_block = MakeShared<Ast::BasicBlock>(func_graph.get(), new_ir_block->StartPC(), new_ir_block->BlockCodeSize());
                    new_block->SetIRBlock(new_ir_block);
                    add_block(new_block);
                    continue;
                }
            }
            auto block = DecodeBasicBlock(func_graph.get(), *block_start, memory);
            add_block(block);
            if (first_block) {
                first_block = false;
                func_graph->SetEntry(block.get());
            }
            auto ir_block = block->GetIRBlock();
            if (ir_block->GetTermReason() == IR::IRBlock::RET) {
                func_graph->SetExit(block.get());
                continue;
            }
            switch (ir_block->GetTermType()) {
                case IR::IRBlock::DIRECT: {
                    auto &address = ir_block->TermDirect().next_block;
                    if (address.IsConst()) {
                        if (ir_block->GetTermReason() == IR::IRBlock::FUNC_CALL) {
                            near_func_calls.push_back({block.get(), address.ConstAddress().Value<VAddr>()});
                        } else {
                            basic_block_start.push_back(address.ConstAddress().Value<VAddr>());
                        }
                    }
                    basic_block_start.push_back(ir_block->StartPC() + ir_block->BlockCodeSize());
                    break;
                }
                case IR::IRBlock::CHECK_COND: {
                    auto &address_then = ir_block->TermCheckCond().then_;
                    auto &address_else = ir_block->TermCheckCond().else_;
                    if (address_then.IsConst()) {
                        basic_block_start.push_back(address_then.ConstAddress().Value<VAddr>());
                    }
                    if (address_else.IsConst()) {
                        basic_block_start.push_back(address_else.ConstAddress().Value<VAddr>());
                    }
                    break;
                }
                case IR::IRBlock::CHECK_BOOL: {
                    auto &address_then = ir_block->TermCheckBool().then_;
                    auto &address_else = ir_block->TermCheckBool().else_;
                    if (address_then.IsConst()) {
                        basic_block_start.push_back(address_then.ConstAddress().Value<VAddr>());
                    }
                    if (address_else.IsConst()) {
                        basic_block_start.push_back(address_else.ConstAddress().Value<VAddr>());
                    }
                    break;
                }
            }
        }
        
        // Link Block
        
        auto link_block = [&] (Ast::BasicBlock *b, IR::Address &address) -> void {
            auto vaddr = address.ConstAddress().Value<VAddr>();
            auto target = find_block(address.ConstAddress().Value<VAddr>());
            if (target) {
                b->AddSuccessor(target);
            }
        };
        
        for (auto &block : func_graph->Blocks()) {
            auto ir_block = block->GetIRBlock();
            switch (ir_block->GetTermType()) {
                case IR::IRBlock::DIRECT: {
                    auto &address = ir_block->TermDirect().next_block;
                    if (address.IsConst()) {
                        if (ir_block->GetTermReason() != IR::IRBlock::FUNC_CALL) {
                            link_block(block.get(), address);
                        }
                    }
                    break;
                }
                case IR::IRBlock::CHECK_COND: {
                    auto &address_then = ir_block->TermCheckCond().then_;
                    auto &address_else = ir_block->TermCheckCond().else_;
                    if (address_then.IsConst()) {
                        link_block(block.get(), address_then);
                    }
                    if (address_else.IsConst()) {
                        link_block(block.get(), address_else);
                    }
                    break;
                }
                case IR::IRBlock::CHECK_BOOL: {
                    auto &address_then = ir_block->TermCheckBool().then_;
                    auto &address_else = ir_block->TermCheckBool().else_;
                    if (address_then.IsConst()) {
                        link_block(block.get(), address_then);
                    }
                    if (address_else.IsConst()) {
                        link_block(block.get(), address_else);
                    }
                    break;
                }
                case IR::IRBlock::DEAD_END: {
                    // End
                    break;
                }
            }
        }
        
        return std::move(func_graph);
    }

    SharedPtr<Ast::BasicBlock> DecodeBasicBlock(Ast::Graph *graph, VAddr pc, MemoryInterface<VAddr> *memory) {
        auto ir_block = MakeShared<IR::IRBlock>(pc);
        IR::Assembler ir_asm{*ir_block};
        Decoder::X64Decoder decoder{pc, &ir_asm, memory};
        decoder.Decode();
        auto basic_block = MakeShared<Ast::BasicBlock>(graph, pc, ir_block->BlockCodeSize());
        basic_block->SetIRBlock(ir_block);
        return std::move(basic_block);
    }

}
