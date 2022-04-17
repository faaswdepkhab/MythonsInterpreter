#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <utility>

#include <iostream>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool ObjectHolder::IsNull() const {
    return data_ == nullptr;
}    
    
bool IsTrue(const ObjectHolder& object) {
    auto p_Number = object.TryAs<Number>();
    if (p_Number) {
        return p_Number->GetValue() != 0;
    }
    
    auto p_Bool = object.TryAs<Bool>();
    if (p_Bool) {
        return p_Bool->GetValue();
    }
    
    auto p_String = object.TryAs<String>();
    if (p_String) {
        return p_String->GetValue() != "";
    }
    
    return false;
}

void ClassInstance::Print(std::ostream& os, Context& context) {
    if (HasMethod("__str__"s, 0U)) {
        Call("__str__"s, {}, context)->Print(os, context);
    } else {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    auto method_ptr = cls_.GetMethod(method);
    if (method_ptr) {
        return method_ptr->formal_params.size() == argument_count;
    } else {
        return false;
    }
}

Closure& ClassInstance::Fields() {
    return fields_;
}

const Closure& ClassInstance::Fields() const {
    return fields_;
}

ClassInstance::ClassInstance(const Class& cls):cls_(cls)  {
}

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context) {
    if (HasMethod(method, actual_args.size())) {
        auto method_ptr = cls_.GetMethod(method);
        Closure params;
        
        // копирование в таблицу параметров
        int count = actual_args.size();
        for (int i = 0; i < count; i++) {
            params[method_ptr->formal_params[i]] = actual_args.at(i);
        }
        params["self"] = ObjectHolder::Share(*this);

        return method_ptr->body->Execute(params, context);
    } else {
        throw runtime_error("Method not implemented");
    }
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent) {
    name_ = std::move(name);
    for (auto &item:methods) {
        methods_[item.name] = std::move(item);
    }
    parent_ = parent;
}

const Method* Class::GetMethod(const std::string& name) const {
    auto it = methods_.find(name);
    if (it != methods_.end()) {
        return &(it->second);
    } else if (parent_) {
        return parent_->GetMethod(name);
    } else {
        return nullptr;
    }
}

[[nodiscard]] const std::string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& os, Context& /*context*/) {
    os << "Class " << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    auto class_ptr = lhs.TryAs<ClassInstance>();
    if (class_ptr) {
        if (class_ptr->HasMethod("__eq__", 1)) {
            return IsTrue(class_ptr->Call("__eq__", {rhs}, context));
        }
    }
    
    if ((!lhs.Get()) && (!rhs.Get())) return true;
    
    auto p_Number_1 = lhs.TryAs<Number>();
    auto p_Number_2 = rhs.TryAs<Number>();
    if (p_Number_1 && p_Number_2) {
        return p_Number_1->GetValue() == p_Number_2->GetValue();
    }
    
    auto p_String_1 = lhs.TryAs<String>();
    auto p_String_2 = rhs.TryAs<String>();
    if (p_String_1 && p_String_2) {
        return p_String_1->GetValue() == p_String_2->GetValue();
    }
    
    auto p_Bool_1 = lhs.TryAs<Bool>();
    auto p_Bool_2 = rhs.TryAs<Bool>();
    if (p_Bool_1 && p_Bool_2) {
        return p_Bool_1->GetValue() == p_Bool_2->GetValue();
    }
    
    throw runtime_error("Error comparing");
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    auto class_ptr = lhs.TryAs<ClassInstance>();
    if (class_ptr) {
        if (class_ptr->HasMethod("__lt__", 1)) {
            return IsTrue(class_ptr->Call("__lt__", {rhs}, context));
        }
    }
    auto p_Number_1 = lhs.TryAs<Number>();
    auto p_Number_2 = rhs.TryAs<Number>();
    if (p_Number_1 && p_Number_2) {
        return p_Number_1->GetValue() < p_Number_2->GetValue();
    }
    
    auto p_String_1 = lhs.TryAs<String>();
    auto p_String_2 = rhs.TryAs<String>();
    if (p_String_1 && p_String_2) {
        return p_String_1->GetValue() < p_String_2->GetValue();
    }
    
    auto p_Bool_1 = lhs.TryAs<Bool>();
    auto p_Bool_2 = rhs.TryAs<Bool>();
    if (p_Bool_1 && p_Bool_2) {
        return p_Bool_1->GetValue() < p_Bool_2->GetValue();
    }
    
    throw runtime_error("Error comparing");
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !LessOrEqual(lhs, rhs, context);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context);
}

}  // namespace runtime
