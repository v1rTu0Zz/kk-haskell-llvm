#include "ast.hpp"

void yyerror(string s);

LLVMContext TheContext;
IRBuilder<> Builder(TheContext);
Module* TheModule;
map<string, AllocaInst*> NamedValues;
legacy::FunctionPassManager* TheFPM;

Value* NumberExprAST::codegen() const {
  return ConstantFP::get(TheContext, APFloat(Val));
}

Value* VariableExprAST::codegen() const {
  AllocaInst* tmp = NamedValues[Name];
  if (tmp == nullptr) {
	//TODO: try calling a function with this name
	auto call = new CallExprAST(Name, vector<ExprAST*>());
	Value* retVal = call->codegen();
  	if(retVal == nullptr)
	    yyerror("Promenljiva " + Name + " ne postoji");
	
	return retVal;
  }
  return Builder.CreateLoad(tmp, Name);
}

InnerExprAST::~InnerExprAST() {
  for (auto i : Vec)
    delete i;
}

InnerExprAST::InnerExprAST(ExprAST *e1) {
  Vec.push_back(e1);
}

InnerExprAST::InnerExprAST(ExprAST *e1, ExprAST *e2) {
  Vec.push_back(e1);
  Vec.push_back(e2);
}

InnerExprAST::InnerExprAST(ExprAST *e1, ExprAST *e2, ExprAST *e3) {
  Vec.push_back(e1);
  Vec.push_back(e2);
  Vec.push_back(e3);
}

InnerExprAST::InnerExprAST(ExprAST *e1, ExprAST *e2, ExprAST *e3, ExprAST *e4) {
  Vec.push_back(e1);
  Vec.push_back(e2);
  Vec.push_back(e3);
  Vec.push_back(e4);
}

Value* AddExprAST::codegen() const {
  Value *l = Vec[0]->codegen();
  Value *r = Vec[1]->codegen();
  if (!l || !r)
    return nullptr;
  return Builder.CreateFAdd(l, r, "addtmp");
}

Value* SubExprAST::codegen() const {
  Value *l = Vec[0]->codegen();
  Value *r = Vec[1]->codegen();
  if (!l || !r)
    return nullptr;
  return Builder.CreateFSub(l, r, "subtmp");
}

Value* MulExprAST::codegen() const {
  Value *l = Vec[0]->codegen();
  Value *r = Vec[1]->codegen();
  if (!l || !r)
    return nullptr;
  return Builder.CreateFMul(l, r, "multmp");
}

Value* DivExprAST::codegen() const {
  Value *l = Vec[0]->codegen();
  Value *r = Vec[1]->codegen();
  if (!l || !r)
    return nullptr;
  return Builder.CreateFDiv(l, r, "divtmp");
}

Value* LtExprAST::codegen() const {
  Value *l = Vec[0]->codegen();
  Value *r = Vec[1]->codegen();
  if (!l || !r)
    return nullptr;
  Value* tmp = Builder.CreateFCmpULT(l, r, "lttmp");
  return Builder.CreateUIToFP(tmp, Type::getDoubleTy(TheContext), "booltmp");
}

Value* GtExprAST::codegen() const {
  Value *l = Vec[0]->codegen();
  Value *r = Vec[1]->codegen();
  if (!l || !r)
    return nullptr;
  Value* tmp = Builder.CreateFCmpUGT(l, r, "gttmp");
  return Builder.CreateUIToFP(tmp, Type::getDoubleTy(TheContext), "booltmp");
}

Value* EqExprAST::codegen() const {
  Value *l = Vec[0]->codegen();
  Value *r = Vec[1]->codegen();
  if (!l || !r)
    return nullptr;
  Value* tmp = Builder.CreateFCmpUEQ(l, r, "eqtmp");
  return Builder.CreateUIToFP(tmp, Type::getDoubleTy(TheContext), "booltmp");
}

Value* CallExprAST::codegen() const {
  Function* CalleeF = TheModule->getFunction(Callee);
  if (CalleeF == nullptr)
    yyerror("Funkcija " + Callee + " ne postoji");

  unsigned arg_size = CalleeF->arg_size();
  if (arg_size != Vec.size())
    yyerror("Funkcija " + Callee + " treba da bude pozvana sa " + 
			to_string(arg_size) + " argumenata");

  vector<Value*> args;
  for (unsigned i = 0; i < arg_size; i++) {
    Value *tmp = Vec[i]->codegen();
    if (tmp == nullptr)
      return nullptr;
    args.push_back(tmp);
  }

  //CallingConv::ID conv = CallingConv::GHC;

  return Builder.CreateCall(CalleeF, args, "calltmp");
}

