#include "praxis/runtime/interpreter.hpp"
#include "praxis/native/bindings.hpp"
#include <functional>
#include <set>
#include <exception>
#include "praxis/common/timing.hpp"
namespace praxis {
namespace {
void expire(const std::shared_ptr<Object> &o) {
    o->alive = false;
    for (auto e : o->elements) expire(e);
    for (auto e : o->members) expire(e);
}
} // namespace
Scope::~Scope() {
    for (auto o : owned)
        expire(o);
}
Scope &Interpreter::current() {
    return frames.empty() ? globals : *frames.back()->scopes.back();
}
void Interpreter::tick(const Location &l) {
    if (++steps > limits.steps && !(cleanupDepth>0&&++cleanupReserve<=10000))
        fail(l, "runtime", "execution step limit exceeded");
}
std::shared_ptr<Object> Interpreter::lookup(const Token &t) {
    if (!frames.empty())
        for (auto it = frames.back()->scopes.rbegin(); it != frames.back()->scopes.rend(); ++it) {
            auto f = (*it)->names.find(t.text);
            if (f != (*it)->names.end())
                return f->second;
        }
    auto f = globals.names.find(t.text);
    if (f != globals.names.end())
        return f->second;
    fail(t.loc, "runtime", "object not yet initialized: " + t.text);
}
std::shared_ptr<Object> Interpreter::allocate(Type t, bool initialized) {
    if(liveObjects>=limits.objects)fail({"<runtime>"},"runtime","live object limit exceeded");
    auto o = std::make_shared<Object>();++liveObjects;++profile.objects;profile.peakObjects=std::max(profile.peakObjects,liveObjects);
    static uint64_t nextId = 1;
    o->id = nextId++;
    o->generation = 1;
    o->type = t;
    o->type.reference = false;
    o->value = zero(t);
    o->initialized = initialized || t.base == Base::String;
    if(t.base==Base::Record && t.pointer == 0){
        auto key=t;key.constant=false;key.reference=false;key.extent=0;
        auto it = classes.find(typeName(key));
        if (it != classes.end()) o->definition = it->second;
        o->initialized=false;
    }
    if (t.extent) {
        Type element = t;
        element.extent = 0;
        try{for (int i = 0; i < t.extent; ++i)o->elements.push_back(allocate(element, initialized));}
        catch(...){auto primary=std::current_exception();try{destroy(o);}catch(...){}std::rethrow_exception(primary);}
        o->initialized = true;
    }
    return o;
}
Value Interpreter::value(const E &e) {
    return read(eval(e), e->token.loc);
}
Evaluation Interpreter::eval(const E &e) {
    if (evalDepth >= 256+(cleanupDepth>0?16:0))
        fail(e->token.loc, "runtime", "evaluation depth limit exceeded");
    ++evalDepth;
    struct DepthExit {
        int &d;
        ~DepthExit() {
            --d;
        }
    } depthExit{evalDepth};
    tick(e->token.loc);
    auto l = e->token.loc;
    switch (e->kind) {
    case Expr::Kind::Literal:
        return {literal(*e), {}};
    case Expr::Kind::Name:
        if (e->op == "nullptr")
            return {{{Base::Nullptr}, Pointer{}}, {}};
        if (e->op == "std::cout")
            return {{{Base::Stream}, std::int64_t{0}}, {}};
        if (e->op == "std::endl")
            return {{{Base::Endl}, std::string("\n")}, {}};
        if(e->implicitMember){auto self=frames.back()->self;requireAlive(self,l);auto it=self->fields.find(e->op);if(it==self->fields.end())fail(l,"runtime","member is not initialized yet: "+e->op);return {{},it->second};}
        return {{}, lookup(e->token)};
    case Expr::Kind::Index: {
        auto a = eval(e->args[0]);
        auto i = std::get<std::int64_t>(value(e->args[1]).data);
        if (a.value.type.pointer > 0) {
            auto ptr = std::get<Pointer>(a.value.data);
            if (ptr.isNull || ptr.targetId == 0) fail(l, "runtime", "null pointer indexing");
            if (!ptr.target || ptr.target->deleted) fail(l, "runtime", "use after free");
            if (!ptr.target->alive) fail(l, "runtime", "indexing expired object");
            int totalOffset = ptr.offset + static_cast<int>(i);
            if (totalOffset < 0 || totalOffset >= static_cast<int>(ptr.target->elements.size()))
                fail(l, "runtime", "pointer index out of bounds");
            return {{}, ptr.target->elements[static_cast<size_t>(totalOffset)]};
        }
        auto obj = a.object;
        if (!obj && a.value.type.base == Base::Record && std::holds_alternative<O>(a.value.data))
            obj = std::get<O>(a.value.data);
        if (obj) {
            if (!obj->alive) fail(l, "runtime", "invalid array lifetime");
            if (i < 0 || i >= static_cast<std::int64_t>(obj->elements.size()))
                fail(l, "runtime", "index out of bounds");
            return {{}, obj->elements[static_cast<size_t>(i)]};
        }
        auto val = read(a, l);
        if (val.type.base == Base::String || val.type.base == Base::Text) {
            const auto &str = std::get<std::string>(val.data);
            if (i < 0 || i >= static_cast<std::int64_t>(str.size()))
                fail(l, "runtime", "string index out of bounds");
            return {{{Base::Char}, static_cast<std::int64_t>(static_cast<signed char>(str[static_cast<size_t>(i)]))}, {}};
        }
        fail(l, "runtime", "invalid index operand");
    }
    case Expr::Kind::Member:{
        auto owner=object(eval(e->args[0]),l);
        if (owner->type.base == Base::Record && owner->type.name == "std::pair") {
            if (e->op == "first") {
                if (owner->elements.empty()) fail(l, "runtime", "uninitialized pair");
                return {{}, owner->elements[0]};
            }
            if (e->op == "second") {
                if (owner->elements.size() < 2) fail(l, "runtime", "uninitialized pair");
                return {{}, owner->elements[1]};
            }
        }
        auto it=owner->fields.find(e->op);
        if(it==owner->fields.end())fail(l,"runtime","member is not initialized yet: "+e->op);
        return {{},it->second};
    }
    case Expr::Kind::Construct:{
        std::vector<Evaluation> args;
        for(auto a:e->args)args.push_back(eval(a));
        if (e->target == "native:pair.$ctor") {
            auto o = allocate(e->type, true);
            o->location = l;
            for (auto &arg : args) {
                auto cell = allocate(arg.value.type, true);
                cell->value = read(arg, l);
                o->elements.push_back(cell);
            }
            o->initialized = true;
            o->complete = true;
            temporaries.push_back(o);
            return {{e->type, o}, {}};
        }
        if (e->target == "native:vector.$ctor") {
            ++profile.containerOperations;
            auto o = allocate(e->type, true);
            o->location = l;
            if (!args.empty()) {
                auto count = std::get<std::int64_t>(read(args[0], l).data);
                Type elemTy = e->type.arguments[0];
                Value initVal = args.size() > 1 ? convertValue(read(args[1], l), elemTy, l) : zero(elemTy);
                for (std::int64_t i = 0; i < count; ++i) {
                    auto cell = allocate(elemTy, true);
                    cell->value = initVal;
                    o->elements.push_back(cell);
                }
            }
            o->initialized = true;
            o->complete = true;
            temporaries.push_back(o);
            return {{e->type, o}, {}};
        }
        auto o=allocate(e->type,false);
        o->location=l;
        o->zeroInitialize=args.empty()&&functions.at(e->target)->special==Function::Special::DefaultConstructor;
        construct(o,e->target,args);
        temporaries.push_back(o);
        return {{e->type,o},{}};
    }
    case Expr::Kind::New: {
        ++profile.dynamicAllocations;
        Type pointee = e->type;
        pointee.pointer--;
        pointee.constant = false;
        pointee.reference = false;
        if (e->op == "[]") {
            auto szVal = value(e->args[0]);
            auto sz = std::get<std::int64_t>(szVal.data);
            if (sz < 0) fail(l, "runtime", "negative array size");
            pointee.extent = static_cast<int>(sz);
            auto o = allocate(pointee, true);
            o->isHeap = true;
            o->location = l;
            Pointer p;
            p.isNull = false;
            p.targetId = o->id;
            p.generation = o->generation;
            p.target = o;
            p.offset = 0;
            return {{e->type, p}, {}};
        }
        auto o = allocate(pointee, false);
        o->isHeap = true;
        o->location = l;
        if (pointee.base == Base::Record) {
            std::vector<Evaluation> args;
            for (auto a : e->args) args.push_back(eval(a));
            construct(o, e->target, args);
        } else {
            if (!e->args.empty()) {
                o->value = convertValue(value(e->args[0]), pointee, l);
            } else {
                o->value = zero(pointee);
            }
            o->initialized = true;
        }
        Pointer p;
        p.isNull = false;
        p.targetId = o->id;
        p.generation = o->generation;
        p.target = o;
        p.offset = 0;
        return {{e->type, p}, {}};
    }
    case Expr::Kind::Delete: {
        auto a = eval(e->args[0]);
        auto val = read(a, l);
        if (val.type.base == Base::Nullptr) return {{zero({Base::Void})}, {}};
        if (!std::holds_alternative<Pointer>(val.data))
            fail(l, "runtime", "delete requires a pointer");
        auto ptr = std::get<Pointer>(val.data);
        if (ptr.isNull || ptr.targetId == 0) return {{zero({Base::Void})}, {}};
        auto target = ptr.target;
        if (!target) return {{zero({Base::Void})}, {}};
        if (target->deleted)
            fail(l, "runtime", "double free detected");
        if (!target->isHeap)
            fail(l, "runtime", "cannot delete non-heap object");
        if (target->generation != ptr.generation)
            fail(l, "runtime", "delete through stale pointer handle");
        target->deleted = true;
        target->generation++;
        ++profile.dynamicDeletions;
        destroy(target);
        return {{zero({Base::Void})}, {}};
    }
    case Expr::Kind::InitList: {
        auto o = allocate(e->type, true);
        o->location = l;
        for (auto a : e->args) {
            auto val = value(a);
            auto cell = allocate(val.type, true);
            cell->value = val;
            o->elements.push_back(cell);
        }
        temporaries.push_back(o);
        return {{e->type, o}, {}};
    }
    case Expr::Kind::Call:{
        if(e->target.starts_with("native:string.")) {
            auto method = e->target.substr(14);
            auto receiver = value(e->args[0]->args[0]);
            const auto &str = std::get<std::string>(receiver.data);
            if (method == "size" || method == "length")
                return {{{Base::Long}, static_cast<std::int64_t>(str.size())}, {}};
            if (method == "empty")
                return {{{Base::Bool}, std::int64_t{str.empty() ? 1 : 0}}, {}};
            if (method == "at") {
                auto idx = std::get<std::int64_t>(value(e->args[1]).data);
                if (idx < 0 || idx >= static_cast<std::int64_t>(str.size()))
                    fail(l, "runtime", "string::at out of range");
                return {{{Base::Char}, static_cast<std::int64_t>(static_cast<signed char>(str[static_cast<size_t>(idx)]))}, {}};
            }
            if (method == "substr") {
                auto pos = std::get<std::int64_t>(value(e->args[1]).data);
                if (pos < 0 || pos > static_cast<std::int64_t>(str.size()))
                    fail(l, "runtime", "string::substr out of range");
                size_t count = std::string::npos;
                if (e->args.size() > 2) {
                    auto cnt = std::get<std::int64_t>(value(e->args[2]).data);
                    if (cnt >= 0) count = static_cast<size_t>(cnt);
                }
                return {{{Base::String}, str.substr(static_cast<size_t>(pos), count)}, {}};
            }
        }
        if (e->target.starts_with("native:vector.")) {
            ++profile.containerOperations;
            auto method = e->target.substr(14);
            auto selfObj = object(eval(e->args[0]->args[0]), l);
            requireAlive(selfObj, l);
            if (method == "size")
                return {{{Base::Long}, static_cast<std::int64_t>(selfObj->elements.size())}, {}};
            if (method == "empty")
                return {{{Base::Bool}, std::int64_t{selfObj->elements.empty() ? 1 : 0}}, {}};
            if (method == "push_back") {
                auto argVal = value(e->args[1]);
                auto cell = allocate(selfObj->type.arguments[0], true);
                if (cell->type.base == Base::Record) {
                    auto a = eval(e->args[1]);
                    auto src = object(a, l);
                    cell = copyObject(src, cell->type, "", l);
                } else {
                    cell->value = convertValue(argVal, cell->type, l);
                }
                selfObj->elements.push_back(cell);
                return {{zero({Base::Void})}, {}};
            }
            if (method == "pop_back") {
                if (selfObj->elements.empty()) fail(l, "runtime", "pop_back on empty vector");
                destroy(selfObj->elements.back());
                selfObj->elements.pop_back();
                return {{zero({Base::Void})}, {}};
            }
            if (method == "clear") {
                for (auto it = selfObj->elements.rbegin(); it != selfObj->elements.rend(); ++it)
                    destroy(*it);
                selfObj->elements.clear();
                return {{zero({Base::Void})}, {}};
            }
            if (method == "front") {
                if (selfObj->elements.empty()) fail(l, "runtime", "front on empty vector");
                return {{}, selfObj->elements.front()};
            }
            if (method == "back") {
                if (selfObj->elements.empty()) fail(l, "runtime", "back on empty vector");
                return {{}, selfObj->elements.back()};
            }
            if (method == "at") {
                auto idx = std::get<std::int64_t>(value(e->args[1]).data);
                if (idx < 0 || idx >= static_cast<std::int64_t>(selfObj->elements.size()))
                    fail(l, "runtime", "vector::at out of range");
                return {{}, selfObj->elements[static_cast<size_t>(idx)]};
            }
            if (method == "begin") {
                Pointer p;
                p.isNull = false;
                p.targetId = selfObj->id;
                p.generation = selfObj->generation;
                p.target = selfObj;
                p.offset = 0;
                return {{e->type, p}, {}};
            }
            if (method == "end") {
                Pointer p;
                p.isNull = false;
                p.targetId = selfObj->id;
                p.generation = selfObj->generation;
                p.target = selfObj;
                p.offset = static_cast<int>(selfObj->elements.size());
                return {{e->type, p}, {}};
            }
            if (method == "reserve") {
                return {{zero({Base::Void})}, {}};
            }
            if (method == "resize") {
                auto newSize = std::get<std::int64_t>(value(e->args[1]).data);
                if (newSize < 0) fail(l, "runtime", "negative vector size");
                size_t targetCount = static_cast<size_t>(newSize);
                Type elemTy = selfObj->type.arguments[0];
                Value defaultVal = e->args.size() > 2 ? convertValue(value(e->args[2]), elemTy, l) : zero(elemTy);
                while (selfObj->elements.size() > targetCount) {
                    destroy(selfObj->elements.back());
                    selfObj->elements.pop_back();
                }
                while (selfObj->elements.size() < targetCount) {
                    auto cell = allocate(elemTy, true);
                    cell->value = defaultVal;
                    selfObj->elements.push_back(cell);
                }
                return {{zero({Base::Void})}, {}};
            }
        }
        if (e->target.starts_with("native:make_pair")) {
            auto o = allocate(e->type, true);
            o->location = l;
            for (size_t i = 1; i < e->args.size(); ++i) {
                auto val = value(e->args[i]);
                auto cell = allocate(val.type, true);
                cell->value = val;
                o->elements.push_back(cell);
            }
            o->initialized = true;
            o->complete = true;
            temporaries.push_back(o);
            return {{e->type, o}, {}};
        }
        if (e->target.starts_with("native:move")) {
            return eval(e->args[1]);
        }
        if (e->target.starts_with("native:swap")) {
            auto a = eval(e->args[1]);
            auto b = eval(e->args[2]);
            auto objA = object(a, l);
            auto objB = object(b, l);
            if (objA->type.base == Base::Record && objA->type.name == "std::vector") {
                std::swap(objA->elements, objB->elements);
            } else {
                std::swap(objA->value, objB->value);
            }
            return {{zero({Base::Void})}, {}};
        }
        if (e->target.starts_with("native:min")) {
            auto a = value(e->args[1]);
            auto b = value(e->args[2]);
            return truth(binary("<", a, b, l)) ? Evaluation{a, {}} : Evaluation{b, {}};
        }
        if (e->target.starts_with("native:max")) {
            auto a = value(e->args[1]);
            auto b = value(e->args[2]);
            return truth(binary(">", a, b, l)) ? Evaluation{a, {}} : Evaluation{b, {}};
        }
        if (e->target.starts_with("native:sort") || e->target.starts_with("native:reverse")) {
            auto p1 = std::get<Pointer>(value(e->args[1]).data);
            auto p2 = std::get<Pointer>(value(e->args[2]).data);
            if (p1.targetId != p2.targetId || !p1.target)
                fail(l, "runtime", "iterators do not refer to the same container");
            auto target = p1.target;
            int start = std::max(0, p1.offset);
            int end = std::min(static_cast<int>(target->elements.size()), p2.offset);
            if (start > end) fail(l, "runtime", "invalid iterator range");
            if (e->target.ends_with("sort")) {
                std::stable_sort(target->elements.begin() + start, target->elements.begin() + end,
                    [&](const O& x, const O& y) {
                        return truth(binary("<", x->value, y->value, l));
                    });
            } else {
                std::reverse(target->elements.begin() + start, target->elements.begin() + end);
            }
            return {{zero({Base::Void})}, {}};
        }
        if (e->target.starts_with("native:find")) {
            auto p1 = std::get<Pointer>(value(e->args[1]).data);
            auto p2 = std::get<Pointer>(value(e->args[2]).data);
            auto val = value(e->args[3]);
            if (p1.targetId != p2.targetId || !p1.target)
                fail(l, "runtime", "iterators do not refer to the same container");
            auto target = p1.target;
            int start = std::max(0, p1.offset);
            int end = std::min(static_cast<int>(target->elements.size()), p2.offset);
            for (int i = start; i < end; ++i) {
                if (truth(binary("==", target->elements[static_cast<size_t>(i)]->value, val, l))) {
                    p1.offset = i;
                    return {{e->type, p1}, {}};
                }
            }
            p1.offset = end;
            return {{e->type, p1}, {}};
        }
        if (e->target.starts_with("native:count")) {
            auto p1 = std::get<Pointer>(value(e->args[1]).data);
            auto p2 = std::get<Pointer>(value(e->args[2]).data);
            auto val = value(e->args[3]);
            if (p1.targetId != p2.targetId || !p1.target)
                fail(l, "runtime", "iterators do not refer to the same container");
            auto target = p1.target;
            int start = std::max(0, p1.offset);
            int end = std::min(static_cast<int>(target->elements.size()), p2.offset);
            std::int64_t count = 0;
            for (int i = start; i < end; ++i) {
                if (truth(binary("==", target->elements[static_cast<size_t>(i)]->value, val, l))) {
                    ++count;
                }
            }
            return {{{Base::Long}, count}, {}};
        }
        if(e->target.starts_with("native:"))return {stringMember(e->target.substr(7),value(e->args[0]->args[0])),{}};
        auto f=functions.at(e->target);O self;
        if(!f->owner.empty()){self=e->implicitMember?frames.back()->self:object(eval(e->args[0]->args[0]),l);requireAlive(self,l);}
        std::vector<Evaluation> args;
        for(size_t i=1;i<e->args.size();++i){auto a=eval(e->args[i]);
            if(f->params[i-1].type.base==Base::Record&&!f->params[i-1].type.reference && f->params[i-1].type.pointer==0){auto source=object(a,l);if(e->args[i]->lvalue){auto copied=copyObject(source,f->params[i-1].type,e->args[i]->copyTarget,l);temporaries.push_back(copied);a={{f->params[i-1].type,copied},{}};}else a={{f->params[i-1].type,source},{}};}
            else if(!f->params[i-1].type.reference){a.value=read(a,l);a.object.reset();}
            args.push_back(a);
        }
        auto v=call(f,args,self);if(v.type.base==Base::Record && v.type.pointer==0)temporaries.push_back(std::get<O>(v.data));return {v,{}};
    }
    case Expr::Kind::Unary:
    case Expr::Kind::Postfix: {
        if (e->op == "&" && e->kind == Expr::Kind::Unary) {
            auto a = eval(e->args[0]);
            auto obj = object(a, l);
            Pointer p;
            p.isNull = false;
            p.targetId = obj->id;
            p.generation = obj->generation;
            p.target = obj;
            p.offset = 0;
            return {{e->type, p}, {}};
        }
        if (e->op == "*" && e->kind == Expr::Kind::Unary) {
            auto a = eval(e->args[0]);
            auto val = read(a, l);
            if (!std::holds_alternative<Pointer>(val.data))
                fail(l, "runtime", "dereference of non-pointer value");
            auto ptr = std::get<Pointer>(val.data);
            if (ptr.isNull || ptr.targetId == 0)
                fail(l, "runtime", "null pointer dereference");
            auto target = ptr.target;
            if (!target || target->deleted)
                fail(l, "runtime", "use after free");
            if (!target->alive)
                fail(l, "runtime", "dereference of expired object");
            if (target->generation != ptr.generation)
                fail(l, "runtime", "dereference through stale pointer handle");
            if (!target->elements.empty()) {
                if (ptr.offset < 0 || ptr.offset >= static_cast<int>(target->elements.size()))
                    fail(l, "runtime", "pointer offset out of bounds");
                return {{}, target->elements[static_cast<size_t>(ptr.offset)]};
            }
            if (ptr.offset != 0)
                fail(l, "runtime", "pointer offset out of bounds");
            return {{}, target};
        }
        auto a = eval(e->args[0]);
        auto v = read(a, l);
        if (e->op == "!")
            return {{{Base::Bool}, std::int64_t{truth(v) ? 0 : 1}}, {}};
        if (e->op == "++" || e->op == "--") {
            if (a.object->type.pointer > 0) {
                auto ptr = std::get<Pointer>(a.object->value.data);
                auto oldPtr = ptr;
                ptr.offset += (e->op == "++" ? 1 : -1);
                a.object->value = {a.object->type, ptr};
                if (e->kind == Expr::Kind::Postfix)
                    return {{a.object->type, oldPtr}, {}};
                return a;
            }
            auto next = binary(e->op == "++" ? "+" : "-", v, {{Base::Int}, std::int64_t{1}}, l);
            a.object->value = convertValue(next, a.object->type, l);
            if (e->kind == Expr::Kind::Postfix)
                return {v, {}};
            return a;
        }
        if (e->op == "+")
            return {convertValue(v, e->type, l), {}};
        if (e->type.base == Base::Float || e->type.base == Base::Double)
            return {convertValue({e->type, -std::get<double>(v.data)}, e->type, l), {}};
        return {binary("-", zero(e->type), v, l), {}};
    }
    case Expr::Kind::Assign: {
        auto b = value(e->args[1]);
        auto a = eval(e->args[0]);
        if (!a.object || !a.object->alive)
            fail(l, "runtime", "invalid assignment target");
        if(e->type.base==Base::Record && e->type.pointer==0){assignObject(a.object,std::get<O>(b.data),l);return a;}
        if (a.object->type.pointer > 0 && e->op != "=") {
            auto p = std::get<Pointer>(a.object->value.data);
            auto delta = std::get<std::int64_t>(b.data);
            p.offset += (e->op == "+=" ? static_cast<int>(delta) : -static_cast<int>(delta));
            a.object->value = {a.object->type, p};
            a.object->initialized = true;
            return a;
        }
        if (e->op != "=")
            b = binary(e->op.substr(0, 1), read(a, l), b, l);
        a.object->value = convertValue(b, a.object->type, l);
        a.object->initialized = true;
        return a;
    }
    case Expr::Kind::Binary: {
        auto a = value(e->args[0]);
        if (e->op == "&&" && !truth(a))
            return {{{Base::Bool}, std::int64_t{0}}, {}};
        if (e->op == "||" && truth(a))
            return {{{Base::Bool}, std::int64_t{1}}, {}};
        auto b = value(e->args[1]);
        if (e->op == "&&" || e->op == "||")
            return {{{Base::Bool}, std::int64_t{truth(b) ? 1 : 0}}, {}};
        if (e->op == "<<" && a.type.base == Base::Stream) {
            output(out, b);
            if (e->args[1]->kind == Expr::Kind::Name && e->args[1]->op == "std::endl")
                out.flush();
            return {a, {}};
        }
        return {binary(e->op, a, b, l), {}};
    }
    }
    fail(l, "runtime", "unknown expression");
}
void Interpreter::declare(const Decl &d) {
    auto &scope = current();
    if (d.type.reference) {
        auto a = eval(d.init);
        scope.names[d.name.text] = a.object;
        return;
    }
    if (d.type.base == Base::Record && d.type.pointer == 0 && d.type.name == "std::pair") {
        auto o = allocate(d.type, true);
        o->location = d.name.loc;
        if (d.init) {
            auto a = eval(d.init);
            if (a.object) o = copyObject(a.object, d.type, "", d.name.loc);
            else if (std::holds_alternative<O>(a.value.data)) o = copyObject(std::get<O>(a.value.data), d.type, "", d.name.loc);
        } else if (!d.arguments.empty()) {
            for (size_t i = 0; i < d.arguments.size() && i < d.type.arguments.size(); ++i) {
                auto cell = allocate(d.type.arguments[i], true);
                cell->value = convertValue(value(d.arguments[i]), d.type.arguments[i], d.name.loc);
                o->elements.push_back(cell);
            }
        }
        o->initialized = true;
        o->complete = true;
        scope.names[d.name.text] = o;
        scope.owned.push_back(o);
        return;
    }
    if (d.type.base == Base::Record && d.type.pointer == 0 && (d.type.name == "std::vector" || d.type.name == "std::array")) {
        ++profile.containerOperations;
        auto o = allocate(d.type, true);
        o->location = d.name.loc;
        if (d.braced || !d.elements.empty()) {
            Type elemTy = d.type.arguments[0];
            for (auto el : d.elements) {
                auto cell = allocate(elemTy, true);
                if (elemTy.base == Base::Record) {
                    auto a = eval(el);
                    auto src = object(a, d.name.loc);
                    cell = copyObject(src, elemTy, "", d.name.loc);
                } else {
                    cell->value = convertValue(value(el), elemTy, d.name.loc);
                }
                o->elements.push_back(cell);
            }
        } else if (d.init) {
            auto a = eval(d.init);
            if (a.object) o = copyObject(a.object, d.type, "", d.name.loc);
            else if (std::holds_alternative<O>(a.value.data)) o = copyObject(std::get<O>(a.value.data), d.type, "", d.name.loc);
        } else if (!d.arguments.empty()) {
            auto count = std::get<std::int64_t>(value(d.arguments[0]).data);
            Type elemTy = d.type.arguments[0];
            Value initVal = d.arguments.size() > 1 ? convertValue(value(d.arguments[1]), elemTy, d.name.loc) : zero(elemTy);
            for (std::int64_t i = 0; i < count; ++i) {
                auto cell = allocate(elemTy, true);
                cell->value = initVal;
                o->elements.push_back(cell);
            }
        }
        o->initialized = true;
        o->complete = true;
        scope.names[d.name.text] = o;
        scope.owned.push_back(o);
        return;
    }
    if(d.type.base==Base::Record && d.type.pointer == 0){O o;
        if(d.init){auto a=eval(d.init);auto source=object(a,d.name.loc);if(!d.init->lvalue){o=source;detachTemporary(o);o->type=d.type;}else o=copyObject(source,d.type,d.constructor,d.name.loc);}
        else {std::vector<Evaluation> args;for(auto e:d.arguments)args.push_back(eval(e));o=allocate(d.type,false);o->location=d.name.loc;o->zeroInitialize=frames.empty();construct(o,d.constructor,args);}
        scope.names[d.name.text]=o;scope.owned.push_back(o);return;
    }
    auto prior = scope.names.find(d.name.text);
    auto o = frames.empty() && prior != scope.names.end() ? prior->second
                                                          : allocate(d.type, frames.empty());
    if (prior == scope.names.end()) {
        scope.owned.push_back(o);
        scope.names[d.name.text] = o;
    }
    if (d.type.extent) {
        if (d.braced) {
            for (size_t i = 0; i < o->elements.size(); ++i) {
                auto cell = o->elements[i];
                if (i < d.elements.size())
                    cell->value = convertValue(value(d.elements[i]), cell->type, d.name.loc);
                cell->initialized = true;
            }
        }
    } else if (d.init) {
        o->value = convertValue(value(d.init), d.type, d.name.loc);
        o->initialized = true;
    }
}
Flow Interpreter::nested(const S& s){
    auto frame=frames.back().get();frame->scopes.push_back(std::make_unique<Scope>());
    Flow flow;try{flow=execute(s,false);cleanup(*frame->scopes.back());frame->scopes.pop_back();return flow;}
    catch(...){auto primary=std::current_exception();try{cleanup(*frame->scopes.back());}catch(...){}frame->scopes.pop_back();try{discardReturn(flow);}catch(...){}std::rethrow_exception(primary);}
}
Flow Interpreter::execute(const S& s,bool scoped){size_t mark=temporaries.size();Flow flow;try{flow=executeInner(s,scoped);cleanupTemporaries(mark);return flow;}catch(...){auto primary=std::current_exception();try{cleanupTemporaries(mark);}catch(...){}try{discardReturn(flow);}catch(...){}std::rethrow_exception(primary);}}
Flow Interpreter::executeInner(const S& s,bool scoped){
    tick(s->token.loc);
    switch (s->kind) {
    case Stmt::Kind::Block: {
        if (scoped)
            return nested(s);
        for (auto c : s->children) {
            auto f = execute(c);
            if (f.kind != Flow::Normal)
                return f;
        }
        break;
    }
    case Stmt::Kind::Declaration:
        declare(s->decl);
        break;
    case Stmt::Kind::RangeFor: {
        auto frame = frames.back().get();
        frame->scopes.push_back(std::make_unique<Scope>());
        Flow result;
        try {
            auto rangeEval = eval(s->expr);
            auto rangeObj = rangeEval.object;
            if (!rangeObj && rangeEval.value.type.base == Base::Record && std::holds_alternative<O>(rangeEval.value.data))
                rangeObj = std::get<O>(rangeEval.value.data);

            if (rangeObj && (rangeObj->type.extent > 0 || (rangeObj->type.base == Base::Record && (rangeObj->type.name == "std::vector" || rangeObj->type.name == "std::array")))) {
                for (size_t i = 0; i < rangeObj->elements.size(); ++i) {
                    auto elemObj = rangeObj->elements[i];
                    auto iterScope = std::make_unique<Scope>();
                    if (s->decl.type.reference) {
                        iterScope->names[s->decl.name.text] = elemObj;
                    } else if (s->decl.type.base == Base::Record) {
                        auto copied = copyObject(elemObj, s->decl.type, "", s->decl.name.loc);
                        iterScope->owned.push_back(copied);
                        iterScope->names[s->decl.name.text] = copied;
                    } else {
                        auto var = allocate(s->decl.type, true);
                        var->value = convertValue(read({{}, elemObj}, s->decl.name.loc), s->decl.type, s->decl.name.loc);
                        iterScope->owned.push_back(var);
                        iterScope->names[s->decl.name.text] = var;
                    }
                    frame->scopes.push_back(std::move(iterScope));
                    auto f = execute(s->children[0], false);
                    cleanup(*frame->scopes.back());
                    frame->scopes.pop_back();
                    if (f.kind == Flow::Return) {
                        result = f;
                        break;
                    }
                    if (f.kind == Flow::Break)
                        break;
                }
            } else {
                auto val = read(rangeEval, s->token.loc);
                const auto &str = std::get<std::string>(val.data);
                for (size_t i = 0; i < str.size(); ++i) {
                    auto iterScope = std::make_unique<Scope>();
                    auto var = allocate(s->decl.type, true);
                    var->value = Value{{Base::Char}, static_cast<std::int64_t>(static_cast<signed char>(str[i]))};
                    iterScope->owned.push_back(var);
                    iterScope->names[s->decl.name.text] = var;
                    frame->scopes.push_back(std::move(iterScope));
                    auto f = execute(s->children[0], false);
                    cleanup(*frame->scopes.back());
                    frame->scopes.pop_back();
                    if (f.kind == Flow::Return) {
                        result = f;
                        break;
                    }
                    if (f.kind == Flow::Break)
                        break;
                }
            }
            cleanup(*frame->scopes.back());
            frame->scopes.pop_back();
            return result;
        } catch (...) {
            auto primary = std::current_exception();
            try { cleanup(*frame->scopes.back()); } catch (...) {}
            frame->scopes.pop_back();
            try { discardReturn(result); } catch (...) {}
            std::rethrow_exception(primary);
        }
    }
    case Stmt::Kind::Expression:
        eval(s->expr);
        break;
    case Stmt::Kind::If:
        if (test(s->condition))
            return nested(s->children[0]);
        else if (s->children.size() > 1)
            return nested(s->children[1]);
        break;
    case Stmt::Kind::While:
    case Stmt::Kind::Do: {
        bool first = s->kind == Stmt::Kind::Do;
        while (first || test(s->condition)) {
            first = false;
            auto f = nested(s->children[0]);
            if (f.kind == Flow::Return)
                return f;
            if (f.kind == Flow::Break)
                break;
        }
        break;
    }
    case Stmt::Kind::For:{
        auto frame=frames.back().get();frame->scopes.push_back(std::make_unique<Scope>());Flow result;
        try{execute(s->children[0]);while(!s->condition||test(s->condition)){auto f=nested(s->children[1]);if(f.kind==Flow::Return){result=f;break;}if(f.kind==Flow::Break)break;if(s->step)discard(s->step);tick(s->token.loc);}cleanup(*frame->scopes.back());frame->scopes.pop_back();return result;}
        catch(...){auto primary=std::current_exception();try{cleanup(*frame->scopes.back());}catch(...){}frame->scopes.pop_back();try{discardReturn(result);}catch(...){}std::rethrow_exception(primary);}
    }
    case Stmt::Kind::Return:{
        if(!s->expr)return {Flow::Return,zero({Base::Void})};
        if(s->expr->type.base==Base::Record && s->expr->type.pointer==0){auto source=object(eval(s->expr),s->token.loc);O returned;
            if(s->expr->lvalue)returned=copyObject(source,frames.back()->function->result,s->expr->copyTarget,s->token.loc);
            else {returned=source;detachTemporary(returned);}
            return {Flow::Return,{frames.back()->function->result,returned}};
        }
        return {Flow::Return,value(s->expr)};
    }
    case Stmt::Kind::Break:
        return {Flow::Break, {}};
    case Stmt::Kind::Continue:
        return {Flow::Continue, {}};
    case Stmt::Kind::Empty:
        break;
    }
    return {};
}
Value Interpreter::call(const std::shared_ptr<Function>& f,const std::vector<Evaluation>& args,O self){
    if(frames.size()>=limits.calls+(cleanupDepth>0?16:0))fail(f->name.loc,"runtime","call depth limit exceeded");
    Timer timer(profile.callNanoseconds);++profile.calls;auto frame=std::make_unique<Frame>();frame->self=self;frame->function=f;frame->scopes.push_back(std::make_unique<Scope>());frames.push_back(std::move(frame));auto active=frames.back().get();Value returned=zero(f->result);
    try{
        for(size_t i=0;i<args.size();++i){auto& d=f->params[i];
            if(d.type.reference){auto o=object(args[i],d.name.loc);current().names[d.name.text]=o;}
            else if(d.type.base==Base::Record && d.type.pointer==0){auto o=object(args[i],d.name.loc);if(args[i].object)o=copyObject(o,d.type,"",d.name.loc);else detachTemporary(o);current().names[d.name.text]=o;current().owned.push_back(o);}
            else{auto converted=convertValue(read(args[i],d.name.loc),d.type,d.name.loc);auto o=allocate(d.type,true);o->value=converted;current().owned.push_back(o);current().names[d.name.text]=o;}
        }
        if(f->constructor)initializeMembers(self,f,args);
        auto flow=f->body?execute(f->body,false):Flow{};
        if(flow.kind==Flow::Return&&f->result.base!=Base::Void)returned=f->result.base==Base::Record && f->result.pointer==0?flow.value:convertValue(flow.value,f->result,f->name.loc);
        cleanup(*active->scopes.back());frames.pop_back();return returned;
    }catch(...){auto primary=std::current_exception();try{cleanup(*active->scopes.back());}catch(...){}frames.pop_back();if(returned.type.base==Base::Record&&returned.type.pointer==0&&std::holds_alternative<O>(returned.data))try{destroy(std::get<O>(returned.data));}catch(...){}std::rethrow_exception(primary);}
}
int Interpreter::run(const Program &p, size_t start) {
    resetBudget();
    functions=p.compiledFunctions;classes=p.compiledClasses;
    try{
    for (size_t i = start; i < p.items.size(); ++i) {
        auto s = p.items[i].statement;
        if (s && s->kind == Stmt::Kind::Declaration && !s->decl.type.reference && (s->decl.type.base!=Base::Record || s->decl.type.pointer > 0)) {
            auto o = allocate(s->decl.type, true);
            globals.names[s->decl.name.text] = o;
            globals.owned.push_back(o);
        }
    }
    // Constant initialization precedes all dynamic global initializers.
    std::set<std::string> constantObjects, initializedEarly;
    std::function<bool(const E &)> constantExpression = [&](const E &e) {
        if (e->kind == Expr::Kind::Literal)
            return numeric(e->type);
        if (e->kind == Expr::Kind::Name)
            return constantObjects.contains(e->op);
        if (e->kind == Expr::Kind::Unary)
            return (e->op == "+" || e->op == "-" || e->op == "!") && constantExpression(e->args[0]);
        if (e->kind != Expr::Kind::Binary || !numeric(e->type) || !constantExpression(e->args[0]))
            return false;
        if (e->op == "&&" && !truth(value(e->args[0])))
            return true;
        if (e->op == "||" && truth(value(e->args[0])))
            return true;
        return constantExpression(e->args[1]);
    };
    for (size_t i = start; i < p.items.size(); ++i) {
        auto s = p.items[i].statement;
        if (!s || s->kind != Stmt::Kind::Declaration)
            continue;
        auto &d = s->decl;
        bool early = false;
        if (d.type.reference && (d.type.base!=Base::Record || d.type.pointer > 0))
            early =
                d.init && d.init->kind == Expr::Kind::Name && globals.names.contains(d.init->op);
        else if (numeric(d.type) && d.init)
            early = constantExpression(d.init);
        else if (d.type.extent && d.braced && d.type.base != Base::String) {
            early = true;
            for (auto e : d.elements)
                early = early && constantExpression(e);
        }
        if (early) {
            declare(d);
            initializedEarly.insert(d.name.text);
            if (d.type.constant && !d.type.reference && integral(d.type))
                constantObjects.insert(d.name.text);
        }
    }
    // Script top-level declarations have static storage; temporary statement frames resolve them
    // globally.
    for (size_t i = start; i < p.items.size(); ++i)
        if (p.items[i].statement) {
            auto s = p.items[i].statement;
            if (s->kind == Stmt::Kind::Declaration) {
                if (!initializedEarly.contains(s->decl.name.text))
                    execute(s);
            } else {
                auto f = std::make_unique<Frame>();
                f->scopes.push_back(std::make_unique<Scope>());
                frames.push_back(std::move(f));
                try {
                    execute(s);
                    cleanup(*frames.back()->scopes.back());
                    frames.pop_back();
                } catch (...) {
                    try{cleanup(*frames.back()->scopes.back());}catch(...){}
                    frames.pop_back();
                    throw;
                }
            }
        }
    if (p.script)
        return 0;
    int code=static_cast<int>(std::get<std::int64_t>(call(functions.at("main()"),{}).data));finish();return code;
    }catch(...){auto primary=std::current_exception();if(!p.script)try{finish();}catch(...){}std::rethrow_exception(primary);}
}
} // namespace praxis
