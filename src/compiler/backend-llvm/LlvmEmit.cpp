// llvm_emit.cpp - 修复版本（解决ICmp类型不匹配问题）

#include "../include/llvm_emit.h"
#include "../include/ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Verifier.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <stdio.h>
#include <map>
#include <string>
#include <vector>
#include <stack>
#include <iostream>
#include <cstdint>

using namespace llvm;

// ==================== TYPES ====================
enum class ValueType {
    VOID,
    INT32,
    INT64,
    INT8,
    FLOAT32,
    FLOAT64,
    BOOL,
    POINTER,
    STRING,
    ARRAY
};

// ==================== INIT!!! ====================
void initTarget() {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();
}

// ==================== ScopeManager ====================
class ScopeManager {
private:
    std::vector<std::map<std::string, AllocaInst*>> scopes;
    Function* currentFunction;
    
public:
    ScopeManager() : currentFunction(nullptr) { enterScope(); }
    
    void enterScope() { scopes.push_back({}); }
    
    void exitScope() {
        if (scopes.size() > 1) scopes.pop_back();
    }
    
    void setCurrentFunction(Function* func) { currentFunction = func; }
    
    Function* getCurrentFunction() { return currentFunction; }
    
    void defineVariable(const std::string& name, AllocaInst* alloc) {
        if (!scopes.empty()) scopes.back()[name] = alloc;
    }
    
    AllocaInst* findVariable(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second;
        }
        return nullptr;
    }
    
    void clear() { scopes.clear(); enterScope(); }
};

// ==================== Type Helper ====================
class TypeHelper {
private:
    LLVMContext& context;
    std::map<std::string, StructType*> structTypes;
    std::map<std::string, std::vector<std::pair<std::string, Type*>>> structFields;
    std::map<std::string, std::pair<Type*, int>> arrayTypes;
    std::map<std::string, int> variableArraySizes;
    std::map<std::string, bool> stringVariables;//标记字符串变量
    
public:
    TypeHelper(LLVMContext& ctx) : context(ctx) {}
    void registerStringVariable(const std::string& name) {
        stringVariables[name] = true;
    }
    bool isStringVariable(const std::string& name) {
        return stringVariables.find(name) != stringVariables.end();
    }
    
    void registerArrayType(const std::string& name, Type* elementType, int elementCount) {
        arrayTypes[name] = {elementType, elementCount};
        if (elementType->isIntegerTy(8)) {
            stringVariables[name] = true;
        }
    }
    
    std::pair<Type*, int>* getArrayTypeInfo(const std::string& name) {
        auto it = arrayTypes.find(name);
        return it != arrayTypes.end() ? &it->second : nullptr;
    }
    
    void registerVariableArraySize(const std::string& varName, int size) {
        variableArraySizes[varName] = size;
    }
    
    int getVariableArraySize(const std::string& varName) {
        auto it = variableArraySizes.find(varName);
        return it != variableArraySizes.end() ? it->second : -1;
    }
    
    Type* createArrayType(Type* elementType, int elementCount) {
        return ArrayType::get(elementType, elementCount);
    }
    
    Type* getArrayElementTypeFromNode(ASTNode* node) {
        if (!node) return Type::getInt32Ty(context);
        
        if (node->type == AST_TYPE_LIST) {
            if (node->data.list_type.element_type) {
                return getTypeFromTypeNode(node->data.list_type.element_type);
            }
        }
        
        return Type::getInt32Ty(context);
    }
    
    int getArrayElementCountFromNode(ASTNode* node) {
        if (!node) return 0;
        
        if (node->type == AST_TYPE_FIXED_SIZE_LIST) {
            return (int)node->data.fixed_size_list_type.size;
        }
        
        return 0;
    }
    
    void registerStructType(const std::string& name, StructType* type, 
                           std::vector<std::pair<std::string, Type*>> fields) {
        structTypes[name] = type;
        structFields[name] = fields;
    }
    
    StructType* getStructType(const std::string& name) {
        auto it = structTypes.find(name);
        return it != structTypes.end() ? it->second : nullptr;
    }
    
    std::vector<std::pair<std::string, Type*>>* getStructFields(const std::string& name) {
        auto it = structFields.find(name);
        return it != structFields.end() ? &it->second : nullptr;
    }
    
    int getFieldIndex(const std::string& structName, const std::string& fieldName) {
        auto it = structFields.find(structName);
        if (it == structFields.end()) return -1;
        for (size_t i = 0; i < it->second.size(); i++) {
            if (it->second[i].first == fieldName) return i;
        }
        return -1;
    }
    
    Type* getLLVMType(ValueType type) {
        switch (type) {
            case ValueType::VOID:    return Type::getVoidTy(context);
            case ValueType::INT32:   return Type::getInt32Ty(context);
            case ValueType::INT64:   return Type::getInt64Ty(context);
            case ValueType::INT8:    return Type::getInt8Ty(context);
            case ValueType::FLOAT32: return Type::getFloatTy(context);
            case ValueType::FLOAT64: return Type::getDoubleTy(context);
            case ValueType::BOOL:    return Type::getInt1Ty(context);
            case ValueType::POINTER: return PointerType::getUnqual(Type::getInt8Ty(context));
            case ValueType::STRING:  return PointerType::getUnqual(Type::getInt8Ty(context));
            case ValueType::ARRAY:   return PointerType::getUnqual(Type::getInt8Ty(context));
            default:                 return Type::getVoidTy(context);
        }
    }
    
    ValueType fromLLVMType(Type* type) {
        if (!type) return ValueType::VOID;
        
        if (type->isIntegerTy(32)) return ValueType::INT32;
        if (type->isIntegerTy(64)) return ValueType::INT64;
        if (type->isIntegerTy(8))  return ValueType::INT8;
        if (type->isIntegerTy(1))  return ValueType::BOOL;
        if (type->isFloatTy())     return ValueType::FLOAT32;
        if (type->isDoubleTy())    return ValueType::FLOAT64;
        if (type->isArrayTy())     return ValueType::ARRAY;
        
        if (type->isPointerTy()) {
            if (type == getLLVMType(ValueType::STRING)) {
                return ValueType::STRING;
            }
            return ValueType::POINTER;
        }
        
        if (type->isStructTy()) {
            return ValueType::POINTER;
        }
        
        return ValueType::VOID;
    }
    
    Type* getTypeFromTypeNode(ASTNode* node) {
        if (!node) return Type::getInt32Ty(context);
        
        if (node->type == AST_TYPE_INT32) return Type::getInt32Ty(context);
        if (node->type == AST_TYPE_INT64) return Type::getInt64Ty(context);
        if (node->type == AST_TYPE_INT8) return Type::getInt8Ty(context);
        if (node->type == AST_TYPE_FLOAT32) return Type::getFloatTy(context);
        if (node->type == AST_TYPE_FLOAT64) return Type::getDoubleTy(context);
        if (node->type == AST_TYPE_STRING) return PointerType::getUnqual(Type::getInt8Ty(context));
        if (node->type == AST_TYPE_VOID) return Type::getVoidTy(context);
        if (node->type == AST_TYPE_POINTER) return PointerType::getUnqual(Type::getInt8Ty(context));
        
        if (node->type == AST_TYPE_LIST) {
            Type* elementType = getArrayElementTypeFromNode(node);
            return PointerType::getUnqual(elementType);
        }
        
        if (node->type == AST_TYPE_FIXED_SIZE_LIST) {
            Type* elementType = getArrayElementTypeFromNode(node);
            int elementCount = getArrayElementCountFromNode(node);
            if (elementCount > 0) {
                return createArrayType(elementType, elementCount);
            }
            return PointerType::getUnqual(elementType);
        }
        
        if (node->type == AST_IDENTIFIER) {
            std::string typeName(node->data.identifier.name);
            StructType* st = getStructType(typeName);
            if (st) return st;
            
            auto* arrayInfo = getArrayTypeInfo(typeName);
            if (arrayInfo) {
                return PointerType::getUnqual(arrayInfo->first);
            }
        }
        
        return Type::getInt32Ty(context);
    }
    
    ValueType fromTypeNode(ASTNode* node) {
        if (!node) return ValueType::INT32;
        switch (node->type) {
            case AST_TYPE_INT32:   return ValueType::INT32;
            case AST_TYPE_INT64:   return ValueType::INT64;
            case AST_TYPE_INT8:    return ValueType::INT8;
            case AST_TYPE_FLOAT32: return ValueType::FLOAT32;
            case AST_TYPE_FLOAT64: return ValueType::FLOAT64;
            case AST_TYPE_STRING:  return ValueType::STRING;
            case AST_TYPE_VOID:    return ValueType::VOID;
            case AST_TYPE_POINTER: return ValueType::POINTER;
            case AST_TYPE_LIST:    return ValueType::ARRAY;
            case AST_TYPE_FIXED_SIZE_LIST: return ValueType::ARRAY;
            default:               return ValueType::INT32;
        }
    }
    
    ValueType getValueTypeFromType(Type* type) {
        return fromLLVMType(type);
    }
    
    std::pair<ValueType, ValueType> promoteTypes(ValueType left, ValueType right) {
        int leftRank = getTypeRank(left);
        int rightRank = getTypeRank(right);
        if (leftRank > rightRank) {
            return {left, promoteTo(right, left)};
        } else if (rightRank > leftRank) {
            return {promoteTo(left, right), right};
        }
        return {left, right};
    }
    
    ValueType promoteTo(ValueType from, ValueType to) {
        if (from == to) return from;
        if (from == ValueType::BOOL || from == ValueType::INT8 || from == ValueType::INT32) {
            if (to == ValueType::INT64 || to == ValueType::FLOAT32 || to == ValueType::FLOAT64) return to;
        }
        if (from == ValueType::INT64) {
            if (to == ValueType::FLOAT32 || to == ValueType::FLOAT64) return to;
        }
        if (from == ValueType::FLOAT32 && to == ValueType::FLOAT64) return to;
        return from;
    }
    
    Value* castValue(IRBuilder<>& builder, Value* val, ValueType from, ValueType to) {
        if (from == to) return val;
        
        if (from == ValueType::BOOL && to == ValueType::INT32)
            return builder.CreateZExt(val, Type::getInt32Ty(context), "zext");
        if (from == ValueType::INT32 && to == ValueType::INT64)
            return builder.CreateSExt(val, Type::getInt64Ty(context), "sext");
        if (from == ValueType::INT8 && to == ValueType::INT32)
            return builder.CreateSExt(val, Type::getInt32Ty(context), "sext8");
        if (from == ValueType::INT8 && to == ValueType::INT64)
            return builder.CreateSExt(val, Type::getInt64Ty(context), "sext64");
        if (from == ValueType::BOOL && to == ValueType::INT64)
            return builder.CreateZExt(val, Type::getInt64Ty(context), "zext64");
        
        if ((from == ValueType::INT32 || from == ValueType::INT64 || from == ValueType::INT8 || from == ValueType::BOOL) 
            && (to == ValueType::FLOAT32 || to == ValueType::FLOAT64)) {
            if (to == ValueType::FLOAT32)
                return builder.CreateSIToFP(val, Type::getFloatTy(context), "itof32");
            else
                return builder.CreateSIToFP(val, Type::getDoubleTy(context), "itof64");
        }
        
        if (from == ValueType::FLOAT32 && to == ValueType::FLOAT64)
            return builder.CreateFPExt(val, Type::getDoubleTy(context), "fext32to64");
        if (from == ValueType::FLOAT64 && to == ValueType::FLOAT32)
            return builder.CreateFPTrunc(val, Type::getFloatTy(context), "ftrunc64to32");
        
        if (from == ValueType::FLOAT32 && (to == ValueType::INT32 || to == ValueType::INT64 || to == ValueType::INT8))
            return builder.CreateFPToSI(val, getLLVMType(to), "ftoi");
        if (from == ValueType::FLOAT64 && (to == ValueType::INT32 || to == ValueType::INT64 || to == ValueType::INT8))
            return builder.CreateFPToSI(val, getLLVMType(to), "ftoi");
        
        if ((from == ValueType::POINTER && to == ValueType::STRING) ||
            (from == ValueType::STRING && to == ValueType::POINTER))
            return val;
        
        return val;
    }
    
private:
    int getTypeRank(ValueType type) {
        switch (type) {
            case ValueType::BOOL:    return 0;
            case ValueType::INT8:    return 1;
            case ValueType::INT32:   return 2;
            case ValueType::INT64:   return 3;
            case ValueType::FLOAT32: return 4;
            case ValueType::FLOAT64: return 5;
            default:                 return -1;
        }
    }
};