Value* IfExprAST::codegen() const {
  Value* CondV = Vec[0]->codegen();
  if (CondV == nullptr)
    return nullptr;
  
  Value *IfCondV = Builder.CreateFCmpONE(CondV, ConstantFP::get(TheContext, APFloat(0.0)), "ifcond");

  Function* TheFunction = Builder.GetInsertBlock()->getParent();
  BasicBlock* ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
  BasicBlock* ElseBB = BasicBlock::Create(TheContext, "else");
  BasicBlock* MergeBB = BasicBlock::Create(TheContext, "ifcont");
  
  Builder.CreateCondBr(IfCondV, ThenBB, ElseBB);

  Builder.SetInsertPoint(ThenBB);
  Value* ThenV = Vec[1]->codegen();
  if (ThenV == nullptr)
    return nullptr;
  Builder.CreateBr(MergeBB);
  ThenBB = Builder.GetInsertBlock();

  TheFunction->getBasicBlockList().push_back(ElseBB);
  Builder.SetInsertPoint(ElseBB);
  Value* ElseV = Vec[2]->codegen();
  if (ElseV == nullptr)
    return nullptr;
  Builder.CreateBr(MergeBB);
  ElseBB = Builder.GetInsertBlock();

  TheFunction->getBasicBlockList().push_back(MergeBB);
  Builder.SetInsertPoint(MergeBB);
  PHINode *PHI = Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, "iftmp");
  PHI->addIncoming(ThenV, ThenBB);
  PHI->addIncoming(ElseV, ElseBB);
  
  return PHI;
}

Value* ForExprAST::codegen() const {
  BasicBlock* LoopBB = BasicBlock::Create(TheContext, "loop");
  BasicBlock* AfterLoopBB = BasicBlock::Create(TheContext, "afterloop");
  Function* TheFunction = Builder.GetInsertBlock()->getParent();

  AllocaInst *I = CreateEntryBlockAlloca(TheFunction, VarName);
  
  Value* StartVal = Vec[0]->codegen();
  if (StartVal == nullptr)
    return nullptr;
  Builder.CreateStore(StartVal, I);
  Builder.CreateBr(LoopBB);

  TheFunction->getBasicBlockList().push_back(LoopBB);
  Builder.SetInsertPoint(LoopBB);
  
  AllocaInst* OldVal = NamedValues[VarName];
  NamedValues[VarName] = I;

  Value* BodyVal = Vec[3]->codegen();
  if (BodyVal == nullptr)
    return nullptr;

  Value* IncVal = Vec[2]->codegen();
  if (IncVal == nullptr)
    return nullptr;

  Value* tmp = Builder.CreateLoad(I);
  Value* NextVar = Builder.CreateFAdd(tmp, IncVal, "nextvar");
  Builder.CreateStore(NextVar, I);
  Value* BoolTmp = Vec[1]->codegen();
  if (BoolTmp == nullptr)
    return nullptr;

  Value *LoopCondV = Builder.CreateFCmpONE(BoolTmp, 
		  ConstantFP::get(TheContext, APFloat(0.0)), "loopcond");

  Builder.CreateCondBr(LoopCondV, LoopBB, AfterLoopBB);

  LoopBB = Builder.GetInsertBlock();

  TheFunction->getBasicBlockList().push_back(AfterLoopBB);
  Builder.SetInsertPoint(AfterLoopBB);

  if (OldVal != nullptr)
    NamedValues[VarName] = OldVal;
  else
    NamedValues.erase(VarName);
  
  return ConstantFP::get(TheContext, APFloat(0.0));
}

Value* CExprAST::codegen() const {
  Value *l = Vec[0]->codegen();
  Value *r = Vec[1]->codegen();
  if (!l || !r)
    return nullptr;
  return r;  
}

Function* PrototypeAST::codegen() const {
  vector<Type*> doubles(Args.size(), Type::getDoubleTy(TheContext));
  FunctionType* FT = FunctionType::get(Type::getDoubleTy(TheContext), doubles, false);

  Function *f = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);

  unsigned i = 0;
  for (auto &arg : f->args())
    arg.setName(Args[i++]);

  return f;
}

FunctionAST::~FunctionAST() {
  delete Proto;
  delete Body;
}

Function* FunctionAST::codegen() const {
  Function* TheFunction = TheModule->getFunction(Proto->getName());
  if (TheFunction == nullptr)
    TheFunction = Proto->codegen();

  //CallingConv::ID conv = CallingConv::GHC;

  if (TheFunction == nullptr)
    return nullptr;

  if (!TheFunction->empty())
    yyerror("Nedozvoljeno je redefinisati fju " + Proto->getName());

  BasicBlock* BB = BasicBlock::Create(TheContext, "entry", TheFunction);
  Builder.SetInsertPoint(BB);

  NamedValues.clear();
  for (auto &arg : TheFunction->args()) {
    AllocaInst* tmp = CreateEntryBlockAlloca(TheFunction, arg.getName());
    NamedValues[arg.getName()] = tmp;
    Builder.CreateStore(&arg, tmp);
  }

  if (Value *V = Body->codegen()) {
    Builder.CreateRet(V);

    verifyFunction(*TheFunction);

    //TheFPM->run(*TheFunction);

    return TheFunction;
  }

  TheFunction->eraseFromParent();
  return nullptr;
}

AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
				   const string &VarName) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Type::getDoubleTy(TheContext), 0,
                           VarName);
}
