//
// Created by herlight on 2019/10/5.
//

#ifndef __MC_EYR_HH__
#define __MC_EYR_HH__

#include <vector>
#include <string>
#include <set>
#include <cassert>
#include <iostream>
#include <list>
#include "util.hh"

namespace mc {
namespace eyr {

struct Item;
struct Function;
struct BasicBlock;
struct Instruction;
struct Variable;
struct AssignInst;
struct BinaryInst;
struct UnaryInst;
struct MoveInst;
struct LoadInst;
struct StoreInst;
struct CallInst;
struct JumpInst;
struct BranchInst;
struct ReturnInst;

struct Module {
    std::vector<Item *> global_items;
    std::vector<Variable *> global_vars;
    std::vector<Function *> global_funcs;
    int T_id{0};
    int t_id{0};
    int l_id{0};

    void addFunction(Function *f);
    Variable *allocGlobalVar(int width = 0, bool addr = false);
    friend std::ostream &operator<<(std::ostream &os, const Module &mod);
};

struct Item {
    virtual ~Item() = default;
    virtual std::ostream &print(std::ostream &os) const = 0;
    friend std::ostream &operator<<(std::ostream &os, const Item &item) { return item.print(os); }
};

struct Function : public Item {
    Module *module;
    std::string name;
    std::vector<Variable *> params;
    std::vector<Variable *> local_vars;
    std::vector<BasicBlock *> blocks;
    BasicBlock *entry;

    Function(Module *m, std::string n, int argc);
    BasicBlock *allocBlock();

    Variable *
    allocLocalVar(bool temp = true, int width = 0, bool addr = false);

    void arrangeBlock();

    std::ostream &print(std::ostream &os) const override;
};

struct BasicBlock : public Item {
    Function *func;
    int label{0}; // global label
    int f_idx{0}; // in-function idx
    bool reachable{true};
    std::list<Instruction *> insts;

    BasicBlock *fall_out{nullptr};
    BasicBlock *fall_in{nullptr};
    BasicBlock *jump_out{nullptr};
    std::set<BasicBlock *> jump_in;

    explicit BasicBlock(Function *f) : func(f) {}
    inline void fall(BasicBlock *b) {
        fall_out = b, b->fall_in = this;
        // debug(this->label, " fall to ", b->label);
    }
    inline void jump(BasicBlock *b) {
        jump_out = b, b->jump_in.insert(this);
        // debug(this->label, " jump to ", b->label);
    }
    inline void unfall() {
        if (fall_out)
            fall_out->fall_in = nullptr, fall_out = nullptr;
    }
    inline void unjump() {
        if (jump_out)
            jump_out->jump_in.erase(this), jump_out = nullptr;
    }

    void safeRemove();
    std::vector<BasicBlock *> outBlocks() const;
    std::vector<BasicBlock *> inBlocks() const;

    Instruction *prevInstOf(Instruction *i);
    Instruction *nextInstOf(Instruction *i);
    void addInstAfter(Instruction *pos, Instruction *i);
    void addInstBefore(Instruction *pos, Instruction *i);
    void addInst(Instruction *i);
    void removeInst(Instruction *i);

    std::ostream &print(std::ostream &os) const override;
};

struct Variable : public Item {
    Function *func;
    std::string name;
    bool temp;
    int width;
    bool addr;

    Variable(Function *f, std::string n, bool tmp = true, int w = 0, bool c = false) :
            func(f), name(std::move(n)), temp(tmp), width(w), addr(c) {}
    std::ostream &print(std::ostream &os) const override;

    inline bool is_global() const { return func == nullptr; }
    inline bool is_param() const { return name[0] == 'p'; }
    inline bool is_local() const { return !is_global(); }
    inline bool is_addr() const { return addr; }
    inline bool is_temp() const { return temp; }
};

struct Operand {
    bool imm;
    int val;
    Variable *var;
    explicit Operand(int v = 0) : imm(true) { var = nullptr, val = v; }
    explicit Operand(Variable *v) : imm(false) { val = 0, var = v; }
    friend std::ostream &operator<<(std::ostream &os, const Operand &opr) {
        if (opr.imm)
            os << opr.val;
        else
            os << opr.var->name;
        return os;
    }
};

struct Instruction : public Item {
    BasicBlock *block{nullptr};
    std::list<Instruction *>::iterator link;