// ==================== LLVM EMITTER ====================
class LLVMCodeGenerator {
private:
    LLVMContext context;
    IRBuilder<> builder;
    std::unique_ptr<Module> module;
    ScopeManager scopeManager;
    TypeHelper typeHelper;
    Function* printfFunction;
    Function* strlenFunction;
    bool isGlobalScope;
    bool mainFunctionCreated;
    
    bool ensureValidInsertPoint() {
        BasicBlock* currentBB = builder.GetInsertBlock();
        if (currentBB) return true;
        
        Function* mainFunc = module->getFunction("main");
        if (!mainFunc) {
            if (!mainFunctionCreated) {
                createDefaultMain();
                mainFunc = module->getFunction("main");
            } else {
                return false;
            }
        }
        
        if (mainFunc) {
            BasicBlock* lastBB = nullptr;
            for (auto& bb : *mainFunc) {
                lastBB = &bb;
            }
            
            if (lastBB && !lastBB->getTerminator()) {
                builder.SetInsertPoint(lastBB);
                return true;
            }
            
            BasicBlock* newBB = BasicBlock::Create(context, "block", mainFunc);
            builder.SetInsertPoint(newBB);
            return true;
        }
        
        return false;
    }
    
    Value* safeCreateGlobalStringPtr(const std::string& str, const std::string& name) {
        if (!ensureValidInsertPoint()) {
            return nullptr;
        }
        
        if (str.empty()) {
            return builder.CreateGlobalStringPtr("", name.c_str());
        }
        
        return builder.CreateGlobalStringPtr(str, name.c_str());
    }
    
    Type* getActualType(AllocaInst* alloc) {
        if (!alloc) return nullptr;
        return alloc->getAllocatedType();
    }
    
    AllocaInst* findVariableInMain(const std::string& name) {
        Function* mainFunc = module->getFunction("main");
        if (!mainFunc) return nullptr;
        
        BasicBlock& entryBlock = mainFunc->getEntryBlock();
        for (Instruction& inst : entryBlock) {
            if (AllocaInst* alloc = dyn_cast<AllocaInst>(&inst)) {
                if (alloc->hasName() && alloc->getName() == name) {
                    return alloc;
                }
            }
        }
        return nullptr;
    }
    
    GlobalVariable* findGlobalVariable(const std::string& name) {
        return module->getGlobalVariable(name);
    }
    
    void initStrlen() {
        if (strlenFunction) return;
        
        FunctionType* strlenType = FunctionType::get(
            Type::getInt64Ty(context),
            {PointerType::getUnqual(Type::getInt8Ty(context))},
            false
        );
        
        strlenFunction = Function::Create(
            strlenType,
            Function::ExternalLinkage,
            "strlen",
            module.get()
        );
        strlenFunction->setCallingConv(CallingConv::C);
    }
    Type* getPointerElementTypeSafely(PointerType* ptrType, const std::string& varName) {
        if (!ptrType) return Type::getInt8Ty(context);
        if (varName.empty()) {
            return Type::getInt32Ty(context);
        }
        AllocaInst* alloc = scopeManager.findVariable(varName);
        if (!alloc) alloc = findVariableInMain(varName);
        if (alloc) {
            Type* allocatedType = getActualType(alloc);
            if (allocatedType->isArrayTy()) {
                ArrayType* arrType = cast<ArrayType>(allocatedType);
                Type* elemType = arrType->getElementType();
                llvm::errs() << "[DEBUG] Local array '" << varName 
                            << "': using element type from alloc: " << *elemType << "\n";
                return elemType;
        }
        auto* arrayInfo = typeHelper.getArrayTypeInfo(varName);
        if (arrayInfo) {
            llvm::errs() << "[DEBUG] Array variable '" << varName 
                        << "': using element type from array info: " 
                        << *arrayInfo->first << "\n";
            return arrayInfo->first;
        }
        if (typeHelper.isStringVariable(varName)) {
            llvm::errs() << "[DEBUG] String variable '" << varName << "': using i8 as element type\n";
            return Type::getInt8Ty(context);
        }
        if (varName == "argv") {
            llvm::errs() << "[DEBUG] argv: using char** as element type\n";
            return PointerType::getUnqual(Type::getInt8Ty(context));
        }
        if (GlobalVariable* gv = module->getGlobalVariable(varName)) {
            Type* globalType = gv->getValueType();
            if (globalType->isArrayTy()) {
                ArrayType* arrType = cast<ArrayType>(globalType);
                Type* elemType = arrType->getElementType();
                llvm::errs() << "[DEBUG] Global array '" << varName 
                            << "': using element type from global: " << *elemType << "\n";
                return elemType;
            }
            if (globalType->isPointerTy()) {
                if (auto* info = typeHelper.getArrayTypeInfo(varName)) {
                    return info->first;
                }
            }
        }
        if (varName.find("str") != std::string::npos || 
            varName.find("Str") != std::string::npos ||
            varName.find("STRING") != std::string::npos ||
            varName.find("lit") != std::string::npos) {//取巧地判断一下变量名里有没有str或者lit
            llvm::errs() << "[DEBUG] Variable '" << varName 
                        << "' looks like a string: using i8\n";
            return Type::getInt8Ty(context);
        }
        llvm::errs() << "[DEBUG] Unknown pointer '" << varName 
                    << "': defaulting to i32\n";
        return Type::getInt32Ty(context);
    }
        llvm::errs() << "[DEBUG] Unknown pointer '" << varName 
                    << "': defaulting to i8\n";
        return Type::getInt8Ty(context);//实在找不到了，默认返回i8
    }
    
public:
    struct VisitResult {
        Value* value;
        ValueType type;
        StructType* structType;
        
        VisitResult() : value(nullptr), type(ValueType::VOID), structType(nullptr) {}
        VisitResult(Value* v, ValueType t) : value(v), type(t), structType(nullptr) {}
        VisitResult(Value* v, ValueType t, StructType* st) : value(v), type(t), structType(st) {}
    };
    
    LLVMCodeGenerator() : builder(context), typeHelper(context) {
        module = std::make_unique<Module>("VixModule", context);
        std::string Triple = sys::getProcessTriple();
        module->setTargetTriple(Triple);
        printfFunction = nullptr;
        strlenFunction = nullptr;
        isGlobalScope = true;
        mainFunctionCreated = false;
        initTarget();
    }
    
    std::unique_ptr<Module> generate(ASTNode* ast_root) {
        if (!ast_root) return nullptr;
        
        initPrintf();
        initStrlen();
        visit(ast_root);
        
        bool hasMain = module->getFunction("main") != nullptr;
        
        if (!hasMain && !mainFunctionCreated) {
            createDefaultMain();
        }
        Function* mainFunc = module->getFunction("main");
        if (mainFunc) {
            BasicBlock* endBB = nullptr;
            for (auto& BB : *mainFunc) {
                if (BB.getName() == "func_end") {
                    endBB = &BB;
                    break;
                }
            }
            if (!endBB) {
                endBB = BasicBlock::Create(context, "func_end", mainFunc);
            }
            std::vector<BasicBlock*> blocks;
            for (auto& BB : *mainFunc) blocks.push_back(&BB);
            for (BasicBlock* BB : blocks) {
                if (BB == endBB) continue;
                if (!BB->getTerminator()) {
                    IRBuilder<> tmpBuilder(BB);
                    tmpBuilder.CreateBr(endBB);
                }
            }
            if (!endBB->getTerminator()) {
                IRBuilder<> tmpBuilder(endBB);
                tmpBuilder.CreateRet(ConstantInt::get(Type::getInt32Ty(context), 0));
            }
        }
        std::string error;
        raw_string_ostream errorStream(error);
        if (verifyModule(*module, &errorStream)) {
            llvm::errs() << ";Module verification failed: " << error << "\n";
            module->print(llvm::errs(), nullptr);
        }
        
        return std::move(module);
    }
    
    void initPrintf() {
        if (printfFunction) return;
        std::vector<Type*> printfArgs;
        printfArgs.push_back(PointerType::getUnqual(Type::getInt8Ty(context)));
        FunctionType* printfType = FunctionType::get(
            Type::getInt32Ty(context), printfArgs, true);
        printfFunction = Function::Create(
            printfType, Function::ExternalLinkage, "printf", module.get());
        printfFunction->setCallingConv(CallingConv::C);
    }
    
    void createDefaultMain() {
        if (module->getFunction("main") || mainFunctionCreated) return;
        std::vector<Type*> mainParamTypes;
        mainParamTypes.push_back(Type::getInt32Ty(context));  // argc
        mainParamTypes.push_back(PointerType::getUnqual(PointerType::getUnqual(Type::getInt8Ty(context))));  // argv as char**

        FunctionType* mainType = FunctionType::get(
            Type::getInt32Ty(context), mainParamTypes, false);
        Function* mainFunc = Function::Create(
            mainType, Function::ExternalLinkage, "main", module.get());
        mainFunctionCreated = true;
        BasicBlock* entryBB = BasicBlock::Create(context, "entry", mainFunc);
        builder.SetInsertPoint(entryBB);
        auto arg_it = mainFunc->arg_begin();
        arg_it->setName("argc");
        (++arg_it)->setName("argv");

        scopeManager.setCurrentFunction(mainFunc);
    }
    
    Function* getCurrentFunction() {
        return scopeManager.getCurrentFunction();
    }
    
    VisitResult visit(ASTNode* node) {
        if (!node) return VisitResult();
        
        switch (node->type) {
            case AST_NUM_INT:      return visitNumInt(node);
            case AST_NUM_FLOAT:    return visitNumFloat(node);
            case AST_STRING:       return visitString(node);
            case AST_CHAR:         return visitChar(node);
            case AST_IDENTIFIER:   return visitIdentifier(node);
            case AST_BINOP:        return visitBinOp(node);
            case AST_UNARYOP:      return visitUnaryOp(node);
            case AST_ASSIGN:       return visitAssign(node);
            case AST_PROGRAM:      return visitProgram(node);
            case AST_IF:           return visitIf(node);
            case AST_WHILE:        return visitWhile(node);
            case AST_FOR:          return visitFor(node);
            case AST_FUNCTION:     return visitFunction(node);
            case AST_CALL:         return visitCall(node);
            case AST_RETURN:       return visitReturn(node);
            case AST_PRINT:        return visitPrint(node);
            case AST_INPUT:        return visitInput(node);
            case AST_TOINT:        return visitToInt(node);
            case AST_TOFLOAT:      return visitToFloat(node);
            case AST_STRUCT_DEF:   return visitStructDef(node);
            case AST_STRUCT_LITERAL: return visitStructLiteral(node);
            case AST_MEMBER_ACCESS: return visitMemberAccess(node);
            case AST_EXPRESSION_LIST: return visitArrayLiteral(node);
            case AST_INDEX:        return visitIndex(node);
            case AST_GLOBAL:       return visitGlobal(node);
            default:               return VisitResult();
        }
    }
    
    VisitResult visitNumInt(ASTNode* node) {
        int64_t val = node->data.num_int.value;
        ValueType type;
        Value* value;
        if (val >= -2147483648LL && val <= 2147483647LL) {
            type = ValueType::INT32;
            value = ConstantInt::get(Type::getInt32Ty(context), val, true);
        } else {
            type = ValueType::INT64;
            value = ConstantInt::get(Type::getInt64Ty(context), val, true);
        }
        return VisitResult(value, type);
    }
    
    VisitResult visitNumFloat(ASTNode* node) {
        double val = node->data.num_float.value;
        Value* value = ConstantFP::get(Type::getDoubleTy(context), val);
        return VisitResult(value, ValueType::FLOAT64);
    }
    
