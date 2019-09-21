#ifndef __AST_HPP__
#define __AST_HPP__ 1

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/GlobalVariable.h"

using namespace llvm;
using namespace llvm::legacy;

#include <string>
#include <vector>
using namespace std;

class ExprAST {
public:
  virtual Value* codegen() const = 0;
  virtual ~ExprAST() {

  }
};

class NumberExprAST : public ExprAST {
public:
  NumberExprAST(double v)
    :Val(v)
  {}
  Value* codegen() const;
private:
  double Val;
};

class VariableExprAST : public ExprAST {
public:
  VariableExprAST(const string &n)
    :Name(n)
  {}
  Value* codegen() const;
private:
  string Name;
};

class InnerExprAST : public ExprAST {
public:
  InnerExprAST(const vector<ExprAST*> &v)
    :Vec(v)
  {}
  InnerExprAST(ExprAST* e1);
  InnerExprAST(ExprAST* e1, ExprAST* e2);
  InnerExprAST(ExprAST* e1, ExprAST* e2, ExprAST* e3);
  InnerExprAST(ExprAST* e1, ExprAST* e2, ExprAST* e3, ExprAST* e4);
  ~InnerExprAST();
private:
  InnerExprAST(const InnerExprAST&);
  InnerExprAST& operator=(const InnerExprAST&);
protected:
  vector<ExprAST*> Vec;
};

class AddExprAST : public InnerExprAST {
public:
  AddExprAST(ExprAST* l, ExprAST *r)
    :InnerExprAST(l, r)
  {}
  Value* codegen() const;
};

class SubExprAST : public InnerExprAST {
public:
  SubExprAST(ExprAST* l, ExprAST *r)
    :InnerExprAST(l, r)
  {}
  Value* codegen() const;
};

class MulExprAST : public InnerExprAST {
public:
  MulExprAST(ExprAST* l, ExprAST *r)
    :InnerExprAST(l, r)
  {}
  Value* codegen() const;
};

class DivExprAST : public InnerExprAST {
public:
  DivExprAST(ExprAST* l, ExprAST *r)
    :InnerExprAST(l, r)
  {}
  Value* codegen() const;
};

class LtExprAST : public InnerExprAST {
public:
  LtExprAST(ExprAST* l, ExprAST *r)
    :InnerExprAST(l, r)
  {}
  Value* codegen() const;
};

class GtExprAST : public InnerExprAST {
public:
  GtExprAST(ExprAST* l, ExprAST *r)
    :InnerExprAST(l, r)
  {}
  Value* codegen() const;
};

class EqExprAST : public InnerExprAST {
public:
  EqExprAST(ExprAST* l, ExprAST *r)
    :InnerExprAST(l, r)
  {}
  Value* codegen() const;
};

class CallExprAST : public InnerExprAST {
public:
  CallExprAST(string c, const vector<ExprAST*> &v)
    :InnerExprAST(v), Callee(c)
  { }
  Value* codegen() const;
private:
  string Callee;
};

class IfExprAST : public InnerExprAST {
public:
  IfExprAST(ExprAST* cond, ExprAST *e1, ExprAST *e2)
    :InnerExprAST(cond, e1, e2)
  {}
  Value* codegen() const;
};

class ForExprAST : public InnerExprAST {
public:
  ForExprAST(string s, ExprAST *e1, ExprAST *e2, ExprAST *e3, ExprAST *e4)
    :InnerExprAST(e1, e2, e3, e4), VarName(s)
  {}
  Value* codegen() const;
private:
  string VarName;
};

class CExprAST : public InnerExprAST {
public:
  CExprAST(ExprAST* e1, ExprAST* e2)
    :InnerExprAST(e1, e2)
  {}
  Value* codegen() const;
};

class PrototypeAST {
public:
  PrototypeAST(const string &n, const vector<string> &v)
    :Name(n), Args(v)
  {}
  Function* codegen() const;
  string getName() const {
    return Name;
  }
private:
  string Name;
  vector<string> Args;
};

class FunctionAST {
public:
  FunctionAST(PrototypeAST *p, ExprAST *b)
    :Proto(p), Body(b)
  {}
  Function* codegen() const;
  ~FunctionAST();
private:
  FunctionAST(const FunctionAST&);
  FunctionAST& operator=(const FunctionAST&);
  PrototypeAST *Proto;
  ExprAST *Body;
};

AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
				   const string &VarName);

#endif