    virtual std::vector<Variable *> uses() const { return {}; }
    virtual std::vector<Variable *> defs() const { return {}; }

    inline Instruction *addAfter(Instruction *i) {
        return assert(block), block->addInstAfter(this, i), i;
    }
    inline Instruction *addBefore(Instruction *i) {
        return assert(block), block->addInstBefore(this, i), i;
    }
    inline Instruction *prev() { return assert(block), block->prevInstOf(this); }
    inline Instruction *next() { return assert(block), block->nextInstOf(this); }
    inline void remove() { assert(block), block->removeInst(this); }
};

struct AssignInst : public Instruction {
    Variable *dst;

    explicit AssignInst(Variable *d) : dst(d) {}
    std::vector<Variable *> defs() const override;
};

struct BinaryInst : public AssignInst {
    enum class BinOp {
        EQ = 2, NE, LT, GT, OR, AND, ADD, SUB, MUL, DIV, REM,
    };
    BinOp opt;
    Operand lhs, rhs;

    BinaryInst(Variable *d, BinOp op, Operand l, Operand r) :
            AssignInst(d), opt(op), lhs(l), rhs(r) {}

    std::vector<Variable *> uses() const override;
    std::ostream &print(std::ostream &os) const override;
};

struct UnaryInst : public AssignInst {
    enum class UnOp {
        NEG = 0, NOT,
    };
    UnOp opt;
    Operand opr;

    UnaryInst(Variable *d, UnOp op, Operand o) :
            AssignInst(d), opt(op), opr(o) {}
    std::vector<Variable *> uses() const override;
    std::ostream &print(std::ostream &os) const override;
};

struct CallInst : public AssignInst {
    std::string name;
    std::vector<Operand> args;

    CallInst(Variable *d, std::string n, std::vector<Operand> a) :
            AssignInst(d), name(std::move(n)), args(std::move(a)) {}
    std::vector<Variable *> uses() const override;
    std::ostream &print(std::ostream &os) const override;
};

struct MoveInst : public AssignInst {
    Operand src;

    MoveInst(Variable *d, Operand s) : AssignInst(d), src(s) {}
    std::vector<Variable *> uses() const override;
    std::ostream &print(std::ostream &os) const override;
};

struct StoreInst : public Instruction {
    Variable *base;
    Operand idx;
    Operand src;

    StoreInst(Variable *bs, Operand i, Operand s) :
            base(bs), idx(i), src(s) {}
    std::vector<Variable *> uses() const override;
    std::ostream &print(std::ostream &os) const override;
};

struct LoadInst : public AssignInst {
    Variable *src;
    Operand idx;

    LoadInst(Variable *d, Variable *s, Operand i) :
            AssignInst(d), src(s), idx(i) {}
    std::vector<Variable *> uses() const override;
    std::ostream &print(std::ostream &os) const override;
};

struct JumpInst : public Instruction {
    BasicBlock *dst;

    explicit JumpInst(BasicBlock *d) : dst(d) {}
    std::ostream &print(std::ostream &os) const override;
};

struct BranchInst : public JumpInst {
    enum class LgcOp {
        EQ = 2, NE, LT, GT, OR, AND,
    };
    LgcOp opt;
    Operand lhs, rhs;

    BranchInst(BasicBlock *d, LgcOp op, Operand l, Operand r) :
            JumpInst(d), opt(op), lhs(l), rhs(r) {}
    std::vector<Variable *> uses() const override;
    std::ostream &print(std::ostream &os) const override;
};

struct ReturnInst : public Instruction {
    Operand opr;

    explicit ReturnInst(Operand o) : opr(o) {}
    std::vector<Variable *> uses() const override;
    std::ostream &print(std::ostream &os) const override;
};

}
}

#endif
