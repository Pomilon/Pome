#include "../include/pome_interpreter.h"
#include "../include/pome_lexer.h"
#include "../include/pome_parser.h"
#include "../include/pome_errors.h"
#include "../include/pome_stdlib.h"
#include <cmath>
#include <string>
#include <limits>
#include <vector>
#include <filesystem>
#include <cstdlib> // For getenv, rand
#include <sstream> // For stringstream
#include <ctime>   // For time

namespace Pome
{

    /**
     * Custom exception for handling return statements
     */
    class ReturnException : public std::runtime_error
    {
    public:
        PomeValue value;
        explicit ReturnException(const PomeValue &val) : std::runtime_error("Return statement"), value(val) {}
    };

    /**
     * Native print function for the Pome language
     */
    PomeValue nativePrint(const std::vector<PomeValue> &args)
    {
        for (size_t i = 0; i < args.size(); ++i)
        {
            std::cout << args[i].toString();
            if (i < args.size() - 1)
            {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
        return PomeValue(std::monostate{}); // Print returns nil
    }

    /**
     * Native len function: returns the length of a string, list, or table
     */
    PomeValue nativeLen(const std::vector<PomeValue> &args)
    {
        if (args.size() != 1)
        {
            throw std::runtime_error("len() expects 1 argument.");
        }
        if (args[0].isString())
        {
            return PomeValue(static_cast<double>(args[0].asString().length()));
        }
        else if (args[0].isList())
        {
            return PomeValue(static_cast<double>(args[0].asList()->elements.size()));
        }
        else if (args[0].isTable())
        {
            return PomeValue(static_cast<double>(args[0].asTable()->elements.size()));
        }
        throw std::runtime_error("len() expects a string, list, or table argument.");
    }

    PomeValue nativeToNumber(const std::vector<PomeValue> &args)
    {
        if (args.size() != 1)
        {
            throw std::runtime_error("tonumber() expects 1 argument.");
        }
        if (!args[0].isString())
        {
            return PomeValue(std::monostate{}); 
        }

        try
        {
            size_t pos;
            double d = std::stod(args[0].asString(), &pos);
            if (pos == args[0].asString().length())
            { 
                return PomeValue(d);
            }
            else
            {
                return PomeValue(std::monostate{}); 
            }
        }
        catch (const std::out_of_range &oor)
        {
            return PomeValue(std::monostate{}); 
        }
        catch (const std::invalid_argument &ia)
        {
            return PomeValue(std::monostate{}); 
        }
    }

    Interpreter::Interpreter() : importer_(*this)
    {
        gc_.setInterpreter(this);
        /**
         * Allocate Environment via GC
         */
        currentEnvironment_ = gc_.allocate<Environment>(nullptr); 
        globalEnvironment_ = currentEnvironment_; // Set global environment
        
        setupGlobalEnvironment();
        
        importer_.addSearchPath("examples/");

        /**
         * Allocate initial export module
         */
        PomeModule* rootModule = gc_.allocate<PomeModule>();
        exportStack_.push_back(rootModule);

        /**
         * Seed random number generator
         */
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    void Interpreter::setupGlobalEnvironment()
    {
        auto printFn = gc_.allocate<NativeFunction>("print", nativePrint);
        currentEnvironment_->define("print", PomeValue(printFn));
        
        auto lenFn = gc_.allocate<NativeFunction>("len", nativeLen);
        currentEnvironment_->define("len", PomeValue(lenFn));
        
        auto toNumFn = gc_.allocate<NativeFunction>("tonumber", nativeToNumber);
        currentEnvironment_->define("tonumber", PomeValue(toNumFn));

        auto gcCountFn = gc_.allocate<NativeFunction>("gc_count", [this](const std::vector<PomeValue>& args) {
            return PomeValue((double)gc_.getObjectCount());
        });
        currentEnvironment_->define("gc_count", PomeValue(gcCountFn));

        auto gcCollectFn = gc_.allocate<NativeFunction>("gc_collect", [this](const std::vector<PomeValue>& args) {
            gc_.collect();
            return PomeValue(std::monostate{});
        });
        currentEnvironment_->define("gc_collect", PomeValue(gcCollectFn));

        /**
         * Add global constants
         */
        currentEnvironment_->define("PI", PomeValue(3.141592653589793));
    }
    
    void Interpreter::markRoots() {
        /**
         * Mark current and global environment
         */
        gc_.markObject(currentEnvironment_);
        gc_.markObject(globalEnvironment_);
        
        /**
         * Mark export stack
         */
        for (auto* mod : exportStack_) {
            gc_.markObject(mod);
        }
        
        /**
         * Mark executed modules
         */
        for (auto& pair : executedModules_) {
            gc_.markObject(pair.second);
        }
        
        /**
         * Mark last evaluated value
         */
        if (lastEvaluatedValue_.asObject()) {
            gc_.markObject(lastEvaluatedValue_.asObject());
        }
    }

    void Interpreter::interpret(Program &program)
    {
        try
        {
            program.accept(*this);
        }
        catch (const ReturnException &e)
        {
            lastEvaluatedValue_ = e.value;
        }
        catch (const PomeException &e)
        { 
            std::cerr << e.what() << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Runtime error: " << e.what() << std::endl;
        }
    }

    /**
     * Helper to evaluate expressions
     */
    PomeValue Interpreter::evaluateExpression(Expression &expr)
    {
        expr.accept(*this);
        return lastEvaluatedValue_;
    }

    /**
     * Helper to execute statements
     */
    void Interpreter::executeStatement(Statement &stmt)
    {
        stmt.accept(*this);
    }

    /**
     * --- Visitor methods for expressions ---
     */

    void Interpreter::visit(NumberExpr &expr)
    {
        lastEvaluatedValue_ = PomeValue(expr.getValue());
    }

    void Interpreter::visit(StringExpr &expr)
    {
        /**
         * Allocate string via GC
         */
        PomeString* s = gc_.allocate<PomeString>(expr.getValue());
        lastEvaluatedValue_ = PomeValue(s);
    }

    void Interpreter::visit(BooleanExpr &expr)
    {
        lastEvaluatedValue_ = PomeValue(expr.getValue());
    }

    void Interpreter::visit(NilExpr &expr)
    {
        lastEvaluatedValue_ = PomeValue(std::monostate{});
    }

    void Interpreter::visit(IdentifierExpr &expr)
    {
        lastEvaluatedValue_ = currentEnvironment_->get(expr.getName());
    }

    void Interpreter::visit(BinaryExpr &expr)
    {
        PomeValue left = evaluateExpression(*expr.getLeft());
        /**
         * Temporarily root left while evaluating right
         */
        RootGuard guard(gc_, left.asObject());
        
        PomeValue right = evaluateExpression(*expr.getRight());
        
        try
        {
            lastEvaluatedValue_ = applyBinaryOp(left, expr.getOperator(), right);
        }
        catch (const std::runtime_error &e)
        {
            throw RuntimeError(e.what(), expr.getLine(), expr.getColumn());
        }
    }

    void Interpreter::visit(UnaryExpr &expr)
    {
        PomeValue operand = evaluateExpression(*expr.getOperand());
        try
        {
            lastEvaluatedValue_ = applyUnaryOp(expr.getOperator(), operand);
        }
        catch (const std::runtime_error &e)
        {
            throw RuntimeError(e.what(), expr.getLine(), expr.getColumn());
        }
    }

    void Interpreter::visit(CallExpr &expr)
    {
        PomeValue calleeValue;
        PomeValue thisValue;
        bool isMethodCall = false;

        if (auto memberAccess = dynamic_cast<MemberAccessExpr *>(expr.getCallee()))
        {
            PomeValue objectValue = evaluateExpression(*memberAccess->getObject());
            /**
             * Root objectValue
             */
            RootGuard objGuard(gc_, objectValue.asObject());
            
            std::string memberName = memberAccess->getMember();

            if (objectValue.isInstance())
            {
                PomeInstance* instance = objectValue.asInstance();
                PomeValue field = instance->get(memberName);
                if (!field.isNil())
                {
                    calleeValue = field;
                }
                else
                {
                    auto method = instance->klass->findMethod(memberName);
                    if (method)
                    {
                        calleeValue = PomeValue(method);
                        thisValue = objectValue; 
                        isMethodCall = true;
                    }
                    else
                    {
                        calleeValue = PomeValue(std::monostate{}); 
                    }
                }
            }
            /**
             * Fix for module access
             */
            else if (objectValue.asModule()) {
                auto module = objectValue.asModule();
                 PomeString* keyStr = gc_.allocate<PomeString>(memberName);
                 PomeValue key(keyStr);
                 
                 if (module->exports.count(key))
                    calleeValue = module->exports[key];
                 else
                    calleeValue = PomeValue(std::monostate{});
            }
            else if (objectValue.isTable())
            {
                auto table = objectValue.asTable();
                PomeString* keyStr = gc_.allocate<PomeString>(memberName);
                PomeValue key(keyStr);
                
                if (table->elements.count(key))
                    calleeValue = table->elements[key];
                else
                    calleeValue = PomeValue(std::monostate{});
            }
        }
        else
        {
            calleeValue = evaluateExpression(*expr.getCallee());
        }

        /**
         * Root callee
         */
        RootGuard calleeGuard(gc_, calleeValue.asObject());

        if (calleeValue.isClass())
        {
            PomeClass* klass = calleeValue.asClass();
            PomeInstance* instance = gc_.allocate<PomeInstance>(klass);

            auto initMethod = klass->findMethod("init");
            if (initMethod)
            {
                std::vector<PomeValue> args;
                RootGuard instanceGuard(gc_, instance);
                
                for (const auto &argExpr : expr.getArgs())
                {
                    args.push_back(evaluateExpression(*argExpr));
                }
                
                callPomeFunction(initMethod, args, instance);
            }

            lastEvaluatedValue_ = PomeValue(instance);
            return;
        }

        if (!calleeValue.isFunction())
        {
            if (!calleeValue.isNil())
                 throw RuntimeError("Attempt to call a non-function value.", expr.getLine(), expr.getColumn());
        }
        
        if (calleeValue.isNil()) {
             throw RuntimeError("Attempt to call a nil value.", expr.getLine(), expr.getColumn());
        }

        std::vector<PomeValue> args;
        std::vector<std::unique_ptr<RootGuard>> argGuards;
        
        for (const auto &argExpr : expr.getArgs())
        {
            PomeValue v = evaluateExpression(*argExpr);
            args.push_back(v);
            argGuards.push_back(std::make_unique<RootGuard>(gc_, v.asObject()));
        }

        if (calleeValue.isNativeFunction())
        {
            NativeFunction* nativeFunc = calleeValue.asNativeFunction();
            try
            {
                lastEvaluatedValue_ = nativeFunc->call(args);
            }
            catch (const std::runtime_error &e)
            {
                throw RuntimeError(e.what(), expr.getLine(), expr.getColumn());
            }
        }
        else if (calleeValue.isPomeFunction())
        {
            PomeFunction* pomeFunc = calleeValue.asPomeFunction();
            PomeInstance* thisInstance = nullptr;
            if (isMethodCall && thisValue.isInstance())
            {
                thisInstance = thisValue.asInstance();
            }

            try
            {
                lastEvaluatedValue_ = callPomeFunction(pomeFunc, args, thisInstance);
            }
            catch (const RuntimeError &e)
            {
                throw RuntimeError(e.what(), expr.getLine(), expr.getColumn());
            }
        }
    }

    PomeValue Interpreter::callPomeFunction(PomeFunction* pomeFunc, const std::vector<PomeValue> &args, PomeInstance* thisInstance)
    {
        if (args.size() != pomeFunc->parameters.size())
        {
            throw std::runtime_error("Function '" + pomeFunc->name + "' expected " +
                                     std::to_string(pomeFunc->parameters.size()) +
                                     " arguments, but got " + std::to_string(args.size()) + ".");
        }

        Environment* previousEnvironment = currentEnvironment_;
        /**
         * New environment rooted via currentEnvironment_ once assigned
         */
        currentEnvironment_ = gc_.allocate<Environment>(pomeFunc->closureEnv);

        for (size_t i = 0; i < pomeFunc->parameters.size(); ++i)
        {
            currentEnvironment_->define(pomeFunc->parameters[i], args[i]);
        }

        if (thisInstance)
        {
            currentEnvironment_->define("this", PomeValue(thisInstance));
        }

        PomeValue returnValue = PomeValue(std::monostate{});
        try
        {
            for (const auto &s : *(pomeFunc->body))
            {
                executeStatement(*s);
            }
        }
        catch (const ReturnException &e)
        {
            returnValue = e.value;
        }

        currentEnvironment_ = previousEnvironment; // Restore env
        
        return returnValue;
    }

    void Interpreter::visit(MemberAccessExpr &expr)
    {
        PomeValue objectValue = evaluateExpression(*expr.getObject());
        const std::string &memberName = expr.getMember();
        
        /**
         * Root object
         */
        RootGuard objGuard(gc_, objectValue.asObject());

        if (auto module = objectValue.asModule())
        { 
            /**
             * Create key string with GC
             */
            PomeString* keyStr = gc_.allocate<PomeString>(memberName);
            PomeValue key(keyStr);
            
            if (module->exports.count(key))
            {
                lastEvaluatedValue_ = module->exports[key];
            }
            else
            {
                throw RuntimeError("Member '" + memberName + "' not found in module.", expr.getLine(), expr.getColumn());
            }
        }
        else if (objectValue.isTable())
        {
            auto table = objectValue.asTable();
            PomeString* keyStr = gc_.allocate<PomeString>(memberName);
            PomeValue key(keyStr);
            
            if (table->elements.count(key))
            {
                lastEvaluatedValue_ = table->elements[key];
            }
            else
            {
                lastEvaluatedValue_ = PomeValue(std::monostate{}); 
            }
        }
        else if (objectValue.isInstance())
        {
            PomeInstance* instance = objectValue.asInstance();
            PomeValue field = instance->get(memberName);
            if (!field.isNil())
            {
                lastEvaluatedValue_ = field;
            }
            else
            {
                auto method = instance->klass->findMethod(memberName);
                if (method)
                {
                    lastEvaluatedValue_ = PomeValue(method);
                }
                else
                {
                    lastEvaluatedValue_ = PomeValue(std::monostate{}); 
                }
            }
        }
        else
        {
            throw RuntimeError("Attempt to access member '" + memberName + "' of a non-environment, non-table, or non-instance object.", expr.getLine(), expr.getColumn());
        }
    }

    void Interpreter::visit(ListExpr &expr)
    {
        std::vector<PomeValue> elements;
        std::vector<std::unique_ptr<RootGuard>> guards;
        
        for (const auto &el : expr.getElements())
        {
            PomeValue v = evaluateExpression(*el);
            elements.push_back(v);
            guards.push_back(std::make_unique<RootGuard>(gc_, v.asObject()));
        }
        
        auto pomeList = gc_.allocate<PomeList>(std::move(elements));
        
        lastEvaluatedValue_ = PomeValue(pomeList);
    }

    void Interpreter::visit(TableExpr &expr)
    {
        std::map<PomeValue, PomeValue> elements;
        std::vector<std::unique_ptr<RootGuard>> guards;
        
        for (const auto &entry : expr.getEntries())
        {
            PomeValue key = evaluateExpression(*(entry.first));
            guards.push_back(std::make_unique<RootGuard>(gc_, key.asObject()));
            
            PomeValue value = evaluateExpression(*(entry.second));
            guards.push_back(std::make_unique<RootGuard>(gc_, value.asObject()));
            
            elements[key] = value;
        }
        
        auto pomeTable = gc_.allocate<PomeTable>(std::move(elements));
        lastEvaluatedValue_ = PomeValue(pomeTable);
    }

    void Interpreter::visit(IndexExpr &expr)
    {
        PomeValue objectValue = evaluateExpression(*expr.getObject());
        RootGuard objGuard(gc_, objectValue.asObject());
        
        PomeValue indexValue = evaluateExpression(*expr.getIndex());
        RootGuard indexGuard(gc_, indexValue.asObject());

        if (objectValue.isList())
        {
            if (!indexValue.isNumber())
            {
                throw RuntimeError("List index must be a number.", expr.getLine(), expr.getColumn());
            }

            auto list = objectValue.asList();
            double idxDouble = indexValue.asNumber();

            if (idxDouble != static_cast<long long>(idxDouble))
            {
                throw RuntimeError("List index must be an integer.", expr.getLine(), expr.getColumn());
            }

            long long idx = static_cast<long long>(idxDouble);

            if (idx < 0 || idx >= static_cast<long long>(list->elements.size()))
            {
                lastEvaluatedValue_ = PomeValue(std::monostate{});
            }
            else {
                lastEvaluatedValue_ = list->elements[idx];
            }
        }
        else if (objectValue.isTable())
        {
            auto table = objectValue.asTable();
            auto it = table->elements.find(indexValue);
            if (it != table->elements.end())
            {
                lastEvaluatedValue_ = it->second;
            }
            else
            {
                lastEvaluatedValue_ = PomeValue(std::monostate{});
            }
        }
        else
        {
            throw RuntimeError("Index access is only supported for lists and tables.", expr.getLine(), expr.getColumn());
        }
    }

    void Interpreter::visit(TernaryExpr &expr)
    {
        PomeValue condition = evaluateExpression(*expr.getCondition());
        if (condition.asBool())
        {
            lastEvaluatedValue_ = evaluateExpression(*expr.getThenExpr());
        }
        else
        {
            lastEvaluatedValue_ = evaluateExpression(*expr.getElseExpr());
        }
    }

    void Interpreter::visit(ThisExpr &expr)
    {
        try
        {
            lastEvaluatedValue_ = currentEnvironment_->get("this");
        }
        catch (const std::runtime_error &)
        {
            throw RuntimeError("'this' used outside of class method.", expr.getLine(), expr.getColumn());
        }
    }

    /**
     * --- Visitor methods for statements ---
     */

    void Interpreter::visit(VarDeclStmt &stmt)
    {
        PomeValue value;
        if (stmt.getInitializer())
        {
            value = evaluateExpression(*stmt.getInitializer());
        }
        else
        {
            value = PomeValue(std::monostate{}); 
        }
        currentEnvironment_->define(stmt.getName(), value);
    }

    void Interpreter::visit(AssignStmt &stmt)
    {
        PomeValue value = evaluateExpression(*stmt.getValue());
        /**
         * Root value
         */
        RootGuard valueGuard(gc_, value.asObject());

        Expression *target = stmt.getTarget();

        if (auto targetId = dynamic_cast<IdentifierExpr *>(target))
        {
            currentEnvironment_->assign(targetId->getName(), value);
        }
        else if (auto targetIndex = dynamic_cast<IndexExpr *>(target))
        {
            PomeValue objectValue = evaluateExpression(*targetIndex->getObject());
            RootGuard objGuard(gc_, objectValue.asObject());
            
            PomeValue indexValue = evaluateExpression(*targetIndex->getIndex());
            RootGuard indexGuard(gc_, indexValue.asObject());

            if (objectValue.isList())
            {
                if (!indexValue.isNumber())
                     throw RuntimeError("List assignment index must be a number.", stmt.getLine(), stmt.getColumn());

                auto list = objectValue.asList();
                double idxDouble = indexValue.asNumber();
                long long idx = static_cast<long long>(idxDouble);

                if (idx < 0 || idx >= static_cast<long long>(list->elements.size()))
                     throw RuntimeError("List assignment index out of bounds.", stmt.getLine(), stmt.getColumn());

                list->elements[idx] = value;
            }
            else if (objectValue.isTable())
            {
                auto table = objectValue.asTable();
                table->elements[indexValue] = value;
            }
            else
            {
                throw RuntimeError("Assignment index access is only supported for lists and tables.", stmt.getLine(), stmt.getColumn());
            }
        }
        else if (auto targetMember = dynamic_cast<MemberAccessExpr *>(target))
        {
            PomeValue objectValue = evaluateExpression(*targetMember->getObject());
            RootGuard objGuard(gc_, objectValue.asObject());
            
            if (objectValue.isTable())
            {
                auto table = objectValue.asTable();
                PomeString* keyStr = gc_.allocate<PomeString>(targetMember->getMember());
                PomeValue key(keyStr);
                table->elements[key] = value;
            }
            else if (objectValue.isInstance())
            {
                PomeInstance* instance = objectValue.asInstance();
                instance->set(targetMember->getMember(), value);
            }
            else
            {
                throw RuntimeError("Member assignment is only supported for tables and instances.", stmt.getLine(), stmt.getColumn());
            }
        }
        else
        {
            throw RuntimeError("Invalid assignment target.", stmt.getLine(), stmt.getColumn());
        }
    }

    void Interpreter::visit(IfStmt &stmt)
    {
        PomeValue condition = evaluateExpression(*stmt.getCondition());

        Environment* previousEnvironment = currentEnvironment_;
        currentEnvironment_ = gc_.allocate<Environment>(currentEnvironment_);

        try
        {
            if (condition.asBool())
            { 
                for (const auto &s : stmt.getThenBranch())
                {
                    executeStatement(*s);
                }
            }
            else
            {
                for (const auto &s : stmt.getElseBranch())
                {
                    executeStatement(*s);
                }
            }
        }
        catch (const ReturnException &e)
        {
            currentEnvironment_ = previousEnvironment; 
            throw;
        }

        currentEnvironment_ = previousEnvironment; 
    }

    void Interpreter::visit(WhileStmt &stmt)
    {
        Environment* previousEnvironment = currentEnvironment_;
        currentEnvironment_ = gc_.allocate<Environment>(currentEnvironment_);

        try
        {
            while (evaluateExpression(*stmt.getCondition()).asBool())
            {
                for (const auto &s : stmt.getBody())
                {
                    executeStatement(*s);
                }
            }
        }
        catch (const ReturnException &e)
        {
            currentEnvironment_ = previousEnvironment; 
            throw;
        }

        currentEnvironment_ = previousEnvironment; 
    }

    void Interpreter::visit(ForStmt &stmt)
    {
        Environment* previousEnvironment = currentEnvironment_;
        currentEnvironment_ = gc_.allocate<Environment>(currentEnvironment_);

        try
        {
            if (stmt.getInitializer())
            {
                executeStatement(*stmt.getInitializer());
            }

            while (true)
            {
                if (stmt.getCondition())
                {
                    PomeValue condition = evaluateExpression(*stmt.getCondition());
                    if (!condition.asBool())
                    {
                        break;
                    }
                }

                Environment* savedEnv = currentEnvironment_;
                currentEnvironment_ = gc_.allocate<Environment>(currentEnvironment_);

                try
                {
                    for (const auto &s : stmt.getBody())
                    {
                        executeStatement(*s);
                    }
                }
                catch (...)
                {
                    currentEnvironment_ = savedEnv;
                    throw;
                }
                currentEnvironment_ = savedEnv;

                if (stmt.getIncrement())
                {
                    executeStatement(*stmt.getIncrement());
                }
            }
        }
        catch (const ReturnException &e)
        {
            currentEnvironment_ = previousEnvironment; 
            throw;
        }

        currentEnvironment_ = previousEnvironment; 
    }

    void Interpreter::visit(ForEachStmt &stmt)
    {
        PomeValue iterableValue = evaluateExpression(*stmt.getIterable());

        Environment* previousEnvironment = currentEnvironment_;
        currentEnvironment_ = gc_.allocate<Environment>(currentEnvironment_);

        try
        {
            if (iterableValue.isList())
            {
                auto list = iterableValue.asList();
                for (const auto &item : list->elements)
                {
                    currentEnvironment_->define(stmt.getVarName(), item);

                    Environment* savedEnv = currentEnvironment_;
                    currentEnvironment_ = gc_.allocate<Environment>(currentEnvironment_);

                    try
                    {
                        for (const auto &s : stmt.getBody())
                        {
                            executeStatement(*s);
                        }
                    }
                    catch (...)
                    {
                        currentEnvironment_ = savedEnv;
                        throw;
                    }
                    currentEnvironment_ = savedEnv;
                }
            }
            else if (iterableValue.isTable())
            {
                auto table = iterableValue.asTable();
                for (const auto &pair : table->elements)
                {
                    currentEnvironment_->define(stmt.getVarName(), pair.first);

                    Environment* savedEnv = currentEnvironment_;
                    currentEnvironment_ = gc_.allocate<Environment>(currentEnvironment_);

                    try
                    {
                        for (const auto &s : stmt.getBody())
                        {
                            executeStatement(*s);
                        }
                    }
                    catch (...)
                    {
                        currentEnvironment_ = savedEnv;
                        throw;
                    }
                    currentEnvironment_ = savedEnv;
                }
            }
            else if (iterableValue.isInstance())
            {
                auto instance = iterableValue.asInstance();
                auto iteratorMethod = instance->klass->findMethod("iterator");
                if (!iteratorMethod)
                {
                    throw RuntimeError("Object is not iterable (no 'iterator' method).", stmt.getLine(), stmt.getColumn());
                }

                PomeValue iteratorObj = callPomeFunction(iteratorMethod, {}, instance);
                if (!iteratorObj.isInstance())
                {
                    throw RuntimeError("'iterator' method must return an object instance.", stmt.getLine(), stmt.getColumn());
                }

                auto iteratorInstance = iteratorObj.asInstance();
                auto nextMethod = iteratorInstance->klass->findMethod("next");
                if (!nextMethod)
                {
                    throw RuntimeError("Iterator object must have 'next' method.", stmt.getLine(), stmt.getColumn());
                }

                while (true)
                {
                    PomeValue item = callPomeFunction(nextMethod, {}, iteratorInstance);
                    if (item.isNil())
                        break; 

                    currentEnvironment_->define(stmt.getVarName(), item);

                    Environment* savedEnv = currentEnvironment_;
                    currentEnvironment_ = gc_.allocate<Environment>(currentEnvironment_);

                    try
                    {
                        for (const auto &s : stmt.getBody())
                        {
                            executeStatement(*s);
                        }
                    }
                    catch (...)
                    {
                        currentEnvironment_ = savedEnv;
                        throw;
                    }
                    currentEnvironment_ = savedEnv;
                }
            }
            else
            {
                throw RuntimeError("For-each loop expects a list, table, or iterable object.", stmt.getLine(), stmt.getColumn());
            }
        }
        catch (const ReturnException &e)
        {
            currentEnvironment_ = previousEnvironment;
            throw;
        }

        currentEnvironment_ = previousEnvironment;
    }

    void Interpreter::visit(ReturnStmt &stmt)
    {
        PomeValue returnValue;
        if (stmt.getValue())
        {
            returnValue = evaluateExpression(*stmt.getValue());
        }
        else
        {
            returnValue = PomeValue(std::monostate{}); 
        }
        throw ReturnException(returnValue); 
    }

    void Interpreter::visit(ExpressionStmt &stmt)
    {
        evaluateExpression(*stmt.getExpression());
    }

    void Interpreter::visit(FunctionDeclStmt &stmt)
    {
        auto pomeFunc = gc_.allocate<PomeFunction>();
        pomeFunc->name = stmt.getName();
        pomeFunc->parameters = stmt.getParams();
        pomeFunc->body = &(stmt.getBody());         
        pomeFunc->closureEnv = currentEnvironment_; 

        currentEnvironment_->define(stmt.getName(), PomeValue(pomeFunc));
    }

    void Interpreter::visit(ClassDeclStmt &stmt)
    {
        auto pomeClass = gc_.allocate<PomeClass>(stmt.getName());

        for (const auto &methodStmt : stmt.getMethods())
        {
            auto pomeFunc = gc_.allocate<PomeFunction>();
            pomeFunc->name = methodStmt->getName();
            pomeFunc->parameters = methodStmt->getParams();
            pomeFunc->body = &(methodStmt->getBody());
            pomeFunc->closureEnv = currentEnvironment_; 

            pomeClass->methods[pomeFunc->name] = pomeFunc;
        }

        currentEnvironment_->define(stmt.getName(), PomeValue(pomeClass));
    }

    void Interpreter::visit(ImportStmt &stmt)
    {
        std::string moduleName = stmt.getModuleName();

        PomeModule* moduleObj = loadModule(moduleName);

        currentEnvironment_->define(moduleName, PomeValue(moduleObj));
    }

    void Interpreter::visit(FromImportStmt &stmt)
    {
        std::string moduleName = stmt.getModuleName();

        PomeModule* moduleObj = loadModule(moduleName);

        for (const auto &symbol : stmt.getSymbols())
        {
            
            /**
             * We must create a PomeString to lookup.
             */
            PomeString* symStr = gc_.allocate<PomeString>(symbol);
            PomeValue key(symStr);
            
            if (moduleObj->exports.count(key))
            {
                currentEnvironment_->define(symbol, moduleObj->exports[key]);
            }
            else
            {
                throw std::runtime_error("Symbol '" + symbol + "' not exported from module '" + moduleName + "'.");
            }
        }
    }
    
    void Interpreter::visit(ExportStmt &stmt)
    {
        /**
         * Execute statement to define variable in current scope
         */
        stmt.getStmt()->accept(*this);

        std::string name;
        if (auto v = dynamic_cast<VarDeclStmt *>(stmt.getStmt()))
        {
            name = v->getName();
        }
        else if (auto f = dynamic_cast<FunctionDeclStmt *>(stmt.getStmt()))
        {
            name = f->getName();
        }
        else
        {
            return;
        }

        try
        {
            PomeValue val = currentEnvironment_->get(name);
            if (!exportStack_.empty())
            {
                /**
                 * Create key
                 */
                PomeString* keyStr = gc_.allocate<PomeString>(name);
                PomeValue key(keyStr);
                
                /**
                 * Add to current exports
                 */
                exportStack_.back()->exports[key] = val;
            }
        }
        catch (const std::runtime_error &)
        {
        }
    }

    PomeModule* Interpreter::loadModule(const std::string &moduleName)
    {
        if (executedModules_.count(moduleName))
        {
            return executedModules_[moduleName];
        }
        
        if (moduleName == "math")
        {
            PomeModule* mod = StdLib::createMathModule(gc_);
            executedModules_[moduleName] = mod;
            return mod;
        }
        if (moduleName == "io")
        {
            PomeModule* mod = StdLib::createIOModule(gc_);
            executedModules_[moduleName] = mod;
            return mod;
        }
        if (moduleName == "string")
        {
            PomeModule* mod = StdLib::createStringModule(gc_);
            executedModules_[moduleName] = mod;
            return mod;
        }

        std::shared_ptr<Program> program = importer_.import(moduleName);
        
        /**
         * Prepare new module execution environment
         * Parent is global environment to access globals like 'print'
         */
        Environment* moduleEnv = gc_.allocate<Environment>(globalEnvironment_);
        PomeModule* moduleObj = gc_.allocate<PomeModule>();
        
        executedModules_[moduleName] = moduleObj;
        
        Environment* previousEnvironment = currentEnvironment_;
        currentEnvironment_ = moduleEnv;
        exportStack_.push_back(moduleObj);
        
        try
        {
            program->accept(*this);
        }
        catch (const ReturnException &e)
        {
            /**
             * Module return allowed?
             */
        }
        catch (...)
        {
            currentEnvironment_ = previousEnvironment;
            exportStack_.pop_back();
            executedModules_.erase(moduleName); // Remove partial module on error
            throw;
        }
        
        currentEnvironment_ = previousEnvironment;
        exportStack_.pop_back();
        
        return moduleObj;
    }
    
    /**
     * --- Program visitor ---
     */

    void Interpreter::visit(Program &program)
    {
        for (const auto &stmt : program.getStatements())
        {
            executeStatement(*stmt);
        }
    }

    /**
     * --- Binary and Unary Operation helpers ---
     */

    PomeValue Interpreter::applyBinaryOp(const PomeValue &left, const std::string &op, const PomeValue &right)
    {
        /**
         * Operator Overloading
         */
        if (left.isInstance())
        {
            PomeInstance* instance = left.asInstance();
            std::string methodName;
            if (op == "+")
                methodName = "__add__";
            else if (op == "-")
                methodName = "__sub__";
            else if (op == "*")
                methodName = "__mul__";
            else if (op == "/")
                methodName = "__div__";
            else if (op == "%")
                methodName = "__mod__";
            else if (op == "==")
                methodName = "__eq__";
            else if (op == "<")
                methodName = "__lt__";
            else if (op == "<=")
                methodName = "__le__";
            else if (op == ">")
                methodName = "__gt__";
            else if (op == ">=")
                methodName = "__ge__";

            if (!methodName.empty())
            {
                auto method = instance->klass->findMethod(methodName);
                if (method)
                {
                    std::vector<PomeValue> args = {right};
                    return callPomeFunction(method, args, instance);
                }
            }
        }

        /**
         * Number operations
         */
        if (left.isNumber() && right.isNumber())
        {
            double lVal = left.asNumber();
            double rVal = right.asNumber();

            if (op == "+")
                return PomeValue(lVal + rVal);
            if (op == "-")
                return PomeValue(lVal - rVal);
            if (op == "*")
                return PomeValue(lVal * rVal);
            if (op == "/")
            {
                if (rVal == 0.0)
                    throw std::runtime_error("Division by zero.");
                return PomeValue(lVal / rVal);
            }
            if (op == "%")
            {
                if (rVal == 0.0)
                    throw std::runtime_error("Modulo by zero.");
                return PomeValue(fmod(lVal, rVal));
            }
            if (op == "==")
                return PomeValue(lVal == rVal);
            if (op == "!=")
                return PomeValue(lVal != rVal);
            if (op == "<")
                return PomeValue(lVal < rVal);
            if (op == "<=")
                return PomeValue(lVal <= rVal);
            if (op == ">")
                return PomeValue(lVal > rVal);
            if (op == ">=")
                return PomeValue(lVal >= rVal);
        }

        /**
         * String concatenation
         */
        if (op == "+" && left.isString() && right.isString())
        {
             PomeString* s = gc_.allocate<PomeString>(left.asString() + right.asString());
             return PomeValue(s);
        }
        /**
         * Equality for other types
         */
        if (op == "==")
            return PomeValue(left == right);
        if (op == "!=")
            return PomeValue(left != right);

        throw std::runtime_error("Unsupported binary operation '" + op + "' between " +
                                 left.toString() + " and " + right.toString());
    }

    PomeValue Interpreter::applyUnaryOp(const std::string &op, const PomeValue &operand)
    {
        /**
         * Operator Overloading
         */
        if (operand.isInstance())
        {
            PomeInstance* instance = operand.asInstance();
            std::string methodName;
            if (op == "-")
                methodName = "__neg__";
            else if (op == "!")
                methodName = "__not__";

            if (!methodName.empty())
            {
                auto method = instance->klass->findMethod(methodName);
                if (method)
                {
                    return callPomeFunction(method, {}, instance); // Unary ops have no args
                }
            }
        }

        if (op == "-")
        {
            if (operand.isNumber())
            {
                return PomeValue(-operand.asNumber());
            }
            throw std::runtime_error("Attempt to unary negate a non-number value.");
        }
        if (op == "!")
        { // Logical NOT
            return PomeValue(!operand.asBool());
        }
        throw std::runtime_error("Unsupported unary operation: " + op);
    }

} // namespace Pome