    VisitResult visitString(ASTNode* node) {
        if (!node) {
            Value* value = safeCreateGlobalStringPtr("", "empty_str_const");
            return VisitResult(value, ValueType::STRING);
        }
        
        const char* str = node->data.string.value;
        if (!str) {
            Value* value = safeCreateGlobalStringPtr("", "null_str_const");
            return VisitResult(value, ValueType::STRING);
        }
        
        Value* value = safeCreateGlobalStringPtr(str, "str_lit");
        return VisitResult(value, ValueType::STRING);
    }

    VisitResult visitChar(ASTNode* node) {
        if (!node) {
            Value* value = ConstantInt::get(Type::getInt8Ty(context), 0);
            return VisitResult(value, ValueType::INT8);
        }

        char ch = node->data.character.value;
        Value* value = ConstantInt::get(Type::getInt8Ty(context), static_cast<int8_t>(ch));
        return VisitResult(value, ValueType::INT8);
    }

    VisitResult visitIdentifier(ASTNode* node) {
        if (!node || !node->data.identifier.name) return VisitResult();
        
        std::string name(node->data.identifier.name);
        AllocaInst* alloc = scopeManager.findVariable(name);
        
        if (!alloc) {
            if (module->getFunction(name)) {
                return VisitResult(nullptr, ValueType::VOID);
            }
            
            alloc = findVariableInMain(name);
            if (alloc) {
                scopeManager.defineVariable(name, alloc);
            } else {
                GlobalVariable* globalVar = findGlobalVariable(name);
                if (globalVar) {
                    Type* globalType = globalVar->getValueType();
                    if (globalType && globalType->isStructTy()) {
                        StructType* st = cast<StructType>(globalType);
                        return VisitResult(globalVar, ValueType::POINTER, st);
                    }
                    Value* loadedValue = builder.CreateLoad(globalType, globalVar, name);
                    ValueType valueType = typeHelper.getValueTypeFromType(globalType);
                    return VisitResult(loadedValue, valueType);
                } else {
                    llvm::errs() << "Warning: Use of undeclared variable '" << name << "'\n";
                    return VisitResult(ConstantInt::get(Type::getInt32Ty(context), 0), ValueType::INT32);
                }
            }
        }
        
        Type* allocatedType = getActualType(alloc);
        
        // 检查是否是字符串变量
        bool isStringVar = typeHelper.isStringVariable(name);
        
        if (allocatedType && allocatedType->isStructTy()) {
            StructType* st = cast<StructType>(allocatedType);
            return VisitResult(alloc, ValueType::POINTER, st);
        }
        
        // 如果是字符串变量，需要正确处理
        if (isStringVar) {
            llvm::errs() << "[DEBUG] String variable '" << name << "' loading value\n";
            Value* val = builder.CreateLoad(allocatedType, alloc, name);
            // 对于字符串，返回的是指向字符数组或字符指针的值
            ValueType vt = typeHelper.getValueTypeFromType(allocatedType);
            return VisitResult(val, vt);
        }

        Value* val = builder.CreateLoad(allocatedType, alloc, name);
        ValueType type = typeHelper.getValueTypeFromType(allocatedType);
        return VisitResult(val, type);
    }
    
    // ==================== 修复：visitBinOp - 确保比较操作类型一致 ====================
    VisitResult visitBinOp(ASTNode* node) {
        if (!node || !node->data.binop.left || !node->data.binop.right)
            return VisitResult();
        
        VisitResult leftRes = visit(node->data.binop.left);
        VisitResult rightRes = visit(node->data.binop.right);
        if (!leftRes.value || !rightRes.value) {
            return VisitResult(ConstantInt::get(Type::getInt32Ty(context), 0), ValueType::INT32);
        }
        
        // 检查操作数类型并确保它们兼容
        // 特别处理 i8 类型的比较操作
        if ((leftRes.type == ValueType::INT8 || rightRes.type == ValueType::INT8) &&
            (node->data.binop.op == OP_EQ || node->data.binop.op == OP_NE ||
             node->data.binop.op == OP_LT || node->data.binop.op == OP_LE ||
             node->data.binop.op == OP_GT || node->data.binop.op == OP_GE)) {
            
            // 对于比较操作，将两个操作数都提升到 i32 以确保类型一致
            Value* leftVal = leftRes.value;
            Value* rightVal = rightRes.value;
            
            // 确保两个操作数都是整数类型
            if (!leftVal->getType()->isIntegerTy() || !rightVal->getType()->isIntegerTy()) {
                llvm::errs() << "Error: Comparison operands must be integers\n";
                return VisitResult();
            }
            
            // 将两个操作数都扩展到 i32（如果它们不是 i32）
            if (leftVal->getType() != Type::getInt32Ty(context)) {
                if (leftVal->getType()->isIntegerTy(1)) {
                    leftVal = builder.CreateZExt(leftVal, Type::getInt32Ty(context), "left_zext");
                } else if (leftVal->getType()->getIntegerBitWidth() < 32) {
                    leftVal = builder.CreateSExt(leftVal, Type::getInt32Ty(context), "left_sext");
                }
            }
            
            if (rightVal->getType() != Type::getInt32Ty(context)) {
                if (rightVal->getType()->isIntegerTy(1)) {
                    rightVal = builder.CreateZExt(rightVal, Type::getInt32Ty(context), "right_zext");
                } else if (rightVal->getType()->getIntegerBitWidth() < 32) {
                    rightVal = builder.CreateSExt(rightVal, Type::getInt32Ty(context), "right_sext");
                }
            }
            
            // 执行比较操作
            switch (node->data.binop.op) {
                case OP_EQ:
                    return VisitResult(builder.CreateICmpEQ(leftVal, rightVal, "eqtmp"), ValueType::BOOL);
                case OP_NE:
                    return VisitResult(builder.CreateICmpNE(leftVal, rightVal, "netmp"), ValueType::BOOL);
                case OP_LT:
                    return VisitResult(builder.CreateICmpSLT(leftVal, rightVal, "lttmp"), ValueType::BOOL);
                case OP_LE:
                    return VisitResult(builder.CreateICmpSLE(leftVal, rightVal, "letmp"), ValueType::BOOL);
                case OP_GT:
                    return VisitResult(builder.CreateICmpSGT(leftVal, rightVal, "gttmp"), ValueType::BOOL);
                case OP_GE:
                    return VisitResult(builder.CreateICmpSGE(leftVal, rightVal, "getmp"), ValueType::BOOL);
                default:
                    return VisitResult();
            }
        }
        
        // 原有的类型提升逻辑
        auto [promotedLeftType, promotedRightType] = typeHelper.promoteTypes(
            leftRes.type, rightRes.type);
        
        Value* leftVal = typeHelper.castValue(builder, leftRes.value, 
                                              leftRes.type, promotedLeftType);
        Value* rightVal = typeHelper.castValue(builder, rightRes.value, 
                                               rightRes.type, promotedRightType);
        
        ValueType resultType = (promotedLeftType > promotedRightType) 
                               ? promotedLeftType : promotedRightType;
        bool isFloat = (resultType == ValueType::FLOAT32 || resultType == ValueType::FLOAT64);
        
        switch (node->data.binop.op) {
            case OP_ADD:
                if (isFloat) return VisitResult(builder.CreateFAdd(leftVal, rightVal, "addtmp"), resultType);
                else return VisitResult(builder.CreateAdd(leftVal, rightVal, "addtmp"), resultType);
            case OP_SUB:
                if (isFloat) return VisitResult(builder.CreateFSub(leftVal, rightVal, "subtmp"), resultType);
                else return VisitResult(builder.CreateSub(leftVal, rightVal, "subtmp"), resultType);
            case OP_MUL:
                if (isFloat) return VisitResult(builder.CreateFMul(leftVal, rightVal, "multmp"), resultType);
                else return VisitResult(builder.CreateMul(leftVal, rightVal, "multmp"), resultType);
            case OP_DIV:
                if (isFloat) return VisitResult(builder.CreateFDiv(leftVal, rightVal, "divtmp"), resultType);
                else return VisitResult(builder.CreateSDiv(leftVal, rightVal, "divtmp"), resultType);
            case OP_MOD:
                if (isFloat) return VisitResult();
                else return VisitResult(builder.CreateSRem(leftVal, rightVal, "modtmp"), resultType);
            case OP_EQ:
                if (isFloat) return VisitResult(builder.CreateFCmpOEQ(leftVal, rightVal, "eqtmp"), ValueType::BOOL);
                else return VisitResult(builder.CreateICmpEQ(leftVal, rightVal, "eqtmp"), ValueType::BOOL);
            case OP_NE:
                if (isFloat) return VisitResult(builder.CreateFCmpONE(leftVal, rightVal, "netmp"), ValueType::BOOL);
                else return VisitResult(builder.CreateICmpNE(leftVal, rightVal, "netmp"), ValueType::BOOL);
            case OP_LT:
                if (isFloat) return VisitResult(builder.CreateFCmpOLT(leftVal, rightVal, "lttmp"), ValueType::BOOL);
                else return VisitResult(builder.CreateICmpSLT(leftVal, rightVal, "lttmp"), ValueType::BOOL);
            case OP_LE:
                if (isFloat) return VisitResult(builder.CreateFCmpOLE(leftVal, rightVal, "letmp"), ValueType::BOOL);
                else return VisitResult(builder.CreateICmpSLE(leftVal, rightVal, "letmp"), ValueType::BOOL);
            case OP_GT:
                if (isFloat) return VisitResult(builder.CreateFCmpOGT(leftVal, rightVal, "gttmp"), ValueType::BOOL);
                else return VisitResult(builder.CreateICmpSGT(leftVal, rightVal, "gttmp"), ValueType::BOOL);
            case OP_GE:
                if (isFloat) return VisitResult(builder.CreateFCmpOGE(leftVal, rightVal, "getmp"), ValueType::BOOL);
                else return VisitResult(builder.CreateICmpSGE(leftVal, rightVal, "getmp"), ValueType::BOOL);
            case OP_AND:
                {
                    Value* leftBool = leftRes.type == ValueType::BOOL ? leftVal : 
                                    builder.CreateICmpNE(leftVal, ConstantInt::get(leftVal->getType(), 0), "and_lhs");
                    Value* rightBool = rightRes.type == ValueType::BOOL ? rightVal : 
                                     builder.CreateICmpNE(rightVal, ConstantInt::get(rightVal->getType(), 0), "and_rhs");
                    Value* result = builder.CreateAnd(leftBool, rightBool, "andtmp");
                    return VisitResult(result, ValueType::BOOL);
                }
            case OP_OR:
                {
                    Value* leftBool = leftRes.type == ValueType::BOOL ? leftVal : 
                                    builder.CreateICmpNE(leftVal, ConstantInt::get(leftVal->getType(), 0), "or_lhs");
                    Value* rightBool = rightRes.type == ValueType::BOOL ? rightVal : 
                                     builder.CreateICmpNE(rightVal, ConstantInt::get(rightVal->getType(), 0), "or_rhs");
                    Value* result = builder.CreateOr(leftBool, rightBool, "ortmp");
                    return VisitResult(result, ValueType::BOOL);
                }
            default: return VisitResult();
        }
    }
    
    VisitResult visitUnaryOp(ASTNode* node) {
        if (!node || !node->data.unaryop.expr) return VisitResult();
        VisitResult operand = visit(node->data.unaryop.expr);
        if (!operand.value) return VisitResult();
        switch (node->data.unaryop.op) {
            case OP_MINUS:
                if (operand.type == ValueType::FLOAT32 || operand.type == ValueType::FLOAT64)
                    return VisitResult(builder.CreateFNeg(operand.value, "negtmp"), operand.type);
                else
                    return VisitResult(builder.CreateNeg(operand.value, "negtmp"), operand.type);
            case OP_PLUS: return operand;
            default: return VisitResult();
        }
    }
    
