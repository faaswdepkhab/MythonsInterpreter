#include "statement.h"

#include <iostream>
#include <sstream>
#include <utility>
#include <functional>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    closure[var_name] = var_value->Execute(closure, context);
    return closure.at(var_name);
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) {
    var_name = std::move(var);
    var_value = std::move(rv);
}

VariableValue::VariableValue(const std::string& var_name)
    : name{var_name} {
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) {
    if (auto size = dotted_ids.size(); size > 0) {
        name = std::move(dotted_ids.at(0));
        list_ids.resize(size - 1);
        std::move(std::next(dotted_ids.begin()), dotted_ids.end(), list_ids.begin());
    }
}

ObjectHolder VariableValue::Execute(Closure &closure, Context &context) {
    if (closure.count(name)) {
        auto result = closure.at(name);
        if (list_ids.size() > 0) {
            if (auto obj = result.TryAs<runtime::ClassInstance>()) {
                return VariableValue(list_ids).Execute(obj->Fields(), context);
            } else {
                throw std::runtime_error("Variable " + name+ " is not class"s);
            }
        }
        return result;
    } else {
        throw std::runtime_error("Variable "s + name + " not found"s);
    }
}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return std::make_unique<Print>(Print(std::make_unique<VariableValue>(VariableValue(name))));
}

Print::Print(unique_ptr<Statement> argument) {
    args_list.push_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args) {
    args_list = std::move(args);
}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    ObjectHolder t;
    bool first = true;
    for (auto &arg:args_list) {
        if (!first) {
            context.GetOutputStream() << ' ';
        }
        first = false;
        t = arg->Execute(closure, context);
        if (t) {
            t->Print(context.GetOutputStream(), context);
        } else {
            context.GetOutputStream() << "None";
        }
    }
    context.GetOutputStream() << endl;
    
    return t;
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args) {
    object_ = std::move(object);
    method_ = std::move(method);
    args_ = std::move(args);
}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    std::vector<runtime::ObjectHolder> values;
    for (auto &arg:args_) {
        values.push_back(arg->Execute(closure, context));
    }
    auto obj_ptr = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();
    return obj_ptr->Call(method_, values, context);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    runtime::DummyContext context_str;
    std::ostringstream os;
    auto value = argument_->Execute(closure, context);
    if (value) {
            value->Print(os, context_str);
            return ObjectHolder::Own(runtime::String(os.str()));
    }  else {
        return ObjectHolder::Own(runtime::String("None"));
    }  
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    auto lhs_value = lhs_->Execute(closure, context);
    auto rhs_value = rhs_->Execute(closure, context);
    
    auto pNumber1 = lhs_value.TryAs<runtime::Number>();
    auto pNumber2 = rhs_value.TryAs<runtime::Number>();
    if (pNumber1 && pNumber2) {
        auto sum = pNumber1->GetValue() + pNumber2->GetValue();
        return ObjectHolder::Own(runtime::Number(sum));
    }
    
    auto pString1 = lhs_value.TryAs<runtime::String>();
    auto pString2 = rhs_value.TryAs<runtime::String>();
    if (pString1 && pString2) {
        auto sum = pString1->GetValue() + pString2->GetValue();
        return ObjectHolder::Own(runtime::String(sum));
    }
    
    auto obj_ptr = lhs_value.TryAs<runtime::ClassInstance>();
    if (obj_ptr) {
        if (obj_ptr->HasMethod(ADD_METHOD, 1)) {
            return obj_ptr->Call(ADD_METHOD, {rhs_value}, context);
        }
    }
    
    throw std::runtime_error("Add operation. Invalid arguments.");
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    auto lhs_value = lhs_->Execute(closure, context);
    auto rhs_value = rhs_->Execute(closure, context);
    
    auto pNumber1 = lhs_value.TryAs<runtime::Number>();
    auto pNumber2 = rhs_value.TryAs<runtime::Number>();
    if (pNumber1 && pNumber2) {
        auto sub = pNumber1->GetValue() - pNumber2->GetValue();
        return ObjectHolder::Own(runtime::Number(sub));
    }
    
    throw std::runtime_error("Sub operation. Invalid arguments.");
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    auto lhs_value = lhs_->Execute(closure, context);
    auto rhs_value = rhs_->Execute(closure, context);
    
    auto pNumber1 = lhs_value.TryAs<runtime::Number>();
    auto pNumber2 = rhs_value.TryAs<runtime::Number>();
    if (pNumber1 && pNumber2) {
        auto mult = pNumber1->GetValue() * pNumber2->GetValue();
        return ObjectHolder::Own(runtime::Number(mult));
    }
    
    throw std::runtime_error("Mult operation. Invalid arguments.");
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    auto lhs_value = lhs_->Execute(closure, context);
    auto rhs_value = rhs_->Execute(closure, context);
    
    auto pNumber1 = lhs_value.TryAs<runtime::Number>();
    auto pNumber2 = rhs_value.TryAs<runtime::Number>();
    if (pNumber1 && pNumber2) {
        if (pNumber2->GetValue() == 0) {
            throw std::runtime_error("Div operation. Divide by zero.");
        }
        auto div = pNumber1->GetValue() / pNumber2->GetValue();
        return ObjectHolder::Own(runtime::Number(div));
    }
    
    throw std::runtime_error("Div operation. Invalid arguments.");
}
    
ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (auto &stmt:args_list) {
        stmt->Execute(closure, context);;
    }
    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw statement_->Execute(closure, context);
}

