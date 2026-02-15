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

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

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

    PomeValue nativeType(GarbageCollector &gc, const std::vector<PomeValue> &args)
    {
        if (args.size() != 1)
        {
            throw std::runtime_error("type() expects 1 argument.");
        }

        if (args[0].isNumber())
        {
            return PomeValue(gc.allocate<PomeString>("number"));
        }
        else if (args[0].isString())
        {
            return PomeValue(gc.allocate<PomeString>("string"));
        }
        else if (args[0].isBool())
        {
            return PomeValue(gc.allocate<PomeString>("boolean"));
        }
        else if (args[0].isNil())
        {
            return PomeValue(gc.allocate<PomeString>("nil"));
        }
        else if (args[0].isList())
        {
            return PomeValue(gc.allocate<PomeString>("list"));
        }
        else if (args[0].isTable())
        {
            return PomeValue(gc.allocate<PomeString>("table"));
        }
        else if (args[0].isFunction())
        {
            return PomeValue(gc.allocate<PomeString>("function"));
        }
        else if (args[0].isClass())
        {
            return PomeValue(gc.allocate<PomeString>("class"));
        }
        else if (args[0].isInstance())
        {
            return PomeValue(gc.allocate<PomeString>("instance"));
        }
        else if (args[0].isModule())
        {
            return PomeValue(gc.allocate<PomeString>("module"));
        }
        else if (auto obj = args[0].asObject())
        {
            if (obj->type() == ObjectType::NATIVE_OBJECT)
            {
                return PomeValue(gc.allocate<PomeString>("native_object"));
            }
        }

        return PomeValue(gc.allocate<PomeString>("unknown"));
    }

    Interpreter::Interpreter() : importer_(*this)
    {
        gc_.setInterpreter(this);
        currentEnvironment_ = gc_.allocate<Environment>(this, nullptr);
        globalEnvironment_ = currentEnvironment_; // Set global environment

        setupGlobalEnvironment();

        // The addSearchPath calls are no longer needed as the Importer's constructor
        // now sets up all search paths according to the specification.
        importer_.addSearchPath("examples/");
        importer_.addSearchPath("examples/modules/");

        /**
         * Allocate initial export module
         */
        PomeModule *rootModule = gc_.allocate<PomeModule>();
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

        auto typeFn = gc_.allocate<NativeFunction>("type", [this](const std::vector<PomeValue> &args)
                                                   { return nativeType(gc_, args); });
        currentEnvironment_->define("type", PomeValue(typeFn));

        auto gcCountFn = gc_.allocate<NativeFunction>("gc_count", [this](const std::vector<PomeValue> &args)
                                                      { return PomeValue((double)gc_.getObjectCount()); });
        currentEnvironment_->define("gc_count", PomeValue(gcCountFn));

        auto gcCollectFn = gc_.allocate<NativeFunction>("gc_collect", [this](const std::vector<PomeValue> &args)
                                                        {
            gc_.collect();
            return PomeValue(std::monostate{}); });
        currentEnvironment_->define("gc_collect", PomeValue(gcCollectFn));

        /**
         * Add global constants
         */
        currentEnvironment_->define("PI", PomeValue(3.141592653589793));
    }

    void Interpreter::markRoots()
    {
        /**
         * Mark current and global environment
         */
        gc_.markObject(currentEnvironment_);
        gc_.markObject(globalEnvironment_);

        /**
         * Mark environment stack (call stack roots)
         */
        for (auto *env : environmentStack_)
        {
            gc_.markObject(env);
        }

        /**
         * Mark export stack
         */
        for (auto *mod : exportStack_)
        {
            gc_.markObject(mod);
        }

        /**
         * Mark executed modules
         */
        for (auto &pair : executedModules_)
        {
            gc_.markObject(pair.second);
        }

        /**
         * Mark module cache
         */
        for (auto const &[key, val] : importer_.getModuleCache())
        {
            if (val.asObject())
            {
                gc_.markObject(val.asObject());
            }
        }

        /**
         * Mark last evaluated value
         */
        if (lastEvaluatedValue_.asObject())
        {
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
            // std::cerr << "Runtime error: " << e.what() << std::endl;
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
        PomeString *s = gc_.allocate<PomeString>(expr.getValue());
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
         * Temporarily root left while evaluating right (or deciding not to)
         */
        RootGuard guard(gc_, left.asObject());

        std::string op = expr.getOperator();

        if (op == "and")
        {
            if (!left.asBool())
            {
                lastEvaluatedValue_ = left;
                return;
            }
        }
        else if (op == "or")
        {
            if (left.asBool())
            {
                lastEvaluatedValue_ = left;
                return;
            }
        }

        PomeValue right = evaluateExpression(*expr.getRight());

        try
        {
            lastEvaluatedValue_ = applyBinaryOp(left, op, right);
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
                PomeInstance *instance = objectValue.asInstance();
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
            else if (objectValue.isModule())
            {
                auto module = objectValue.asModule();
                PomeString *keyStr = gc_.allocate<PomeString>(memberName);
                PomeValue key(keyStr);

                if (module->exports.count(key))
                    calleeValue = module->exports[key];
                else
                    calleeValue = PomeValue(std::monostate{});
            }
            else if (objectValue.isTable())
            {
                auto table = objectValue.asTable();
                PomeString *keyStr = gc_.allocate<PomeString>(memberName);
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
            PomeClass *klass = calleeValue.asClass();
            PomeInstance *instance = gc_.allocate<PomeInstance>(klass);

            auto initMethod = klass->findMethod("init");
            if (initMethod)
            {
                std::vector<PomeValue> args;
                RootGuard instanceGuard(gc_, instance);

                for (const auto &argExpr : expr.getArgs())
                {
                    args.push_back(evaluateExpression(*argExpr));
                }

                // callPomeFunction logic handles stack internally now?
                // No, I need to find callPomeFunction.
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

        if (calleeValue.isNil())
        {
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
            NativeFunction *nativeFunc = calleeValue.asNativeFunction();
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
            PomeFunction *pomeFunc = calleeValue.asPomeFunction();
            PomeInstance *thisInstance = nullptr;
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

    PomeValue Interpreter::callPomeFunction(PomeFunction *pomeFunc, const std::vector<PomeValue> &args, PomeInstance *thisInstance)
    {
        if (args.size() != pomeFunc->parameters.size())
        {
            throw std::runtime_error("Function '" + pomeFunc->name + "' expected " +
                                     std::to_string(pomeFunc->parameters.size()) +
                                     " arguments, but got " + std::to_string(args.size()) + ".");
        }

        Environment *previousEnvironment = currentEnvironment_;
        
        // Push caller's environment to stack so GC can see it even if the new environment doesn't link to it.
        environmentStack_.push_back(previousEnvironment);
        
        /**
         * New environment rooted via currentEnvironment_ once assigned
         */
        currentEnvironment_ = gc_.allocate<Environment>(this, pomeFunc->closureEnv);

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
        environmentStack_.pop_back(); // Remove from GC root stack

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

        // Check if it's a module
        if (objectValue.isModule())
        {
            PomeModule *module = objectValue.asModule();

            PomeString *keyStr = gc_.allocate<PomeString>(memberName); // Key for lookup. Allocate PomeString via GC.
            PomeValue key(keyStr);

            auto it = module->exports.find(key); // Use find for map lookup

            if (it != module->exports.end())
            {
                lastEvaluatedValue_ = it->second;
            }
            else
            {
                throw std::runtime_error("Module '" + module->toString() + "' does not have member '" + memberName + "'.");
            }
            return;
        }
        else if (objectValue.isTable())
        {
            auto table = objectValue.asTable();
            PomeString *keyStr = gc_.allocate<PomeString>(memberName);
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
            PomeInstance *instance = objectValue.asInstance();
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

            if (idx < 0)
                idx += list->elements.size(); // Handle negative index

            if (idx < 0 || idx >= static_cast<long long>(list->elements.size()))
            {
                lastEvaluatedValue_ = PomeValue(std::monostate{});
            }
            else
            {
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

    void Interpreter::visit(SliceExpr &expr)
    {
        PomeValue objectValue = evaluateExpression(*expr.getObject());
        RootGuard objGuard(gc_, objectValue.asObject());

        if (!objectValue.isList() && !objectValue.isString())
        {
            throw RuntimeError("Slicing is only supported for lists and strings.", expr.getLine(), expr.getColumn());
        }

        long long len = 0;
        if (objectValue.isList())
            len = objectValue.asList()->elements.size();
        else
            len = objectValue.asString().length();

        long long startIdx = 0;
        if (expr.getStart())
        {
            PomeValue startVal = evaluateExpression(*expr.getStart());
            if (!startVal.isNumber())
                throw RuntimeError("Slice start must be a number.", expr.getLine(), expr.getColumn());
            startIdx = static_cast<long long>(startVal.asNumber());
            if (startIdx < 0)
                startIdx += len;
            if (startIdx < 0)
                startIdx = 0;
            if (startIdx > len)
                startIdx = len;
        }

        long long endIdx = len;
        if (expr.getEnd())
        {
            PomeValue endVal = evaluateExpression(*expr.getEnd());
            if (!endVal.isNumber())
                throw RuntimeError("Slice end must be a number.", expr.getLine(), expr.getColumn());
            endIdx = static_cast<long long>(endVal.asNumber());
            if (endIdx < 0)
                endIdx += len;
            if (endIdx < 0)
                endIdx = 0;
            if (endIdx > len)
                endIdx = len;
        }

        if (startIdx > endIdx)
            startIdx = endIdx;

        if (objectValue.isList())
        {
            std::vector<PomeValue> newElements;
            auto list = objectValue.asList();
            for (long long i = startIdx; i < endIdx; ++i)
            {
                newElements.push_back(list->elements[i]);
            }
            auto newList = gc_.allocate<PomeList>(std::move(newElements));
            lastEvaluatedValue_ = PomeValue(newList);
        }
        else
        {
            std::string s = objectValue.asString();
            std::string sub = s.substr(startIdx, endIdx - startIdx);
            auto newStr = gc_.allocate<PomeString>(sub);
            lastEvaluatedValue_ = PomeValue(newStr);
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

    void Interpreter::visit(FunctionExpr &expr)
    {
        auto pomeFunc = gc_.allocate<PomeFunction>();
        pomeFunc->name = expr.getName();
        pomeFunc->parameters = expr.getParams();
        pomeFunc->body = &(expr.getBody()); // Store raw pointer to AST body
        pomeFunc->closureEnv = currentEnvironment_;

        lastEvaluatedValue_ = PomeValue(pomeFunc);
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

                if (idx < 0)
                    throw RuntimeError("List assignment index cannot be negative.", stmt.getLine(), stmt.getColumn());

                if (idx >= static_cast<long long>(list->elements.size()))
                {
                    if (idx == static_cast<long long>(list->elements.size()))
                    {
                        list->elements.push_back(value);
                    }
                    else
                    {
                        throw RuntimeError("List assignment index out of bounds (can only append to end).", stmt.getLine(), stmt.getColumn());
                    }
                }
                else
                {
                    list->elements[idx] = value;
                }
            }
            else if (objectValue.isTable())
            {
                auto table = objectValue.asTable();
                // The indexValue is already a PomeValue, use it directly as the key
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
                PomeString *keyStr = gc_.allocate<PomeString>(targetMember->getMember());
                PomeValue key(keyStr);
                table->elements[key] = value;
            }
            else if (objectValue.isInstance())
            {
                PomeInstance *instance = objectValue.asInstance();
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

        Environment *previousEnvironment = currentEnvironment_;
        currentEnvironment_ = gc_.allocate<Environment>(this, currentEnvironment_);

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
        Environment *previousEnvironment = currentEnvironment_;
        currentEnvironment_ = gc_.allocate<Environment>(this, currentEnvironment_);

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
        Environment *previousEnvironment = currentEnvironment_;
        currentEnvironment_ = gc_.allocate<Environment>(this, currentEnvironment_);

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

                Environment *savedEnv = currentEnvironment_;
                currentEnvironment_ = gc_.allocate<Environment>(this, currentEnvironment_);

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

        Environment *previousEnvironment = currentEnvironment_;
        currentEnvironment_ = gc_.allocate<Environment>(this, currentEnvironment_);

        try
        {
            if (iterableValue.isList())
            {
                auto list = iterableValue.asList();
                for (const auto &item : list->elements)
                {
                    currentEnvironment_->define(stmt.getVarName(), item);

                    Environment *savedEnv = currentEnvironment_;
                    currentEnvironment_ = gc_.allocate<Environment>(this, currentEnvironment_);

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

                    Environment *savedEnv = currentEnvironment_;
                    currentEnvironment_ = gc_.allocate<Environment>(this, currentEnvironment_);

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

                    Environment *savedEnv = currentEnvironment_;
                    currentEnvironment_ = gc_.allocate<Environment>(this, currentEnvironment_);

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
        pomeFunc->body = &(stmt.getBody()); // Store raw pointer to AST body
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
            pomeFunc->body = &(methodStmt->getBody()); // Store raw pointer to AST body
            pomeFunc->closureEnv = currentEnvironment_;

            pomeClass->methods[pomeFunc->name] = pomeFunc;
        }

        currentEnvironment_->define(stmt.getName(), PomeValue(pomeClass));
    }

    void Interpreter::visit(ImportStmt &stmt)
    {
        std::string moduleName = stmt.getModuleName();

        PomeModule *moduleObj = loadModule(moduleName);

        currentEnvironment_->define(moduleName, PomeValue(moduleObj));
    }

    void Interpreter::visit(FromImportStmt &stmt)
    {
        std::string moduleName = stmt.getModuleName();

        PomeModule *moduleObj = loadModule(moduleName);

        for (const auto &symbol : stmt.getSymbols())
        {

            /**
             * We must create a PomeString to lookup.
             */
            PomeString *symStr = gc_.allocate<PomeString>(symbol);
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
        // Execute the statement being exported
        stmt.getStmt()->accept(*this);

        std::string exportName;
        PomeValue exportedValue;

        if (auto v = dynamic_cast<VarDeclStmt *>(stmt.getStmt()))
        {
            exportName = v->getName();
            exportedValue = currentEnvironment_->get(exportName);
        }
        else if (auto f = dynamic_cast<FunctionDeclStmt *>(stmt.getStmt()))
        {
            exportName = f->getName();
            exportedValue = currentEnvironment_->get(exportName);
        }
        else if (auto c = dynamic_cast<ClassDeclStmt *>(stmt.getStmt()))
        {
            exportName = c->getName();
            exportedValue = currentEnvironment_->get(exportName);
        }
        else
        {
            throw RuntimeError("Unsupported statement type for 'export'.", stmt.getLine(), stmt.getColumn());
        }

        if (exportStack_.empty())
        {
            throw RuntimeError("Export statement used outside of a module context.", stmt.getLine(), stmt.getColumn());
        }
        PomeModule *currentModule = exportStack_.back();
        PomeString *keyStr = gc_.allocate<PomeString>(exportName);
        currentModule->exports[PomeValue(keyStr)] = exportedValue;
    }

    void Interpreter::visit(ExportExpressionStmt &stmt)

    {

        // Evaluate the expression being exported

        PomeValue exportedValue = evaluateExpression(*stmt.getExpression());

        // Root exportedValue during name resolution if it's an object

        RootGuard valueGuard(gc_, exportedValue.asObject());

        if (exportStack_.empty())
        {

            throw RuntimeError("Export statement used outside of a module context.", stmt.getLine(), stmt.getColumn());
        }

        std::string exportName;

        // The name of the exported symbol is based on the expression.

        // It could be a simple IdentifierExpr or a MemberAccessExpr.

        if (auto idExpr = dynamic_cast<IdentifierExpr *>(stmt.getExpression()))
        {

            exportName = idExpr->getName();
        }
        else if (auto memberAccessExpr = dynamic_cast<MemberAccessExpr *>(stmt.getExpression()))
        {

            // For member access, the last member name is the exported name

            // Example: 'export my_module.greet;' should export 'greet'.

            exportName = memberAccessExpr->getMember();
        }
        else
        {

            throw RuntimeError("Exporting non-identifier or non-member-access expressions directly is not supported.", stmt.getLine(), stmt.getColumn());
        }

        PomeModule *currentModule = exportStack_.back();

        PomeString *keyStr = gc_.allocate<PomeString>(exportName);

        PomeValue key(keyStr);

        currentModule->exports[key] = exportedValue;
    }

    PomeModule *Interpreter::loadModule(const std::string &moduleName)
    {
        // Delegate module resolution and loading to the Importer
        PomeValue moduleValue = importer_.import(moduleName);
        if (!moduleValue.isModule())
        {
            throw std::runtime_error("Importer returned non-module PomeValue for module: " + moduleName);
        }

        // Cache this locally too for quick access (importer also caches)
        executedModules_[moduleName] = moduleValue.asModule();
        return moduleValue.asModule();
    }

    // Helper for native module loading
    PomeValue Interpreter::loadNativeModule(const std::string &libraryPath)
    {
        void *handle = nullptr;
#ifdef _WIN32
        handle = LoadLibrary(libraryPath.c_str());
#else
        handle = dlopen(libraryPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif

        if (!handle)
        {
#ifdef _WIN32
            // GetLastError for Windows error message
            throw std::runtime_error("NativeModuleError: Failed to load native library '" + libraryPath + "'.");
#else
            throw std::runtime_error("NativeModuleError: Failed to load native library '" + libraryPath + "': " + dlerror());
#endif
        }

        // Reset errors for dlsym
#ifndef _WIN32
        dlerror();
#endif

        // Look up the entry point function
        // The function signature is `Pome::Value PomeInitModule(Pome::Interpreter* interp);
        using InitModuleFunc = Pome::PomeValue (*)(Pome::Interpreter *);
        InitModuleFunc initFunc = nullptr;

#ifdef _WIN32
        initFunc = (InitModuleFunc)GetProcAddress((HMODULE)handle, "PomeInitModule");
#else
        initFunc = (InitModuleFunc)dlsym(handle, "PomeInitModule");
#endif

        const char *dlsym_error = nullptr;
#ifndef _WIN32
        dlsym_error = dlerror();
#endif
        if (dlsym_error || !initFunc)
        {
#ifdef _WIN32
            FreeLibrary((HMODULE)handle);
#else
            dlclose(handle);
#endif
            throw std::runtime_error("NativeModuleError: Native module '" + libraryPath + "' does not export 'PomeInitModule' function or symbol lookup failed.");
        }

        // Call the initialization function, passing the current interpreter
        Pome::PomeValue moduleTable = initFunc(this);

        // Optionally store the handle if we need to manage the lifetime of the loaded library
        // For now, we don't explicitly store handles, assuming the library remains loaded.
        return moduleTable; // The Pome::Table object returned by the native module
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
            PomeInstance *instance = left.asInstance();
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

        // Logical AND and OR (short-circuiting)
        if (op == "and")
        {
            // Root left while evaluating its truthiness to prevent GC if it's an object
            RootGuard leftGuard(gc_, left.asObject());
            if (!left.asBool())
            { // Short-circuit if left is falsy
                return left;
            }
            return right; // Return right operand's value
        }
        if (op == "or")
        {
            // Root left while evaluating its truthiness to prevent GC if it's an object
            RootGuard leftGuard(gc_, left.asObject());
            if (left.asBool())
            { // Short-circuit if left is truthy
                return left;
            }
            return right; // Return right operand's value
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
            if (op == "^")
            {
                return PomeValue(std::pow(lVal, rVal));
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
         * String concatenation (with implicit conversion for right operand if string is left operand)
         */
        if (op == "+" && left.isString())
        {
            PomeString *s = gc_.allocate<PomeString>(left.asString() + right.toString());
            return PomeValue(s);
        }

        /**
         * List concatenation
         */
        if (op == "+" && left.isList() && right.isList())
        {
            auto list1 = left.asList();
            auto list2 = right.asList();
            std::vector<PomeValue> newElements = list1->elements;
            newElements.insert(newElements.end(), list2->elements.begin(), list2->elements.end());
            auto newList = gc_.allocate<PomeList>(std::move(newElements));
            return PomeValue(newList);
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
            PomeInstance *instance = operand.asInstance();
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
        if (op == "!" || op == "not")
        { // Logical NOT
            return PomeValue(!operand.asBool());
        }
        throw std::runtime_error("Unsupported unary operation: " + op);
    }

} // namespace Pome