    VisitResult visitAssign(ASTNode* node) {
        if (!node || !node->data.assign.left || !node->data.assign.right)
            return VisitResult();
        
        if (node->data.assign.right->type == AST_STRUCT_LITERAL) {
            return visitStructAssign(node);
        }
        
        if (node->data.assign.left->type == AST_MEMBER_ACCESS) {
            return visitMemberAssign(node);
        }

        if (node->data.assign.left->type == AST_INDEX) {
            return visitIndexAssign(node);
        }

        if (node->data.assign.left->type != AST_IDENTIFIER)
            return VisitResult();
        
        std::string name(node->data.assign.left->data.identifier.name);
        VisitResult rightVal = visit(node->data.assign.right);
        if (!rightVal.value) return VisitResult();
        
        // 检查是否赋值字符串常量给字符串变量
        bool isStringAssign = (node->data.assign.right->type == AST_STRING);
        
        AllocaInst* alloc = scopeManager.findVariable(name);
        if (!alloc) {
            GlobalVariable* gvar = findGlobalVariable(name);
            if (gvar) {
                Function* func = getCurrentFunction();
                if (!func) {
                    func = module->getFunction("main");
                    if (!func) {
                        createDefaultMain();
                        func = module->getFunction("main");
                    }
                }

                Type* globalType = gvar->getValueType();
                ValueType gvt = typeHelper.getValueTypeFromType(globalType);
                Value* castedVal = typeHelper.castValue(builder, rightVal.value, rightVal.type, gvt);
                builder.CreateStore(castedVal, gvar);
                return VisitResult(castedVal, gvt);
            }
            
            Function* func = getCurrentFunction();
            if (!func) {
                func = module->getFunction("main");
                if (!func) {
                    createDefaultMain();
                    func = module->getFunction("main");
                }
            }
            if (!func) return VisitResult();
            
            BasicBlock* entryBB = &func->getEntryBlock();
            BasicBlock* savedBB = builder.GetInsertBlock();
            
            Type* varType = nullptr;
            if (isStringAssign) {
                varType = PointerType::getUnqual(Type::getInt8Ty(context));
            } else {
                varType = rightVal.value->getType();
            }
            
            IRBuilder<> tempBuilder(entryBB, entryBB->begin());
            alloc = tempBuilder.CreateAlloca(varType, nullptr, name);
            
            if (savedBB) {
                builder.SetInsertPoint(savedBB);
            }
            scopeManager.defineVariable(name, alloc);
            
            if (node->data.assign.right->type == AST_STRING) {
                const char* s_val = node->data.assign.right->data.string.value;
                int strlen_val = 0;
                if (s_val) strlen_val = (int)strlen(s_val);
                typeHelper.registerArrayType(name, Type::getInt8Ty(context), strlen_val);
                typeHelper.registerVariableArraySize(name, strlen_val);
                typeHelper.registerStringVariable(name);
            }

            if (node->data.assign.right->type == AST_EXPRESSION_LIST) {
                int arraySize = node->data.assign.right->data.expression_list.expression_count;
                typeHelper.registerVariableArraySize(name, arraySize);
                
                if (rightVal.value && rightVal.value->getType()->isPointerTy()) {
                    Type* elemType = Type::getInt32Ty(context);
                    typeHelper.registerArrayType(name, elemType, arraySize);
                }
            }
        }
        
        Type* allocatedType = getActualType(alloc);
        ValueType varType = typeHelper.getValueTypeFromType(allocatedType);
        Value* val = typeHelper.castValue(builder, rightVal.value, rightVal.type, varType);
        builder.CreateStore(val, alloc);
        return VisitResult(val, varType);
    }
    
    VisitResult visitIndexAssign(ASTNode* node) {
        ASTNode* indexNode = node->data.assign.left;
        ASTNode* target = indexNode->data.index.target;
        ASTNode* idxExpr = indexNode->data.index.index;

        VisitResult idxRes = visit(idxExpr);
        if (!idxRes.value) return VisitResult();
        Value* idxVal = idxRes.value;
        if (!idxVal->getType()->isIntegerTy(32)) {
            idxVal = builder.CreateIntCast(idxVal, Type::getInt32Ty(context), true, "idxcast");
        }

        AllocaInst* baseAlloc = nullptr;
        std::string varName;
        if (target->type == AST_IDENTIFIER) {
            varName = std::string(target->data.identifier.name);
            baseAlloc = scopeManager.findVariable(varName);
            if (!baseAlloc) baseAlloc = findVariableInMain(varName);
        }

        if (baseAlloc) {
            Type* allocatedType = getActualType(baseAlloc);
            if (allocatedType && allocatedType->isArrayTy()) {
                ArrayType* at = cast<ArrayType>(allocatedType);
                Type* elemType = at->getElementType();
                Value* gep = builder.CreateInBoundsGEP(allocatedType, baseAlloc, 
                    {ConstantInt::get(Type::getInt32Ty(context),0), idxVal}, "arr_index_ptr");

                VisitResult rightVal = visit(node->data.assign.right);
                if (!rightVal.value) return VisitResult();
                ValueType vt = typeHelper.getValueTypeFromType(elemType);
                Value* casted = typeHelper.castValue(builder, rightVal.value, rightVal.type, vt);
                builder.CreateStore(casted, gep);
                return VisitResult(casted, vt);
            }
            
            if (allocatedType && allocatedType->isPointerTy()) {
                Value* arrayPtr = builder.CreateLoad(allocatedType, baseAlloc, "array_ptr");
                Type* elemType = getPointerElementTypeSafely(dyn_cast<PointerType>(allocatedType), varName);
                if (varName == "argv") {
                    elemType = PointerType::getUnqual(Type::getInt8Ty(context));
                }

                Value* gep = builder.CreateInBoundsGEP(elemType, arrayPtr, idxVal, "ptr_index_ptr");
                VisitResult rightVal = visit(node->data.assign.right);
                if (!rightVal.value) return VisitResult();
                ValueType vt = typeHelper.getValueTypeFromType(elemType);
                Value* casted = typeHelper.castValue(builder, rightVal.value, rightVal.type, vt);
                builder.CreateStore(casted, gep);
                return VisitResult(casted, vt);
            }
        }

        VisitResult targRes = visit(target);
        if (!targRes.value) return VisitResult();

        if (targRes.value->getType()->isPointerTy()) {
            if (target->type == AST_IDENTIFIER) {
                varName = std::string(target->data.identifier.name);
            }
            Type* elemType = getPointerElementTypeSafely(dyn_cast<PointerType>(targRes.value->getType()), varName);

            Value* gep = builder.CreateInBoundsGEP(elemType, targRes.value, idxVal, "arr_index_ptr");

            if (gep) {
                VisitResult rightVal = visit(node->data.assign.right);
                if (!rightVal.value) return VisitResult();
                ValueType vt = typeHelper.getValueTypeFromType(elemType);
                Value* casted = typeHelper.castValue(builder, rightVal.value, rightVal.type, vt);
                builder.CreateStore(casted, gep);
                return VisitResult(casted, vt);
            }
        }

        return VisitResult();
    }
    
    VisitResult visitStructAssign(ASTNode* node) {
        if (!node || node->type != AST_ASSIGN) return VisitResult();
        
        ASTNode* left = node->data.assign.left;
        ASTNode* right = node->data.assign.right;
        
        if (!left || left->type != AST_IDENTIFIER) return VisitResult();
        if (!right || right->type != AST_STRUCT_LITERAL) return VisitResult();
        
        std::string varName(left->data.identifier.name);
        
        ASTNode* typeNameNode = right->data.struct_literal.type_name;
        if (!typeNameNode || typeNameNode->type != AST_IDENTIFIER) return VisitResult();
        
        std::string structName(typeNameNode->data.identifier.name);
        StructType* structType = typeHelper.getStructType(structName);
        if (!structType) return VisitResult();
        
        Function* func = getCurrentFunction();
        if (!func) {
            func = module->getFunction("main");
            if (!func) {
                createDefaultMain();
                func = module->getFunction("main");
            }
            if (func) {
                builder.SetInsertPoint(&func->getEntryBlock());
            }
        }
        
        if (!func) return VisitResult();
        BasicBlock* entryBB = &func->getEntryBlock();
        BasicBlock* savedBB = builder.GetInsertBlock();
        
        IRBuilder<> tempBuilder(entryBB, entryBB->begin());
        AllocaInst* alloc = tempBuilder.CreateAlloca(structType, nullptr, varName);
        
        if (savedBB) {
            builder.SetInsertPoint(savedBB);
        }
        
        scopeManager.defineVariable(varName, alloc);
        initStructLiteral(alloc, right);
        return VisitResult(alloc, ValueType::POINTER, structType);
    }
    
    void initStructLiteral(AllocaInst* structAlloc, ASTNode* node) {
        if (!node || node->type != AST_STRUCT_LITERAL) return;
        
        ASTNode* typeName = node->data.struct_literal.type_name;
        ASTNode* fields = node->data.struct_literal.fields;
        if (!typeName || typeName->type != AST_IDENTIFIER) return;
        
        std::string structName(typeName->data.identifier.name);
        StructType* structType = typeHelper.getStructType(structName);
        auto* fieldInfo = typeHelper.getStructFields(structName);
        if (!structType || !fieldInfo) return;
        
        if (!fields || fields->type != AST_EXPRESSION_LIST) return;
        
        int fieldCount = fields->data.expression_list.expression_count;
        for (size_t i = 0; i < fieldInfo->size(); i++) {
            const auto& field = (*fieldInfo)[i];
            std::string fieldName = field.first;
            Type* expectedType = field.second;
            
            Value* fieldPtr = builder.CreateStructGEP(structType, structAlloc, i, fieldName);
            Value* defaultValue = nullptr;
            
            if (expectedType->isPointerTy()) {
                defaultValue = ConstantPointerNull::get(cast<PointerType>(expectedType));
            } else if (expectedType->isIntegerTy()) {
                defaultValue = ConstantInt::get(expectedType, 0);
            } else if (expectedType->isFloatTy()) {
                defaultValue = ConstantFP::get(expectedType, 0.0);
            } else if (expectedType->isDoubleTy()) {
                defaultValue = ConstantFP::get(expectedType, 0.0);
            } else if (expectedType->isStructTy()) {
                defaultValue = Constant::getNullValue(expectedType);
            } else {
                continue;
            }

            builder.CreateStore(defaultValue, fieldPtr);
        }
        for (int i = 0; i < fieldCount && i < (int)fieldInfo->size(); i++) {
            ASTNode* fieldExpr = fields->data.expression_list.expressions[i];
            
            if (fieldExpr && fieldExpr->type == AST_ASSIGN) {
                ASTNode* fieldNameNode = fieldExpr->data.assign.left;
                ASTNode* exprNode = fieldExpr->data.assign.right;
                
                if (fieldNameNode && fieldNameNode->type == AST_IDENTIFIER && exprNode) {
                    std::string fieldName(fieldNameNode->data.identifier.name);
                    
                    int idx = -1;
                    for (size_t j = 0; j < fieldInfo->size(); j++) {
                        if ((*fieldInfo)[j].first == fieldName) {
                            idx = j;
                            break;
                        }
                    }
                    
                    if (idx >= 0) {
                        VisitResult fieldVal = visit(exprNode);
                        if (fieldVal.value) {
                            Value* fieldPtr = builder.CreateStructGEP(structType, structAlloc, idx, fieldName);
                            Type* expectedType = (*fieldInfo)[idx].second;
                            
                            if (expectedType->isStructTy()) {
                                Value* toStore = nullptr;
                                if (fieldVal.value->getType()->isPointerTy()) {
                                    toStore = builder.CreateLoad(expectedType, fieldVal.value, fieldName + "_val");
                                } else if (fieldVal.value->getType() == expectedType) {
                                    toStore = fieldVal.value;
                                }
                                if (toStore) builder.CreateStore(toStore, fieldPtr);
                            } else {
                                ValueType expectedValueType = typeHelper.getValueTypeFromType(expectedType);
                                Value* castedVal = typeHelper.castValue(builder, fieldVal.value, 
                                                                       fieldVal.type, expectedValueType);
                                builder.CreateStore(castedVal, fieldPtr);
                            }
                        }
                    }
                }
            }
        }
    }
    