ClassDefinition::ClassDefinition(ObjectHolder cls): cls_(cls) {
}

ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
    string name = cls_.TryAs<runtime::Class>()->GetName();
    closure[name] = cls_;
    return runtime::ObjectHolder::Share(*closure.at(name));
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv): object_(object) {
    field_name_ = std::move(field_name);
    rv_ = std::move(rv);

}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    auto obj_ptr = object_.Execute(closure, context).TryAs<runtime::ClassInstance>();
    if (!obj_ptr) {
        throw std::runtime_error("Is not object");
    }
    obj_ptr->Fields()[field_name_] = rv_->Execute(closure, context);
    if (obj_ptr->Fields().at(field_name_)) {
        return ObjectHolder::Share(*obj_ptr->Fields().at(field_name_));
    } else {
        return runtime::ObjectHolder::None();
    }
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body) {
    condition_ = std::move(condition);
    if_body_ = std::move(if_body);
    else_body_ = std::move(else_body);
}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    bool f = runtime::IsTrue(condition_->Execute(closure, context));
    if (f) {
        return if_body_->Execute(closure, context);
    } else {
        if (else_body_) {
            return else_body_->Execute(closure, context);
        } else {
            return ObjectHolder::None();
        }
    }
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    auto lhs_value = lhs_->Execute(closure, context);
    if (runtime::IsTrue(lhs_value)) {
        return ObjectHolder::Own(runtime::Bool(true));
    } else {
        auto rhs_value = rhs_->Execute(closure, context);
        return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(rhs_value)));
    }
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    auto lhs_value = lhs_->Execute(closure, context);
    if (!runtime::IsTrue(lhs_value)) {
        return ObjectHolder::Own(runtime::Bool(false));
    } else {
        auto rhs_value = rhs_->Execute(closure, context);
        return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(rhs_value)));
    }
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    bool f = runtime::IsTrue(argument_->Execute(closure, context));
    return ObjectHolder::Own(runtime::Bool(!f));
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(cmp) {
}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    auto lhs_value = lhs_->Execute(closure, context);
    auto rhs_value = rhs_->Execute(closure, context);
    
    bool f = cmp_(lhs_value, rhs_value, context);
    return ObjectHolder::Own(runtime::Bool(f));
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args): class_def(class_){
    args_list = std::move(args);
}

NewInstance::NewInstance(const runtime::Class& class_): class_def(class_) {
}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    auto obj = ObjectHolder::Own(runtime::ClassInstance(class_def));
    
    if (obj.TryAs<runtime::ClassInstance>()->HasMethod(INIT_METHOD, args_list.size())) {
        std::vector<ObjectHolder> actual_args;
        for (auto &item:args_list) {
            actual_args.push_back(ObjectHolder::Share(*item->Execute(closure, context)));
        }
        obj.TryAs<runtime::ClassInstance>()->Call(INIT_METHOD, actual_args, context);
    }
    return obj;
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body) {
    body_ = std::forward<std::unique_ptr<Statement>>(body);
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
        body_->Execute(closure, context);
        return ObjectHolder::None();
    }  catch (runtime::ObjectHolder &result) {
        return result;
    }  catch (...) {
        throw std::runtime_error("Unknown exception");
    }
}

}  // namespace ast
