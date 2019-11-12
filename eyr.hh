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
struct DeclInst;
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
    std::vector<DeclInst *> global_vars;
    std::vector<Function *> global_funcs;
    int T_id{0};
    int t_id{0};
    int l_id{0};

    void addFunction(Function *f);
    DeclInst *allocGlobalVar(int width = -1, bool constant = false);
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
    std::vector<DeclInst *> params;
    std::vector<DeclInst *> local_vars;
    std::vector<BasicBlock *> blocks;
    BasicBlock *fake;
    BasicBlock *entry;

    Function(Module *m, std::string n, int argc);
    BasicBlock *allocBlock();

    DeclInst *
    allocLocalVar(BasicBlock *block, bool temp = true, int width = -1, bool constant = false);

    void arrangeBlock();

    std::ostream &print(std::ostream &os) const override;
};

struct BasicBlock : public Item {
    Function *func;
    int label{0}; // global label
    int f_idx{0}; // in-function idx
    bool reachable{false};
    std::vector<Instruction *> insts;

    BasicBlock *fall_out{nullptr};
    BasicBlock *fall_in{nullptr};
    BasicBlock *jump_out{nullptr};
    std::set<BasicBlock *> jump_in;

    explicit BasicBlock(Function *f) : func(f) {}
    inline void fall(BasicBlock *b) {
        fall_out = b, b->fall_in = this;
    }
    inline void jump(BasicBlock *b) {
        jump_out = b, b->jump_in.insert(this);
    }

    inline void addInst(Instruction *i) { insts.push_back(i); }
    std::ostream &print(std::ostream &os) const override;
};

struct Instruction : public Item {
    BasicBlock *block;
    int b_idx{0}; // in-block idx
    int f_idx{0}; // in-function idx

    explicit Instruction(BasicBlock *b) : block(b) {}
};

struct DeclInst : public Instruction {
    std::string name;
    bool temp;
    int width;
    bool constant;

    DeclInst(BasicBlock *b, std::string n, bool tmp = true, int w = -1, bool c = false) :
            Instruction(b), name(std::move(n)), temp(tmp), width(w), constant(c) {}
    std::ostream &print(std::ostream &os) const override;
};

struct AssignInst : public Instruction {
    DeclInst *dst;

    AssignInst(BasicBlock *b, DeclInst *d) : Instruction(b), dst(d) {}
};

struct Operand {
    bool imm;
    int val;
    DeclInst *var;

    explicit Operand(int v = 0) : imm(true), val(v), var(nullptr) {}
    explicit Operand(DeclInst *v) : imm(false), val(0), var(v) {}
    friend std::ostream &operator<<(std::ostream &os, const Operand &opr) {
        if (opr.imm)
            os << opr.val;
        else
            os << opr.var->name;
        return os;
    }
};

struct BinaryInst : public AssignInst {
    enum class BinOp {
        EQ = 2, NE, LT, GT, OR, AND, ADD, SUB, MUL, DIV, REM,
    };
    BinOp opt;
    Operand lhs, rhs;

    BinaryInst(BasicBlock *b, DeclInst *d, BinOp op, Operand l, Operand r) :
            AssignInst(b, d), opt(op), lhs(l), rhs(r) {}
    std::ostream &print(std::ostream &os) const override;
};

struct UnaryInst : public AssignInst {
    enum class UnOp {
        NEG = 0, NOT,
    };
    UnOp opt;
    Operand opr;

    UnaryInst(BasicBlock *b, DeclInst *d, UnOp op, Operand o) :
            AssignInst(b, d), opt(op), opr(o) {}
    std::ostream &print(std::ostream &os) const override;
};

struct CallInst : public AssignInst {
    std::string name;
    std::vector<Operand> args;

    CallInst(BasicBlock *b, DeclInst *d, std::string n, std::vector<Operand> a) :
            AssignInst(b, d), name(std::move(n)), args(std::move(a)) {}
    std::ostream &print(std::ostream &os) const override;
};

struct MoveInst : public AssignInst {
    Operand src;

    MoveInst(BasicBlock *b, DeclInst *d, Operand s) : AssignInst(b, d), src(s) {}
    std::ostream &print(std::ostream &os) const override;
};

struct StoreInst : public AssignInst {
    Operand src;
    Operand idx;

    StoreInst(BasicBlock *b, DeclInst *d, Operand i, Operand s) :
            AssignInst(b, d), src(s), idx(i) {}
    std::ostream &print(std::ostream &os) const override;
};

struct LoadInst : public AssignInst {
    DeclInst *src;
    Operand idx;

    LoadInst(BasicBlock *b, DeclInst *d, DeclInst *s, Operand i) :
            AssignInst(b, d), src(s), idx(i) {}
    std::ostream &print(std::ostream &os) const override;
};

struct JumpInst : public Instruction {
    BasicBlock *dst;

    JumpInst(BasicBlock *b, BasicBlock *d) : Instruction(b), dst(d) {}
    std::ostream &print(std::ostream &os) const override;
};

struct BranchInst : public JumpInst {
    enum class LgcOp {
        EQ = 2, NE, LT, GT, OR, AND,
    };
    LgcOp opt;
    Operand lhs, rhs;

    BranchInst(BasicBlock *b, BasicBlock *d, LgcOp op, Operand l, Operand r) :
            JumpInst(b, d), opt(op), lhs(l), rhs(r) {}
    std::ostream &print(std::ostream &os) const override;
};

struct ReturnInst : public Instruction {
    Operand opr;

    ReturnInst(BasicBlock *b, Operand o) : Instruction(b), opr(o) {}
    std::ostream &print(std::ostream &os) const override;
};

};
};

#endif