    VisitResult visitMemberAssign(ASTNode* node) {
        if (!node || node->type != AST_ASSIGN) return VisitResult();
        
        ASTNode* left = node->data.assign.left;
        ASTNode* right = node->data.assign.right;
        if (!left || left->type != AST_MEMBER_ACCESS) return VisitResult();
        
        ASTNode* object = left->data.member_access.object;
        ASTNode* field = left->data.member_access.field;
        if (!object || !field) return VisitResult();
        
        VisitResult objectRes = visit(object);
        if (!objectRes.value) return VisitResult();
        
        if (!objectRes.value->getType()->isPointerTy()) return VisitResult();
        if (field->type != AST_IDENTIFIER) return VisitResult();
        
        std::string fieldName(field->data.identifier.name);
        
        StructType* structType = nullptr;
        Value* basePtr = nullptr;
        
        if (objectRes.structType) {
            structType = objectRes.structType;
            basePtr = objectRes.value;
        } else if (AllocaInst* alloc = dyn_cast<AllocaInst>(objectRes.value)) {
            Type* allocatedType = getActualType(alloc);
            if (allocatedType->isStructTy()) {
                structType = cast<StructType>(allocatedType);
                basePtr = alloc;
            }
        } else if (LoadInst* load = dyn_cast<LoadInst>(objectRes.value)) {
            Value* ptr = load->getPointerOperand();
            if (ptr->getType()->isPointerTy()) {
                if (AllocaInst* allocPtr = dyn_cast<AllocaInst>(ptr)) {
                    Type* allocatedType = getActualType(allocPtr);
                    if (allocatedType->isStructTy()) {
                        structType = cast<StructType>(allocatedType);
                        basePtr = ptr;
                    }
                }
            }
        }
        
        if (!structType || !basePtr) {
            llvm::errs() << "Error: Cannot assign to member '" << fieldName 
                        << "' of non-struct type\n";
            return VisitResult();
        }
        
        std::string structName = structType->getName().str();
        int idx = typeHelper.getFieldIndex(structName, fieldName);
        if (idx < 0) {
            llvm::errs() << "Error: Struct '" << structName 
                        << "' has no member named '" << fieldName << "'\n";
            return VisitResult();
        }
        
        Value* fieldPtr = builder.CreateStructGEP(structType, basePtr, idx, fieldName);
        
        VisitResult rightVal = visit(right);
        if (!rightVal.value) return VisitResult();
        
        Type* fieldType = structType->getElementType(idx);
        ValueType fieldValueType = typeHelper.getValueTypeFromType(fieldType);
        Value* castedVal = typeHelper.castValue(builder, rightVal.value, rightVal.type, fieldValueType);
        
        builder.CreateStore(castedVal, fieldPtr);
        
        return VisitResult(castedVal, fieldValueType, structType);
    }
    
    VisitResult visitProgram(ASTNode* node) {
        if (!node) return VisitResult();
        for (int i = 0; i < node->data.program.statement_count; i++) {
            visit(node->data.program.statements[i]);
        }
        
        return VisitResult();
    }
    
    VisitResult visitIf(ASTNode* node) {
        Function* func = getCurrentFunction();
        if (!func) return VisitResult();
        
        VisitResult condRes = visit(node->data.if_stmt.condition);
        if (!condRes.value) return VisitResult();
        
        Value* cond = condRes.value;
        if (cond->getType() != Type::getInt1Ty(context)) {
            cond = builder.CreateICmpNE(cond, ConstantInt::get(cond->getType(), 0), "ifcond");
        }
        
        BasicBlock* thenBB = BasicBlock::Create(context, "then", func);
        BasicBlock* elseBB = BasicBlock::Create(context, "else");
        BasicBlock* mergeBB = BasicBlock::Create(context, "ifcont");
        
        builder.CreateCondBr(cond, thenBB, elseBB);
        
        builder.SetInsertPoint(thenBB);
        scopeManager.enterScope();
        visit(node->data.if_stmt.then_body);
        scopeManager.exitScope();
        thenBB = builder.GetInsertBlock();
        if (!thenBB->getTerminator()) {
            builder.CreateBr(mergeBB);
        }
        
        func->insert(func->end(), elseBB);
        func->insert(func->end(), mergeBB);
        
        builder.SetInsertPoint(elseBB);
        if (node->data.if_stmt.else_body) {
            scopeManager.enterScope();
            visit(node->data.if_stmt.else_body);
            scopeManager.exitScope();
        }
        elseBB = builder.GetInsertBlock();
        if (!elseBB->getTerminator()) {
            builder.CreateBr(mergeBB);
        }
        
        builder.SetInsertPoint(mergeBB);
        return VisitResult();
    }
    
    VisitResult visitWhile(ASTNode* node) {
        Function* func = getCurrentFunction();
        if (!func) return VisitResult();
        
        BasicBlock* condBB = BasicBlock::Create(context, "whilecond", func);
        BasicBlock* loopBB = BasicBlock::Create(context, "whilebody");
        BasicBlock* afterBB = BasicBlock::Create(context, "whilecont");
        
        builder.CreateBr(condBB);
        
        builder.SetInsertPoint(condBB);
        VisitResult condRes = visit(node->data.while_stmt.condition);
        if (!condRes.value) return VisitResult();
        
        Value* cond = condRes.value;
        if (cond->getType() != Type::getInt1Ty(context)) {
            cond = builder.CreateICmpNE(cond, ConstantInt::get(cond->getType(), 0), "whilecond");
        }
        func->insert(func->end(), loopBB);
        func->insert(func->end(), afterBB);
        builder.CreateCondBr(cond, loopBB, afterBB);
        builder.SetInsertPoint(loopBB);
        scopeManager.enterScope();
        visit(node->data.while_stmt.body);
        scopeManager.exitScope();
        loopBB = builder.GetInsertBlock();
        if (!loopBB->getTerminator()) {
            builder.CreateBr(condBB);
        }
        
        builder.SetInsertPoint(afterBB);
        return VisitResult();
    }
    
