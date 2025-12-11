#include "../include/pome_interpreter.h"
#include "../include/pome_lexer.h"
#include "../include/pome_parser.h"
#include "../include/pome_errors.h" // Added
#include "../include/pome_stdlib.h" // Added Standard Library
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

    // Custom exception for handling return statements
    class ReturnException : public std::runtime_error
    {
    public:
        PomeValue value;
        explicit ReturnException(const PomeValue &val) : std::runtime_error("Return statement"), value(val) {}
    };

    // Native print function for the Pome language
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

    // Native len function: returns the length of a string, list, or table
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

    // Native tonumber function: converts a string to a number (or nil if invalid)
    PomeValue nativeToNumber(const std::vector<PomeValue> &args)
    {
        if (args.size() != 1)
        {
            throw std::runtime_error("tonumber() expects 1 argument.");
        }
        if (!args[0].isString())
        {
            return PomeValue(std::monostate{}); // If not a string, return nil (Lua-like behavior)
        }

        try
        {
            size_t pos;
            double d = std::stod(args[0].asString(), &pos);
            if (pos == args[0].asString().length())
            { // Successfully converted entire string
                return PomeValue(d);
            }
            else
            {
                return PomeValue(std::monostate{}); // Partial conversion, return nil
            }
        }
        catch (const std::out_of_range &oor)
        {
            return PomeValue(std::monostate{}); // Value out of range, return nil
        }
        catch (const std::invalid_argument &ia)
        {
            return PomeValue(std::monostate{}); // Not a valid number, return nil
        }
    }

    Interpreter::Interpreter()
    {
        currentEnvironment_ = std::make_shared<Environment>();
        setupGlobalEnvironment();

        // Initialize search paths from POME_PATH environment variable
        const char *pomePathEnv = std::getenv("POME_PATH");
        if (pomePathEnv)
        {
            std::string paths(pomePathEnv);
            std::stringstream ss(paths);
            std::string path;
            while (std::getline(ss, path, ':'))
            {
                if (!path.empty())
                {
                    searchPaths_.push_back(path);
                }
            }
        }

        searchPaths_.push_back("."); // Default search path

        // Initialize export stack
        exportStack_.push_back(std::make_shared<std::map<PomeValue, PomeValue>>());

        // Seed random number generator
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    void Interpreter::setupGlobalEnvironment()
    {
        // Add native functions to the global environment
        currentEnvironment_->define("print", PomeValue(std::make_shared<NativeFunction>("print", nativePrint)));
        currentEnvironment_->define("len", PomeValue(std::make_shared<NativeFunction>("len", nativeLen)));
        currentEnvironment_->define("tonumber", PomeValue(std::make_shared<NativeFunction>("tonumber", nativeToNumber)));

        // Add global constants if any
        currentEnvironment_->define("PI", PomeValue(3.141592653589793));
    }

    void Interpreter::interpret(Program &program)
    { // Removed const
        try
        {
            // Enclose program execution in a try-catch for potential top-level returns (shouldn't happen in Pome)
            program.accept(*this);
        }
        catch (const ReturnException &e)
        {
            // A top-level return should not happen in Pome as a script.
            // If it does, we just store the value, but it won't affect anything.
            lastEvaluatedValue_ = e.value;
        }
        catch (const PomeException &e)
        { // Catch Pome exceptions
            std::cerr << e.what() << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Runtime error: " << e.what() << std::endl;
        }
    }

    // Helper to evaluate expressions
    PomeValue Interpreter::evaluateExpression(Expression &expr)
    {
        expr.accept(*this);
        return lastEvaluatedValue_;
    }

    // Helper to execute statements
    void Interpreter::executeStatement(Statement &stmt)
    {
        stmt.accept(*this);
    }

    // --- Visitor methods for expressions ---

    void Interpreter::visit(NumberExpr &expr)
    {
        lastEvaluatedValue_ = PomeValue(expr.getValue());
    }

    void Interpreter::visit(StringExpr &expr)
    {
        lastEvaluatedValue_ = PomeValue(expr.getValue());
    }

    void Interpreter::visit(IdentifierExpr &expr)
    {
        lastEvaluatedValue_ = currentEnvironment_->get(expr.getName());
    }

    void Interpreter::visit(BinaryExpr &expr)
    {
        PomeValue left = evaluateExpression(*expr.getLeft());
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

        // Check if callee is member access to capture 'this'
        if (auto memberAccess = dynamic_cast<MemberAccessExpr *>(expr.getCallee()))
        {
            PomeValue objectValue = evaluateExpression(*memberAccess->getObject());
            std::string memberName = memberAccess->getMember();

            if (objectValue.isInstance())
            {
                std::shared_ptr<PomeInstance> instance = objectValue.asInstance();
                // Check fields first
                PomeValue field = instance->get(memberName);
                if (!field.isNil())
                {
                    calleeValue = field;
                }
                else
                {
                    // Check class methods
                    auto method = instance->klass->findMethod(memberName);
                    if (method)
                    {
                        calleeValue = PomeValue(method);
                        thisValue = objectValue; // Bind 'this'
                        isMethodCall = true;
                    }
                    else
                    {
                        calleeValue = PomeValue(std::monostate{}); // Nil
                    }
                }
            }
            else if (objectValue.isEnvironment())
            { // Checks for MODULE
                // It's a module
                auto module = objectValue.asModule();
                PomeValue key(memberName);
                auto it = module->exports->find(key);
                if (it != module->exports->end())
                    calleeValue = it->second;
                else
                    calleeValue = PomeValue(std::monostate{});
            }
            else if (objectValue.isTable())
            {
                auto table = objectValue.asTable();
                PomeValue key(memberName);
                auto it = table->elements.find(key);
                if (it != table->elements.end())
                    calleeValue = it->second;
                else
                    calleeValue = PomeValue(std::monostate{});
            }
            else
            {
                // Other types don't have members yet
                calleeValue = PomeValue(std::monostate{});
            }
        }
        else
        {
            calleeValue = evaluateExpression(*expr.getCallee());
        }

        if (calleeValue.isClass())
        {
            // Instantiate class
            std::shared_ptr<PomeClass> klass = calleeValue.asClass();
            auto instance = std::make_shared<PomeInstance>(klass);

            // Look for constructor 'init'
            auto initMethod = klass->findMethod("init");
            if (initMethod)
            {
                std::vector<PomeValue> args;
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
            throw RuntimeError("Attempt to call a non-function value.", expr.getLine(), expr.getColumn());
        }

        std::vector<PomeValue> args;
        for (const auto &argExpr : expr.getArgs())
        {
            args.push_back(evaluateExpression(*argExpr));
        }

        if (calleeValue.isNativeFunction())
        {
            std::shared_ptr<NativeFunction> nativeFunc = calleeValue.asNativeFunction();
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
            std::shared_ptr<PomeFunction> pomeFunc = calleeValue.asPomeFunction();
            // thisValue is set if isMethodCall is true
            std::shared_ptr<PomeInstance> thisInstance = nullptr;
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
                // Rethrow with current location if missing
                throw RuntimeError(e.what(), expr.getLine(), expr.getColumn());
            }
        }
    }

    PomeValue Interpreter::callPomeFunction(std::shared_ptr<PomeFunction> pomeFunc, const std::vector<PomeValue> &args, std::shared_ptr<PomeInstance> thisInstance)
    {
        if (args.size() != pomeFunc->parameters.size())
        {
            throw std::runtime_error("Function '" + pomeFunc->name + "' expected " +
                                     std::to_string(pomeFunc->parameters.size()) +
                                     " arguments, but got " + std::to_string(args.size()) + ".");
        }

        std::shared_ptr<Environment> previousEnvironment = currentEnvironment_;
        currentEnvironment_ = std::make_shared<Environment>(pomeFunc->closureEnv);

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

        currentEnvironment_ = previousEnvironment;
        return returnValue;
    }

    void Interpreter::visit(MemberAccessExpr &expr)
    {
        PomeValue objectValue = evaluateExpression(*expr.getObject());
        const std::string &memberName = expr.getMember();

        if (objectValue.isEnvironment())
        { // Module
            auto module = objectValue.asModule();
            PomeValue key(memberName);
            auto it = module->exports->find(key);
            if (it != module->exports->end())
            {
                lastEvaluatedValue_ = it->second;
            }
            else
            {
                std::cerr << "Debug: Available keys in module:" << std::endl;
                for (const auto &pair : *module->exports)
                {
                    std::cerr << " - " << pair.first.toString() << std::endl;
                }
                std::cerr << "Debug: Looking for key: " << key.toString() << std::endl;
                throw RuntimeError("Member '" + memberName + "' not found in module.", expr.getLine(), expr.getColumn());
            }
        }
        else if (objectValue.isTable())
        {
            // Treat member access as string key lookup
            auto table = objectValue.asTable();
            PomeValue key(memberName);
            auto it = table->elements.find(key);
            if (it != table->elements.end())
            {
                lastEvaluatedValue_ = it->second;
            }
            else
            {
                lastEvaluatedValue_ = PomeValue(std::monostate{}); // Nil if key not found
            }
        }
        else if (objectValue.isInstance())
        {
            std::shared_ptr<PomeInstance> instance = objectValue.asInstance();
            // Check fields first
            PomeValue field = instance->get(memberName);
            if (!field.isNil())
            {
                lastEvaluatedValue_ = field;
            }
            else
            {
                // Check class methods
                auto method = instance->klass->findMethod(memberName);
                if (method)
                {
                    lastEvaluatedValue_ = PomeValue(method);
                }
                else
                {
                    lastEvaluatedValue_ = PomeValue(std::monostate{}); // Nil if not found
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
        for (const auto &el : expr.getElements())
        {
            elements.push_back(evaluateExpression(*el));
        }
        // Create PomeList which takes vector by value (move)
        auto pomeList = std::make_shared<PomeList>(std::move(elements));
        lastEvaluatedValue_ = PomeValue(pomeList);
    }

    void Interpreter::visit(TableExpr &expr)
    {
        std::map<PomeValue, PomeValue> elements;
        for (const auto &entry : expr.getEntries())
        {
            PomeValue key = evaluateExpression(*(entry.first));
            PomeValue value = evaluateExpression(*(entry.second));
            elements[key] = value;
        }
        // Create PomeTable
        auto pomeTable = std::make_shared<PomeTable>(std::move(elements));
        lastEvaluatedValue_ = PomeValue(pomeTable);
    }

    void Interpreter::visit(IndexExpr &expr)
    {
        PomeValue objectValue = evaluateExpression(*expr.getObject());
        PomeValue indexValue = evaluateExpression(*expr.getIndex());

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
                return;
            }

            lastEvaluatedValue_ = list->elements[idx];
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

    // --- Visitor methods for statements ---

    void Interpreter::visit(VarDeclStmt &stmt)
    {
        PomeValue value;
        if (stmt.getInitializer())
        {
            value = evaluateExpression(*stmt.getInitializer());
        }
        else
        {
            value = PomeValue(std::monostate{}); // Default to nil if no initializer
        }
        currentEnvironment_->define(stmt.getName(), value);
    }

    void Interpreter::visit(AssignStmt &stmt)
    {
        // Evaluate the right-hand side
        PomeValue value = evaluateExpression(*stmt.getValue());

        // Check target type
        Expression *target = stmt.getTarget();

        if (auto targetId = dynamic_cast<IdentifierExpr *>(target))
        {
            currentEnvironment_->assign(targetId->getName(), value);
        }
        else if (auto targetIndex = dynamic_cast<IndexExpr *>(target))
        {
            // Handle list/table assignment: list[i] = value or table[k] = value
            PomeValue objectValue = evaluateExpression(*targetIndex->getObject());
            PomeValue indexValue = evaluateExpression(*targetIndex->getIndex());

            if (objectValue.isList())
            {
                if (!indexValue.isNumber())
                {
                    throw RuntimeError("List assignment index must be a number.", stmt.getLine(), stmt.getColumn());
                }

                auto list = objectValue.asList();
                double idxDouble = indexValue.asNumber();

                if (idxDouble != static_cast<long long>(idxDouble))
                {
                    throw RuntimeError("List assignment index must be an integer.", stmt.getLine(), stmt.getColumn());
                }
                long long idx = static_cast<long long>(idxDouble);

                if (idx < 0 || idx >= static_cast<long long>(list->elements.size()))
                {
                    throw RuntimeError("List assignment index out of bounds.", stmt.getLine(), stmt.getColumn());
                }

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
            // Handle table member assignment: table.key = value
            PomeValue objectValue = evaluateExpression(*targetMember->getObject());
            if (objectValue.isTable())
            {
                auto table = objectValue.asTable();
                PomeValue key(targetMember->getMember());
                table->elements[key] = value;
            }
            else if (objectValue.isInstance())
            {
                std::shared_ptr<PomeInstance> instance = objectValue.asInstance();

                if (targetMember->getMember() == "current")
                {
                    std::cout << "DEBUG: Updating 'current' on Instance Address: "
                              << instance.get() // Print the raw pointer address
                              << " | New Value: " << value.toString() << std::endl;
                }

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

        // Create a new scope for the if/else branches
        std::shared_ptr<Environment> previousEnvironment = currentEnvironment_;
        currentEnvironment_ = std::make_shared<Environment>(currentEnvironment_);

        try
        {
            if (condition.asBool())
            { // Lua-like truthiness
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
            // Propagate return exception up the call stack
            currentEnvironment_ = previousEnvironment; // Restore environment before rethrowing
            throw;
        }

        currentEnvironment_ = previousEnvironment; // Restore previous environment
    }

    void Interpreter::visit(WhileStmt &stmt)
    {
        // Create a new scope for the while loop body
        std::shared_ptr<Environment> previousEnvironment = currentEnvironment_;
        currentEnvironment_ = std::make_shared<Environment>(currentEnvironment_);

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
            // Propagate return exception up the call stack
            currentEnvironment_ = previousEnvironment; // Restore environment before rethrowing
            throw;
        }

        currentEnvironment_ = previousEnvironment; // Restore previous environment
    }

    void Interpreter::visit(ForStmt &stmt)
    {
        // Create a new scope for the for loop (initializer can declare variables)
        std::shared_ptr<Environment> previousEnvironment = currentEnvironment_;
        currentEnvironment_ = std::make_shared<Environment>(currentEnvironment_);

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

                // Create an inner scope for the loop body
                std::shared_ptr<Environment> loopBodyEnv = std::make_shared<Environment>(currentEnvironment_);
                std::shared_ptr<Environment> savedEnv = currentEnvironment_;
                currentEnvironment_ = loopBodyEnv;

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
            // Propagate return exception up the call stack
            currentEnvironment_ = previousEnvironment; // Restore environment before rethrowing
            throw;
        }

        currentEnvironment_ = previousEnvironment; // Restore previous environment
    }

    void Interpreter::visit(ForEachStmt &stmt)
    {
        PomeValue iterableValue = evaluateExpression(*stmt.getIterable());

        // Create a new scope for the loop
        std::shared_ptr<Environment> previousEnvironment = currentEnvironment_;
        currentEnvironment_ = std::make_shared<Environment>(currentEnvironment_);

        try
        {
            if (iterableValue.isList())
            {
                auto list = iterableValue.asList();
                for (const auto &item : list->elements)
                {
                    currentEnvironment_->define(stmt.getVarName(), item);

                    std::shared_ptr<Environment> loopBodyEnv = std::make_shared<Environment>(currentEnvironment_);
                    std::shared_ptr<Environment> savedEnv = currentEnvironment_;
                    currentEnvironment_ = loopBodyEnv;

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
                    // Iterate keys
                    currentEnvironment_->define(stmt.getVarName(), pair.first);

                    std::shared_ptr<Environment> loopBodyEnv = std::make_shared<Environment>(currentEnvironment_);
                    std::shared_ptr<Environment> savedEnv = currentEnvironment_;
                    currentEnvironment_ = loopBodyEnv;

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
                // Iterator protocol
                auto instance = iterableValue.asInstance();
                auto iteratorMethod = instance->klass->findMethod("iterator");
                if (!iteratorMethod)
                {
                    throw RuntimeError("Object is not iterable (no 'iterator' method).", stmt.getLine(), stmt.getColumn());
                }

                // Get iterator object
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
                    // Call next()
                    PomeValue item = callPomeFunction(nextMethod, {}, iteratorInstance);
                    if (item.isNil())
                        break; // End of iteration

                    // Execute body
                    currentEnvironment_->define(stmt.getVarName(), item);

                    std::shared_ptr<Environment> loopBodyEnv = std::make_shared<Environment>(currentEnvironment_);
                    std::shared_ptr<Environment> savedEnv = currentEnvironment_;
                    currentEnvironment_ = loopBodyEnv;

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
            returnValue = PomeValue(std::monostate{}); // Implicit return nil
        }
        throw ReturnException(returnValue); // Throw the custom exception
    }

    void Interpreter::visit(ExpressionStmt &stmt)
    {
        evaluateExpression(*stmt.getExpression());
        // The result of an expression statement is typically discarded
    }

    void Interpreter::visit(FunctionDeclStmt &stmt)
    {
        // When a function declaration is visited, we create a PomeFunction object
        // and store it in the current environment.
        // The PomeFunction needs to capture the current environment for closures.
        auto pomeFunc = std::make_shared<PomeFunction>();
        pomeFunc->name = stmt.getName();
        pomeFunc->parameters = stmt.getParams();
        pomeFunc->body = &(stmt.getBody());         // Store a pointer to the AST body. This implies the AST outlives the interpreter.
                                                    // This is okay as long as AST is built once and kept.
        pomeFunc->closureEnv = currentEnvironment_; // Capture current environment

        currentEnvironment_->define(stmt.getName(), PomeValue(pomeFunc));
    }

    void Interpreter::visit(ClassDeclStmt &stmt)
    {
        auto pomeClass = std::make_shared<PomeClass>(stmt.getName());

        // Process methods
        for (const auto &methodStmt : stmt.getMethods())
        {
            auto pomeFunc = std::make_shared<PomeFunction>();
            pomeFunc->name = methodStmt->getName();
            pomeFunc->parameters = methodStmt->getParams();
            pomeFunc->body = &(methodStmt->getBody());
            pomeFunc->closureEnv = currentEnvironment_; // Capture definition environment

            pomeClass->methods[pomeFunc->name] = pomeFunc;
        }

        currentEnvironment_->define(stmt.getName(), PomeValue(pomeClass));
    }

    void Interpreter::visit(ImportStmt &stmt)
    {
        std::string moduleName = stmt.getModuleName();

        // Load module (if not already loaded) and get its exports
        auto moduleExports = loadModule(moduleName);

        // Define the module object (table of exports) in current environment
        // Wrap the exports map in a PomeModule object
        auto moduleObj = std::make_shared<PomeModule>(moduleExports);
        currentEnvironment_->define(moduleName, PomeValue(moduleObj));
    }

    void Interpreter::visit(FromImportStmt &stmt)
    {
        std::string moduleName = stmt.getModuleName();

        // Load module and get its exports
        auto moduleExports = loadModule(moduleName);

        // For each symbol, look it up in the export table and define in current environment
        for (const auto &symbol : stmt.getSymbols())
        {
            PomeValue key(symbol);
            auto it = moduleExports->find(key);
            if (it != moduleExports->end())
            {
                currentEnvironment_->define(symbol, it->second);
            }
            else
            {
                throw std::runtime_error("Symbol '" + symbol + "' not exported from module '" + moduleName + "'.");
            }
        }
    }

    void Interpreter::visit(ExportStmt &stmt)
    {
        // Execute inner statement (defines variable/function in current scope)
        stmt.getStmt()->accept(*this);

        // Extract name and add to current export table
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
            // Should not happen if parser is correct
            return;
        }

        // Retrieve value from current environment
        try
        {
            PomeValue val = currentEnvironment_->get(name);
            // Add to current export map (top of stack)
            if (!exportStack_.empty())
            {
                PomeValue key(name);
                (*exportStack_.back())[key] = val;
            }
        }
        catch (const std::runtime_error &)
        {
            // Ignore if not found (shouldn't happen)
        }
    }

    std::shared_ptr<std::map<PomeValue, PomeValue>> Interpreter::loadModule(const std::string &moduleName)
    {
        // Check cache
        auto it = loadedModules_.find(moduleName);
        if (it != loadedModules_.end())
        {
            return it->second.exports;
        }

        // Check for built-in modules
        if (moduleName == "math")
        {
            auto exports = StdLib::createMathExports();
            loadedModules_[moduleName] = ModuleCacheEntry{nullptr, nullptr, exports};
            return exports;
        }
        if (moduleName == "io")
        {
            auto exports = StdLib::createIOExports();
            loadedModules_[moduleName] = ModuleCacheEntry{nullptr, nullptr, exports};
            return exports;
        }
        if (moduleName == "string")
        {
            auto exports = StdLib::createStringExports();
            loadedModules_[moduleName] = ModuleCacheEntry{nullptr, nullptr, exports};
            return exports;
        }

        std::string modulePath = resolveModulePath(moduleName);

        // Read module source
        std::ifstream moduleFile(modulePath);
        if (!moduleFile.is_open())
        {
            throw std::runtime_error("Could not open module file: " + modulePath);
        }
        std::stringstream buffer;
        buffer << moduleFile.rdbuf();
        std::string moduleSource = buffer.str();
        moduleFile.close();

        // Create a new Lexer, Parser, and Program for the module
        Lexer moduleLexer(moduleSource);
        Parser moduleParser(moduleLexer);
        std::shared_ptr<Program> moduleProgram = moduleParser.parseProgram();

        // Create a new environment for the module, parented by the current environment
        std::shared_ptr<Environment> moduleEnvironment = std::make_shared<Environment>(currentEnvironment_);

        // Prepare export table
        auto moduleExports = std::make_shared<std::map<PomeValue, PomeValue>>();

        // Save state
        std::shared_ptr<Environment> previousEnvironment = currentEnvironment_;
        currentEnvironment_ = moduleEnvironment;
        exportStack_.push_back(moduleExports);

        try
        {
            moduleProgram->accept(*this);
        }
        catch (const ReturnException &e)
        {
            // Module return logic (optional, but we support exports now)
        }
        catch (const std::runtime_error &e)
        {
            currentEnvironment_ = previousEnvironment;
            exportStack_.pop_back();
            throw;
        }

        // Restore state
        currentEnvironment_ = previousEnvironment;
        exportStack_.pop_back();

        // Cache module
        loadedModules_[moduleName] = ModuleCacheEntry{moduleProgram, moduleEnvironment, moduleExports};

        return moduleExports;
    }

    std::string Interpreter::resolveModulePath(const std::string &moduleName)
    {
        for (const auto &path : searchPaths_)
        {
            // C++17 filesystem would be cleaner, but string manipulation works
            std::string fullPath = path + "/" + moduleName + ".pome";
            if (std::filesystem::exists(fullPath))
            {
                return fullPath;
            }
        }
        throw std::runtime_error("Module '" + moduleName + "' not found in search paths.");
    }

    // --- Program visitor ---

    void Interpreter::visit(Program &program)
    {
        for (const auto &stmt : program.getStatements())
        {
            executeStatement(*stmt);
        }
    }

    // --- Binary and Unary Operation helpers ---

    PomeValue Interpreter::applyBinaryOp(const PomeValue &left, const std::string &op, const PomeValue &right)
    {
        // Operator Overloading
        if (left.isInstance())
        {
            std::shared_ptr<PomeInstance> instance = left.asInstance();
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

        // Number operations
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

        // String concatenation (using + operator, like Lua for numbers implicitly converts to string)
        if (op == "+" && left.isString() && right.isString())
        {
            return PomeValue(left.asString() + right.asString());
        }
        // Equality for other types
        if (op == "==")
            return PomeValue(left == right);
        if (op == "!=")
            return PomeValue(left != right);

        throw std::runtime_error("Unsupported binary operation '" + op + "' between " +
                                 left.toString() + " and " + right.toString());
    }

    PomeValue Interpreter::applyUnaryOp(const std::string &op, const PomeValue &operand)
    {
        // Operator Overloading
        if (operand.isInstance())
        {
            std::shared_ptr<PomeInstance> instance = operand.asInstance();
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

        // Operator Overloading
        if (operand.isInstance())
        {
            std::shared_ptr<PomeInstance> instance = operand.asInstance();
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