    VisitResult visitFor(ASTNode* node) {
        if (!node) return VisitResult();
        
        Function* func = getCurrentFunction();
        if (!func) {
            llvm::errs() << "[ERROR] No current function in for loop\n";
            return VisitResult();
        }
        ASTNode* var_node = node->data.for_stmt.var;
        ASTNode* start_node = node->data.for_stmt.start;
        ASTNode* end_node = node->data.for_stmt.end;
        ASTNode* body_node = node->data.for_stmt.body;
        if (!var_node || !start_node || !end_node || !body_node) return VisitResult();
        if (var_node->type != AST_IDENTIFIER) return VisitResult();
        std::string var_name(var_node->data.identifier.name);
        VisitResult start_val = visit(start_node);
        if (!start_val.value) return VisitResult();
        VisitResult end_val = visit(end_node);
        if (!end_val.value) {
            llvm::errs() << "[ERROR] Failed to evaluate for loop end condition\n";
            return VisitResult();
        }
        
        llvm::errs() << "[DEBUG] For loop end value type: " << *end_val.value->getType() << "\n";
        AllocaInst* var_alloc = scopeManager.findVariable(var_name);
        if (!var_alloc) {
            BasicBlock* entryBB = &func->getEntryBlock();
            BasicBlock* savedBB = builder.GetInsertBlock();
            
            Type* var_type = Type::getInt32Ty(context);
            IRBuilder<> tempBuilder(entryBB, entryBB->begin());
            var_alloc = tempBuilder.CreateAlloca(var_type, nullptr, var_name);
            
            if (savedBB) {
                builder.SetInsertPoint(savedBB);
            }
            
            scopeManager.defineVariable(var_name, var_alloc);
        }
        Value* start_val_casted = typeHelper.castValue(builder, start_val.value, start_val.type, ValueType::INT32);
        builder.CreateStore(start_val_casted, var_alloc);
        Value* end_val_casted = typeHelper.castValue(builder, end_val.value, end_val.type, ValueType::INT32);
        BasicBlock* condBB = BasicBlock::Create(context, "forcond", func);
        BasicBlock* loopBB = BasicBlock::Create(context, "forbody");
        BasicBlock* incBB = BasicBlock::Create(context, "forinc");
        BasicBlock* afterBB = BasicBlock::Create(context, "forcont");
        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);
        Value* cur_val = builder.CreateLoad(Type::getInt32Ty(context), var_alloc, var_name);
        Value* cond = builder.CreateICmpSLT(cur_val, end_val_casted, "forcond");
        llvm::errs() << "[DEBUG] for cond: " << *cur_val->getType()
                     << " < " << *end_val_casted->getType() << "\n";
        func->insert(func->end(), loopBB);
        func->insert(func->end(), incBB);
        func->insert(func->end(), afterBB);
        builder.CreateCondBr(cond, loopBB, afterBB);
        builder.SetInsertPoint(loopBB);
        scopeManager.enterScope();
        visit(body_node);
        scopeManager.exitScope();
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(incBB);
        }
        builder.SetInsertPoint(incBB);
        Value* cur_val_for_inc = builder.CreateLoad(Type::getInt32Ty(context), var_alloc, var_name);
        Value* one_val = ConstantInt::get(Type::getInt32Ty(context), 1);
        Value* new_val = builder.CreateAdd(cur_val_for_inc, one_val, "inc");
        builder.CreateStore(new_val, var_alloc);
        builder.CreateBr(condBB);
        
        builder.SetInsertPoint(afterBB);
        return VisitResult();
    }
    
    VisitResult visitFunction(ASTNode* node) {
        std::string funcName(node->data.function.name);
        
        if (Function* existingFunc = module->getFunction(funcName)) {
            return VisitResult(existingFunc, typeHelper.fromLLVMType(existingFunc->getReturnType()));
        }
        
        std::vector<Type*> paramTypes;
        std::vector<std::string> paramNames;
        std::vector<ValueType> paramValueTypes;
        
        if (node->data.function.params) {
            if (node->data.function.params->type == AST_EXPRESSION_LIST) {
                int paramCount = node->data.function.params->data.expression_list.expression_count;
                for (int i = 0; i < paramCount; i++) {
                    ASTNode* param = node->data.function.params->data.expression_list.expressions[i];
                    
                    if (param->type == AST_ASSIGN) {
                        ASTNode* left = param->data.assign.left;
                        ASTNode* right = param->data.assign.right;
                        if (left && left->type == AST_IDENTIFIER) {
                            std::string paramName(left->data.identifier.name);
                            paramNames.push_back(paramName);
                            ValueType paramType = ValueType::INT32;
                            if (right && right->type == AST_IDENTIFIER) {
                                std::string typeName(right->data.identifier.name);
                                if (typeName == "ptr") {
                                    if (funcName == "main" && paramName == "argv") {
                                        paramType = ValueType::POINTER;
                                        paramTypes.push_back(PointerType::getUnqual(PointerType::getUnqual(Type::getInt8Ty(context))));
                                    } else {
                                        paramType = ValueType::POINTER;
                                        paramTypes.push_back(PointerType::getUnqual(Type::getInt8Ty(context)));
                                        typeHelper.registerStringVariable(paramName);//未指明更具体类型的 ptr 参数，通常按字符串/字节指针处理
                                    }
                                } else if (typeName == "i32") {
                                    paramType = ValueType::INT32;
                                    paramTypes.push_back(Type::getInt32Ty(context));
                                } else if (typeName == "i64") {
                                    paramType = ValueType::INT64;
                                    paramTypes.push_back(Type::getInt64Ty(context));
                                } else if (typeName == "f32") {
                                    paramType = ValueType::FLOAT32;
                                    paramTypes.push_back(Type::getFloatTy(context));
                                } else if (typeName == "f64") {
                                    paramType = ValueType::FLOAT64;
                                    paramTypes.push_back(Type::getDoubleTy(context));
                                } else if (typeName == "str") {
                                    paramType = ValueType::STRING;
                                    paramTypes.push_back(PointerType::getUnqual(Type::getInt8Ty(context)));
                                } else {
                                    paramTypes.push_back(Type::getInt32Ty(context));
                                }
                            }
                            else if (right) {
                                paramType = typeHelper.fromTypeNode(right);
                                if (right->type == AST_TYPE_POINTER) {
                                    if (funcName == "main" && paramName == "argv") {
                                        paramTypes.push_back(PointerType::getUnqual(PointerType::getUnqual(Type::getInt8Ty(context))));
                                    } else {
                                        paramTypes.push_back(PointerType::getUnqual(Type::getInt8Ty(context)));
                                    }
                                } else {
                                    paramTypes.push_back(typeHelper.getLLVMType(paramType));
                                }
                            } else {
                                paramTypes.push_back(Type::getInt32Ty(context));
                            }
                            paramValueTypes.push_back(paramType);
                            if (paramType == ValueType::STRING) {
                                typeHelper.registerStringVariable(paramName);
                            }
                        }
                    } else if (param->type == AST_IDENTIFIER) {
                        std::string paramName(param->data.identifier.name);
                        paramNames.push_back(paramName);
                        paramValueTypes.push_back(ValueType::INT32);
                        paramTypes.push_back(Type::getInt32Ty(context));
                    }
                }
            }
        }
        ValueType returnValueType = ValueType::VOID;
        if (node->data.function.return_type) {
            returnValueType = typeHelper.fromTypeNode(node->data.function.return_type);
        }
        Type* returnType = typeHelper.getLLVMType(returnValueType);
        bool isVarArg = node->data.function.vararg == 1;
        FunctionType* funcType = FunctionType::get(returnType, paramTypes, isVarArg);
        Function* func = Function::Create(funcType, Function::ExternalLinkage, funcName, module.get());
        if (node->data.function.is_extern && node->data.function.body == NULL) {
            return VisitResult(func, returnValueType);
        }
        
        if (funcName == "main") {
            mainFunctionCreated = true;
        }
        Function* prevFunc = scopeManager.getCurrentFunction();
        scopeManager.setCurrentFunction(func);
        BasicBlock* entryBB = BasicBlock::Create(context, "entry", func);
        builder.SetInsertPoint(entryBB);
        scopeManager.enterScope();
        unsigned idx = 0;
        for (auto& arg : func->args()) {
            if (idx < paramNames.size()) {
                arg.setName(paramNames[idx]);
                Type* paramAllocType = nullptr;
                if (idx < paramValueTypes.size()) {
                    ValueType vt = paramValueTypes[idx];
                    if (vt == ValueType::POINTER || vt == ValueType::STRING) {
                        if (funcName == "main" && paramNames[idx] == "argv") {
                            paramAllocType = PointerType::getUnqual(PointerType::getUnqual(Type::getInt8Ty(context)));
                        } else {
                            paramAllocType = PointerType::getUnqual(Type::getInt8Ty(context));
                        }
                    } else {
                        paramAllocType = typeHelper.getLLVMType(vt);
                    }
                } else {
                    paramAllocType = arg.getType();
                }
                
                AllocaInst* alloc = builder.CreateAlloca(paramAllocType, nullptr, paramNames[idx]);
                builder.CreateStore(&arg, alloc);
                scopeManager.defineVariable(paramNames[idx], alloc);
                if (idx < paramValueTypes.size() && paramValueTypes[idx] == ValueType::STRING) {
                    typeHelper.registerStringVariable(paramNames[idx]);
                }
            }
            ++idx;
        }
        
        ASTNode* body = node->data.function.body;
        if (body) {
            if (body->type == AST_PROGRAM) {
                for (int i = 0; i < body->data.program.statement_count; i++) {
                    visit(body->data.program.statements[i]);
                }
            } else {
                visit(body);
            }
        }
        
        scopeManager.exitScope();
        BasicBlock* endBB = BasicBlock::Create(context, "func_end", func);
        std::vector<BasicBlock*> blocks;
        for (auto& BB : *func) {
            blocks.push_back(&BB);
        }
        
        for (BasicBlock* BB : blocks) {
            if (BB == endBB) continue;
            if (!BB->getTerminator()) {
                IRBuilder<> tmpBuilder(BB);
                tmpBuilder.CreateBr(endBB);
            }
        }
        builder.SetInsertPoint(endBB);
        if (returnType->isVoidTy()) {
            builder.CreateRetVoid();
        } else {
            Value* defaultRetVal = nullptr;
            if (returnType->isIntegerTy()) {
                defaultRetVal = ConstantInt::get(returnType, 0);
            } else if (returnType->isFloatTy()) {
                defaultRetVal = ConstantFP::get(returnType, 0.0);
            } else if (returnType->isDoubleTy()) {
                defaultRetVal = ConstantFP::get(returnType, 0.0);
            } else if (returnType->isPointerTy()) {
                defaultRetVal = ConstantPointerNull::get(cast<PointerType>(returnType));
            } else {
                defaultRetVal = ConstantInt::get(returnType, 0);
            }
            builder.CreateRet(defaultRetVal);
        }
        scopeManager.setCurrentFunction(prevFunc);
        builder.ClearInsertionPoint();/*清除插入点，
        以便后续的顶级代码生成不会继续在刚刚完成的函数的基本块内进行
         该基本块可能已经包含返回指令 */
        return VisitResult(func, returnValueType);
    }
    
    VisitResult visitCall(ASTNode* node) {
        if (!node->data.call.func) return VisitResult();
        if (node->data.call.func->type != AST_IDENTIFIER) return VisitResult();
        
        std::string calleeName(node->data.call.func->data.identifier.name);
        Function* callee = module->getFunction(calleeName);
        if (!callee) {
            llvm::errs() << "Error: Call to undefined function '" << calleeName << "'\n";
            return VisitResult();
        }
        
        int expectedParamCount = callee->getFunctionType()->getNumParams();
        int actualParamCount = node->data.call.args ? 
            node->data.call.args->data.expression_list.expression_count : 0;
        bool isVarArg = callee->getFunctionType()->isVarArg();
        bool isKnownVarArgFunc = (calleeName == "printf" || calleeName == "sprintf" ||
                                  calleeName == "snprintf" || calleeName == "scanf");
        
        if (!isVarArg && !isKnownVarArgFunc && actualParamCount != expectedParamCount) {
            llvm::errs() << "Error: Function '" << calleeName 
                        << "' expects " << expectedParamCount 
                        << " arguments but got " << actualParamCount << "\n";
            return VisitResult();
        }
        
        std::vector<Value*> args;
        if (node->data.call.args) {
            for (int i = 0; i < actualParamCount; i++) {
                ASTNode* argNode = node->data.call.args->data.expression_list.expressions[i];
                VisitResult argRes = visit(argNode);
                if (!argRes.value) {
                    llvm::errs() << "Error: Failed to evaluate argument " << i 
                                << " for function '" << calleeName << "'\n";
                    return VisitResult();
                }
                if (i < expectedParamCount || isVarArg || isKnownVarArgFunc) {
                    Type* expectedType = nullptr;
                    if (i < expectedParamCount) {
                        expectedType = callee->getFunctionType()->getParamType(i);
                    } else {
                        if (isKnownVarArgFunc) {
                            if (argRes.value->getType()->isIntegerTy() && 
                                argRes.value->getType()->getIntegerBitWidth() < 32) {
                                expectedType = Type::getInt32Ty(context);//提升到i32
                            } else {
                                expectedType = argRes.value->getType();
                            }
                        } else {
                            expectedType = argRes.value->getType();
                        }
                    }
                    
                    ValueType expectedValueType = typeHelper.getValueTypeFromType(expectedType);
                    
                    Value* arg = typeHelper.castValue(builder, argRes.value, argRes.type, expectedValueType);
                    args.push_back(arg);
                }
            }
        }
        
        CallInst* callInst = builder.CreateCall(callee, args);
        if (!callee->getReturnType()->isVoidTy()) callInst->setName("calltmp");
        ValueType returnType = typeHelper.getValueTypeFromType(callee->getReturnType());
        return VisitResult(callInst, returnType);
    }
    
    VisitResult visitReturn(ASTNode* node) {
        Function* currentFunc = getCurrentFunction();
        if (!currentFunc) return VisitResult();
        
        Type* expectedReturnType = currentFunc->getReturnType();
        
        if (node->data.return_stmt.expr) {
            VisitResult retVal = visit(node->data.return_stmt.expr);
            if (!retVal.value) return VisitResult();
            
            ValueType expectedValueType = typeHelper.getValueTypeFromType(expectedReturnType);
            Value* retValue = typeHelper.castValue(builder, retVal.value, retVal.type, expectedValueType);
            builder.CreateRet(retValue);
            return VisitResult(retValue, expectedValueType);
        }
        
        if (expectedReturnType->isVoidTy()) {
            builder.CreateRetVoid();
        } else {
            builder.CreateRet(ConstantInt::get(expectedReturnType, 0));
        }
        return VisitResult(nullptr, ValueType::VOID);
    }
    
    VisitResult visitStructDef(ASTNode* node) {
        std::string structName(node->data.struct_def.name);
        if (typeHelper.getStructType(structName)) return VisitResult();
        
        ASTNode* fields = node->data.struct_def.fields;
        if (!fields || fields->type != AST_EXPRESSION_LIST) return VisitResult();
        
        std::vector<Type*> fieldTypes;
        std::vector<std::pair<std::string, Type*>> fieldInfo;
        StructType* structType = StructType::create(context, structName);
        typeHelper.registerStructType(structName, structType, {});

        int fieldCount = fields->data.expression_list.expression_count;
        for (int i = 0; i < fieldCount; i++) {
            ASTNode* field = fields->data.expression_list.expressions[i];
            
            if (field->type == AST_ASSIGN) {
                ASTNode* left = field->data.assign.left;
                ASTNode* right = field->data.assign.right;
                
                if (left && left->type == AST_IDENTIFIER) {
                    std::string fieldName(left->data.identifier.name);
                    Type* fieldType = typeHelper.getTypeFromTypeNode(right);
                    if (!fieldType) fieldType = Type::getInt32Ty(context);
                    fieldTypes.push_back(fieldType);
                    fieldInfo.push_back({fieldName, fieldType});
                }
            }
            else if (field->type == AST_TYPE_INT32 || field->type == AST_TYPE_INT64 ||
                     field->type == AST_TYPE_FLOAT32 || field->type == AST_TYPE_FLOAT64 ||
                     field->type == AST_TYPE_STRING) {
                char defaultName[32];
                snprintf(defaultName, sizeof(defaultName), "field%d", i);
                std::string fieldName(defaultName);
                Type* fieldType = typeHelper.getTypeFromTypeNode(field);
                fieldTypes.push_back(fieldType);
                fieldInfo.push_back({fieldName, fieldType});
            }
        }
        
        if (fieldTypes.empty()) return VisitResult();
        structType->setBody(fieldTypes, false);
        typeHelper.registerStructType(structName, structType, fieldInfo);
        return VisitResult(nullptr, ValueType::VOID);
    }
    
    VisitResult visitStructLiteral(ASTNode* node) {
        ASTNode* typeName = node->data.struct_literal.type_name;
        if (!typeName || typeName->type != AST_IDENTIFIER) return VisitResult();
        
        std::string structName(typeName->data.identifier.name);
        StructType* structType = typeHelper.getStructType(structName);
        if (!structType) return VisitResult();
        
        Function* func = getCurrentFunction();
        if (!func) {
            func = module->getFunction("main");
            if (!func) {
                createDefaultMain();
                func = module->getFunction("main");
            }
        }
        
        if (!func) return VisitResult();
        BasicBlock* entryBB = &func->getEntryBlock();
        BasicBlock* savedBB = builder.GetInsertBlock();
        
        IRBuilder<> tempBuilder(entryBB, entryBB->begin());
        AllocaInst* structAlloc = tempBuilder.CreateAlloca(structType, nullptr, "tmp_struct");
        
        if (savedBB) {
            builder.SetInsertPoint(savedBB);
        }
        
        initStructLiteral(structAlloc, node);
        
        return VisitResult(structAlloc, ValueType::POINTER, structType);
    }
    
    VisitResult handleArrayLength(ASTNode* object) {
        
        if (object->type == AST_IDENTIFIER) {
            std::string varName(object->data.identifier.name);
            llvm::errs() << "[DEBUG] to geet length of variable: " << varName << "\n";
            
            AllocaInst* alloc = scopeManager.findVariable(varName);
            llvm::errs() << "[DEBUG] found in scopeMar: " << (alloc ? "yes" : "no") << "\n";
            
            if (!alloc) {
                alloc = findVariableInMain(varName);
                llvm::errs() << "[DEBUG] found in main: " << (alloc ? "yes" : "no") << "\n";
            }
            if (alloc) {
                Type* allocatedType = getActualType(alloc);
                llvm::errs() << "[DEBUG] type: " << *allocatedType << "\n";
                if (allocatedType && allocatedType->isArrayTy()) {
                    ArrayType* arrayType = cast<ArrayType>(allocatedType);
                    uint64_t numElements = arrayType->getNumElements();
                    Value* length = ConstantInt::get(Type::getInt32Ty(context), numElements);
                    llvm::errs() << "[DEBUG] Arr length (s): " << numElements << "\n";
                    return VisitResult(length, ValueType::INT32);
                }
                if (allocatedType && allocatedType->isPointerTy()) {
                    auto* arrayInfo = typeHelper.getArrayTypeInfo(varName);
                    if (arrayInfo) {
                        int elementCount = arrayInfo->second;
                        if (elementCount > 0) {
                            Value* length = ConstantInt::get(Type::getInt32Ty(context), elementCount);
                            llvm::errs() << "[DEBUG] Array length (r): " << elementCount << "\n";
                            return VisitResult(length, ValueType::INT32);
                        }
                    }
                    if (typeHelper.isStringVariable(varName)) {
                        Value* strPtr = builder.CreateLoad(allocatedType, alloc, varName);
                        CallInst* strlenCall = builder.CreateCall(strlenFunction, {strPtr}, "strlen");
                        Value* length = builder.CreateIntCast(strlenCall, Type::getInt32Ty(context), false, "len");
                        llvm::errs() << "[DEBUG] String length (strlen): dynamic\n";
                        return VisitResult(length, ValueType::INT32);
                    }
                    llvm::errs() << "[DEBUG] Unknown pointer type, returning 0\n";
                    Value* length = ConstantInt::get(Type::getInt32Ty(context), 0);
                    return VisitResult(length, ValueType::INT32);
                }
            }
            auto* arrayInfo = typeHelper.getArrayTypeInfo(varName);
            if (arrayInfo) {
                int elementCount = arrayInfo->second;
                Value* length = ConstantInt::get(Type::getInt32Ty(context), elementCount);
                llvm::errs() << "[DEBUG] Array length (type info): " << elementCount << "\n";
                return VisitResult(length, ValueType::INT32);
            }
            if (typeHelper.isStringVariable(varName)) {
                AllocaInst* varAlloc = scopeManager.findVariable(varName);
                if (!varAlloc) varAlloc = findVariableInMain(varName);
                
                if (varAlloc) {
                    Type* allocatedType = getActualType(varAlloc);
                    Value* strPtr = builder.CreateLoad(allocatedType, varAlloc, varName);
                    CallInst* strlenCall = builder.CreateCall(strlenFunction, {strPtr}, "strlen");
                    Value* length = builder.CreateIntCast(strlenCall, Type::getInt32Ty(context), false, "len");
                    llvm::errs() << "[DEBUG] String length (strlen from isStringVariable): dynamic\n";
                    return VisitResult(length, ValueType::INT32);
                }
            }
        }
        if (object->type == AST_EXPRESSION_LIST) {
            int count = object->data.expression_list.expression_count;
            Value* length = ConstantInt::get(Type::getInt32Ty(context), count);
            llvm::errs() << "[DEBUG] Literal length: " << count << "\n";
            return VisitResult(length, ValueType::INT32);
        }
        
        llvm::errs() << "[WARNING] Could not determine length, returning 0\n";
        Value* length = ConstantInt::get(Type::getInt32Ty(context), 0);
        return VisitResult(length, ValueType::INT32);
    }
    
    VisitResult visitMemberAccess(ASTNode* node) {
        ASTNode* object = node->data.member_access.object;
        ASTNode* field = node->data.member_access.field;
        if (!object || !field) return VisitResult();
        
        if (field->type == AST_IDENTIFIER) {
            std::string fieldName(field->data.identifier.name);
            if (fieldName == "length" || fieldName == "size") {
                return handleArrayLength(object);
            }
        }
        
        VisitResult objectRes = visit(object);
        if (!objectRes.value) return VisitResult();
        
        if (!objectRes.value->getType()->isPointerTy()) {
            llvm::errs() << "Error: Object is not a pointer\n";
            return VisitResult(ConstantInt::get(Type::getInt32Ty(context), 0), ValueType::INT32);
        }
        
        if (field->type != AST_IDENTIFIER) return VisitResult();
        
        std::string fieldName(field->data.identifier.name);
        
        StructType* structType = nullptr;
        Value* basePtr = nullptr;
        
        if (objectRes.structType) {
            structType = objectRes.structType;
            basePtr = objectRes.value;
        } else if (AllocaInst* alloc = dyn_cast<AllocaInst>(objectRes.value)) {
            Type* allocatedType = getActualType(alloc);
            if (allocatedType->isStructTy()) {
                structType = cast<StructType>(allocatedType);
                basePtr = alloc;
            }
        } else if (LoadInst* load = dyn_cast<LoadInst>(objectRes.value)) {
            Value* ptr = load->getPointerOperand();
            if (ptr->getType()->isPointerTy()) {
                if (AllocaInst* allocPtr = dyn_cast<AllocaInst>(ptr)) {
                    Type* allocatedType = getActualType(allocPtr);
                    if (allocatedType->isStructTy()) {
                        structType = cast<StructType>(allocatedType);
                        basePtr = ptr;
                    }
                }
            }
        }
        
        if (!structType || !basePtr) {
            llvm::errs() << "Error: Cannot access member '" << fieldName 
                        << "' - not a struct type\n";
            return VisitResult(ConstantInt::get(Type::getInt32Ty(context), 0), ValueType::INT32);
        }
        
        std::string structName = structType->getName().str();
        int idx = typeHelper.getFieldIndex(structName, fieldName);
        if (idx < 0) {
            llvm::errs() << "Error: Struct '" << structName 
                        << "' has no member named '" << fieldName << "'\n";
            return VisitResult(ConstantInt::get(Type::getInt32Ty(context), 0), ValueType::INT32);
        }
        
        Value* fieldPtr = builder.CreateStructGEP(structType, basePtr, idx, fieldName);

        Type* fieldType = structType->getElementType(idx);
        if (fieldType->isStructTy()) {
            StructType* nested = cast<StructType>(fieldType);
            return VisitResult(fieldPtr, ValueType::POINTER, nested);
        }

        Value* fieldVal = builder.CreateLoad(fieldType, fieldPtr, fieldName);
        ValueType resultType = typeHelper.getValueTypeFromType(fieldType);

        return VisitResult(fieldVal, resultType, structType);
    }
    
    VisitResult visitPrint(ASTNode* node) {
        if (!node || !node->data.print.expr) return VisitResult();
        
        if (!ensureValidInsertPoint()) {
            llvm::errs() << "Error: Cannot find valid insertion point for print\n";
            return VisitResult();
        }
        
        initPrintf();
        
        if (node->data.print.expr->type == AST_EXPRESSION_LIST) {
            ASTNode* list = node->data.print.expr;
            int exprCount = list->data.expression_list.expression_count;
            
            for (int i = 0; i < exprCount; i++) {
                ASTNode* expr = list->data.expression_list.expressions[i];
                VisitResult exprRes = visit(expr);
                
                if (!exprRes.value) {
                    Value* emptyStr = safeCreateGlobalStringPtr("", "empty_str");
                    Value* formatStr = safeCreateGlobalStringPtr("%s", "fmt_s");
                    builder.CreateCall(printfFunction, {formatStr, emptyStr});
                    continue;
                }
                
                Value* printValue = exprRes.value;
                ValueType printType = exprRes.type;
                Value* formatStr = nullptr;
                
                switch (printType) {
                    case ValueType::INT32:
                        formatStr = safeCreateGlobalStringPtr("%d", "fmt_i32");
                        builder.CreateCall(printfFunction, {formatStr, printValue});
                        break;
                        
                    case ValueType::INT8:
                        formatStr = safeCreateGlobalStringPtr("%c", "fmt_c");
                        builder.CreateCall(printfFunction, {formatStr, printValue});
                        break;
                        
                    case ValueType::INT64:
                        formatStr = safeCreateGlobalStringPtr("%lld", "fmt_i64");
                        builder.CreateCall(printfFunction, {formatStr, printValue});
                        break;
                        
                    case ValueType::FLOAT32:
                    case ValueType::FLOAT64:
                        formatStr = safeCreateGlobalStringPtr("%f", "fmt_f");
                        printValue = typeHelper.castValue(builder, printValue, printType, ValueType::FLOAT64);
                        builder.CreateCall(printfFunction, {formatStr, printValue});
                        break;
                        
                    case ValueType::STRING:
                        formatStr = safeCreateGlobalStringPtr("%s", "fmt_s");
                        builder.CreateCall(printfFunction, {formatStr, printValue});
                        break;
                        
                    case ValueType::BOOL:
                        formatStr = safeCreateGlobalStringPtr("%d", "fmt_b");
                        printValue = typeHelper.castValue(builder, printValue, printType, ValueType::INT32);
                        builder.CreateCall(printfFunction, {formatStr, printValue});
                        break;
                        
                    case ValueType::POINTER:
                        formatStr = safeCreateGlobalStringPtr("%p", "fmt_p");
                        builder.CreateCall(printfFunction, {formatStr, printValue});
                        break;
                        
                    case ValueType::ARRAY:
                        formatStr = safeCreateGlobalStringPtr("%p", "fmt_p");
                        builder.CreateCall(printfFunction, {formatStr, printValue});
                        break;
                        
                    default:
                        formatStr = safeCreateGlobalStringPtr("%d", "fmt_d");
                        if (printValue->getType()->isIntegerTy()) {
                            builder.CreateCall(printfFunction, {formatStr, printValue});
                        } else {
                            Value* defaultVal = ConstantInt::get(Type::getInt32Ty(context), 0);
                            builder.CreateCall(printfFunction, {formatStr, defaultVal});
                        }
                        break;
                }
            }
            
            Value* newline = safeCreateGlobalStringPtr("\n", "fmt_nl");
            if (newline) {
                builder.CreateCall(printfFunction, {newline});
            }
            
            return VisitResult(nullptr, ValueType::VOID);
        } else {
            VisitResult expr = visit(node->data.print.expr);
            
            if (!expr.value) {
                return VisitResult();
            }
            
            Value* printValue = expr.value;
            ValueType printType = expr.type;
            Value* formatStr = nullptr;
            
            switch (printType) {
                case ValueType::INT32:
                    formatStr = safeCreateGlobalStringPtr("%d\n", "fmt_i32_nl");
                    builder.CreateCall(printfFunction, {formatStr, printValue});
                    break;
                    
                case ValueType::INT8:
                    formatStr = safeCreateGlobalStringPtr("%c\n", "fmt_c_nl");
                    builder.CreateCall(printfFunction, {formatStr, printValue});
                    break;
                    
                case ValueType::INT64:
                    formatStr = safeCreateGlobalStringPtr("%lld\n", "fmt_i64_nl");
                    builder.CreateCall(printfFunction, {formatStr, printValue});
                    break;
                    
                case ValueType::FLOAT32:
                case ValueType::FLOAT64:
                    formatStr = safeCreateGlobalStringPtr("%f\n", "fmt_f_nl");
                    printValue = typeHelper.castValue(builder, printValue, printType, ValueType::FLOAT64);
                    builder.CreateCall(printfFunction, {formatStr, printValue});
                    break;
                    
                case ValueType::STRING:
                    formatStr = safeCreateGlobalStringPtr("%s\n", "fmt_s_nl");
                    builder.CreateCall(printfFunction, {formatStr, printValue});
                    break;
                    
                case ValueType::BOOL:
                    formatStr = safeCreateGlobalStringPtr("%d\n", "fmt_b_nl");
                    printValue = typeHelper.castValue(builder, printValue, printType, ValueType::INT32);
                    builder.CreateCall(printfFunction, {formatStr, printValue});
                    break;
                    
                case ValueType::POINTER:
                    formatStr = safeCreateGlobalStringPtr("%p\n", "fmt_p_nl");
                    builder.CreateCall(printfFunction, {formatStr, printValue});
                    break;
                    
                case ValueType::ARRAY:
                    formatStr = safeCreateGlobalStringPtr("%p\n", "fmt_p_nl");
                    builder.CreateCall(printfFunction, {formatStr, printValue});
                    break;
                    
                default:
                    formatStr = safeCreateGlobalStringPtr("%d\n", "fmt_d_nl");
                    if (printValue->getType()->isIntegerTy()) {
                        builder.CreateCall(printfFunction, {formatStr, printValue});
                    } else {
                        Value* defaultVal = ConstantInt::get(Type::getInt32Ty(context), 0);
                        builder.CreateCall(printfFunction, {formatStr, defaultVal});
                    }
                    break;
            }
            
            return VisitResult(printValue, printType);
        }
    }
    
    VisitResult visitInput(ASTNode* node) {
        (void)node;
        return VisitResult(ConstantInt::get(Type::getInt32Ty(context), 0), ValueType::INT32);
    }
    
    VisitResult visitToInt(ASTNode* node) {
        if (!node->data.toint.expr) return VisitResult();
        VisitResult expr = visit(node->data.toint.expr);
        if (!expr.value) return VisitResult();
        Value* val = typeHelper.castValue(builder, expr.value, expr.type, ValueType::INT32);
        return VisitResult(val, ValueType::INT32);
    }
    
    VisitResult visitToFloat(ASTNode* node) {
        if (!node->data.tofloat.expr) return VisitResult();
        VisitResult expr = visit(node->data.tofloat.expr);
        if (!expr.value) return VisitResult();
        Value* val = typeHelper.castValue(builder, expr.value, expr.type, ValueType::FLOAT64);
        return VisitResult(val, ValueType::FLOAT64);
    }

    VisitResult visitArrayLiteral(ASTNode* node) {
        if (!node || node->type != AST_EXPRESSION_LIST) return VisitResult();
        int count = node->data.expression_list.expression_count;
        if (count == 0) {
            return VisitResult(ConstantPointerNull::get(cast<PointerType>(typeHelper.getLLVMType(ValueType::POINTER))), ValueType::ARRAY);
        }

        std::vector<Value*> elems;
        elems.reserve(count);
        Type* elemType = nullptr;
        ValueType elemValueType = ValueType::INT32;

        for (int i = 0; i < count; i++) {
            ASTNode* e = node->data.expression_list.expressions[i];
            VisitResult r = visit(e);
            if (!r.value) return VisitResult();
            if (i == 0) {
                elemType = r.value->getType();
                elemValueType = typeHelper.getValueTypeFromType(elemType);
            }
            Value* v = r.value;
            if (typeHelper.getValueTypeFromType(v->getType()) != elemValueType) {
                v = typeHelper.castValue(builder, v, r.type, elemValueType);
            }
            elems.push_back(v);
        }

        if (!elemType) return VisitResult();

        Function* func = getCurrentFunction();
        if (!func) {
            createDefaultMain();
            func = module->getFunction("main");
            if (!func) return VisitResult();
        }
        BasicBlock* entryBB = &func->getEntryBlock();
        BasicBlock* savedBB = builder.GetInsertBlock();

        ArrayType* arrTy = ArrayType::get(elemType, count);
        IRBuilder<> tempBuilder(entryBB, entryBB->begin());
        AllocaInst* alloc = tempBuilder.CreateAlloca(arrTy, nullptr, "arrtmp");

        if (savedBB) {
            builder.SetInsertPoint(savedBB);
        }

        std::string arrayTypeName = "array_tmp_" + std::to_string((uintptr_t)alloc);
        typeHelper.registerArrayType(arrayTypeName, elemType, count);

        for (int i = 0; i < count; i++) {
            Value* idx0 = ConstantInt::get(Type::getInt32Ty(context), 0);
            Value* idx1 = ConstantInt::get(Type::getInt32Ty(context), i);
            Value* gep = builder.CreateInBoundsGEP(arrTy, alloc, {idx0, idx1}, "elem_ptr");
            builder.CreateStore(elems[i], gep);
        }

        Value* zero = ConstantInt::get(Type::getInt32Ty(context), 0);
        Value* gep = builder.CreateInBoundsGEP(arrTy, alloc, {zero, zero}, "arr_ptr");
        
        return VisitResult(gep, ValueType::ARRAY);
    }

    VisitResult visitGlobal(ASTNode* node) {
        if (!node) return VisitResult();
        
        ASTNode* identifier = node->data.global_decl.identifier;
        ASTNode* typeNode = node->data.global_decl.type;
        ASTNode* initializer = node->data.global_decl.initializer;
        
        if (!identifier || identifier->type != AST_IDENTIFIER) {
            llvm::errs() << "Error: Global declaration must have an identifier\n";
            return VisitResult();
        }
        
        std::string varName(identifier->data.identifier.name);
        
        Type* globalType = nullptr;
        ValueType valueType = ValueType::INT32;
        
        if (typeNode) {
            valueType = typeHelper.fromTypeNode(typeNode);
            globalType = typeHelper.getLLVMType(valueType);
        } else if (initializer) {
            VisitResult initResult = visit(initializer);
            if (initResult.value) {
                globalType = initResult.value->getType();
                valueType = initResult.type;
            }
        }
        
        if (!globalType) {
            globalType = Type::getInt32Ty(context);
            valueType = ValueType::INT32;
        }
        
        Constant* initValue = nullptr;
        if (initializer) {
            VisitResult initResult = visit(initializer);
            if (initResult.value && isa<Constant>(initResult.value)) {
                initValue = cast<Constant>(initResult.value);
            }
        }
        
        if (!initValue) {
            initValue = Constant::getNullValue(globalType);
        }
        
        GlobalVariable* globalVar = new GlobalVariable(
            *module,
            globalType,
            false,
            GlobalValue::ExternalLinkage,
            initValue,
            varName
        );
        
        return VisitResult(globalVar, valueType);
    }

    VisitResult visitIndex(ASTNode* node) {
        if (!node || !node->data.index.target || !node->data.index.index) return VisitResult();
        ASTNode* target = node->data.index.target;
        ASTNode* indexNode = node->data.index.index;
        
        
        if (target->type == AST_IDENTIFIER) {//处理字符串索引s[i]
            std::string varName(target->data.identifier.name);
            
            AllocaInst* alloc = scopeManager.findVariable(varName);
            if (!alloc) alloc = findVariableInMain(varName);
            
            if (alloc) {
                Type* allocatedType = getActualType(alloc);
                bool isStringType = false;
                auto* arrayInfo = typeHelper.getArrayTypeInfo(varName);
                if (arrayInfo && arrayInfo->first->isIntegerTy(8)) {
                    isStringType = true;
                }
                
                if (typeHelper.isStringVariable(varName)) {
                    isStringType = true;
                }
                
                if (allocatedType->isArrayTy()) {
                    ArrayType* arrType = cast<ArrayType>(allocatedType);
                    if (arrType->getElementType()->isIntegerTy(8)) {
                        isStringType = true;
                    }
                }
                if (allocatedType->isPointerTy() && typeHelper.isStringVariable(varName)) {
                    isStringType = true;
                }
                if (isStringType) {
                    VisitResult idxRes = visit(indexNode);
                    if (!idxRes.value) return VisitResult();
                    
                    Value* idxVal = idxRes.value;
                    if (!idxVal->getType()->isIntegerTy(32)) {
                        idxVal = builder.CreateIntCast(idxVal, Type::getInt32Ty(context), true, "idxcast");
                    }
                    
                    Value* charPtr = nullptr;
                    
                    if (allocatedType->isArrayTy()) {
                        Value* zero = ConstantInt::get(Type::getInt32Ty(context), 0);
                        charPtr = builder.CreateInBoundsGEP(
                            allocatedType, alloc, {zero, idxVal}, "char_ptr");
                    } else {
                        Value* strPtr = builder.CreateLoad(allocatedType, alloc, varName);
                        Type* pointeeType = nullptr;
                        if (allocatedType->isPointerTy()) {
                            pointeeType = getPointerElementTypeSafely(dyn_cast<PointerType>(allocatedType), varName);
                        } else {
                            pointeeType = allocatedType;
                        }
                        
                        charPtr = builder.CreateInBoundsGEP(
                            pointeeType, strPtr, idxVal, "char_ptr");
                    }
                    
                    Value* charVal = builder.CreateLoad(Type::getInt8Ty(context), charPtr, "char");
                    
                    llvm::errs() << "[DEBUG] string index: " << varName << "[i] = char (i8)\n";
                    
                    return VisitResult(charVal, ValueType::INT8);
                }
            }
        }
        AllocaInst* baseAlloc = nullptr;
        std::string varName;
        if (target->type == AST_IDENTIFIER) {
            varName = std::string(target->data.identifier.name);
            baseAlloc = scopeManager.findVariable(varName);
            if (!baseAlloc) baseAlloc = findVariableInMain(varName);
        }

        VisitResult idxRes = visit(indexNode);
        if (!idxRes.value) return VisitResult();
        Value* idxVal = idxRes.value;
        if (!idxVal->getType()->isIntegerTy(32)) {
            idxVal = builder.CreateIntCast(idxVal, Type::getInt32Ty(context), true, "idxcast");
        }

        if (baseAlloc) {
            Type* allocatedType = getActualType(baseAlloc);
            if (allocatedType && allocatedType->isArrayTy()) {
                ArrayType* at = cast<ArrayType>(allocatedType);
                Type* elemType = at->getElementType();
                Value* gep = builder.CreateInBoundsGEP(allocatedType, baseAlloc, 
                    {ConstantInt::get(Type::getInt32Ty(context),0), idxVal}, "arr_index_ptr");
                Value* loaded = builder.CreateLoad(elemType, gep, "arr_index_load");
                ValueType vt = typeHelper.getValueTypeFromType(elemType);
                return VisitResult(loaded, vt);
            }
            
            if (allocatedType && allocatedType->isPointerTy()) {
                Value* arrayPtr = builder.CreateLoad(allocatedType, baseAlloc, "array_ptr");
                Type* elemType = getPointerElementTypeSafely(dyn_cast<PointerType>(allocatedType), varName);
                if (varName == "argv") {//处理argv特殊情况
                    elemType = PointerType::getUnqual(Type::getInt8Ty(context));
                }

                Value* gep = builder.CreateInBoundsGEP(elemType, arrayPtr, idxVal, "ptr_index_ptr");
                Value* loaded = builder.CreateLoad(elemType, gep, "ptr_index_load");
                ValueType vt = typeHelper.getValueTypeFromType(elemType);
                return VisitResult(loaded, vt);
            }
        }

        VisitResult targetRes = visit(target);
        if (!targetRes.value) return VisitResult();

        if (AllocaInst* alloc = dyn_cast<AllocaInst>(targetRes.value)) {
            Type* allocatedType = getActualType(alloc);
            if (allocatedType && allocatedType->isArrayTy()) {
                ArrayType* at = cast<ArrayType>(allocatedType);
                Type* elemType = at->getElementType();
                Value* gep = builder.CreateInBoundsGEP(allocatedType, alloc, 
                    {ConstantInt::get(Type::getInt32Ty(context),0), idxVal}, "arr_index_ptr2");
                Value* loaded = builder.CreateLoad(elemType, gep, "arr_index_load2");
                ValueType vt = typeHelper.getValueTypeFromType(elemType);
                return VisitResult(loaded, vt);
            }
        }

        if (targetRes.value->getType()->isPointerTy()) {
            if (target->type == AST_IDENTIFIER) {
                varName = std::string(target->data.identifier.name);
            }
            Type* elemType = getPointerElementTypeSafely(dyn_cast<PointerType>(targetRes.value->getType()), varName);

            Value* gep = builder.CreateInBoundsGEP(elemType, targetRes.value, idxVal, "arr_index_ptr3");
            Value* loaded = builder.CreateLoad(elemType, gep, "arr_index_load3");
            ValueType vt = typeHelper.getValueTypeFromType(elemType);
            return VisitResult(loaded, vt);
        }

        return VisitResult();
    }
};

// ==================== C API ====================
void llvm_emit_from_ast(ASTNode* ast_root, FILE* llvm_fp) {
    if (!ast_root || !llvm_fp) return;
    
    LLVMCodeGenerator generator;
    std::unique_ptr<Module> module = generator.generate(ast_root);
    
    if (module) {
        std::string llvm_ir;
        raw_string_ostream ros(llvm_ir);
        module->print(ros, nullptr);
        fprintf(llvm_fp, "%s", llvm_ir.c_str());
    }
}