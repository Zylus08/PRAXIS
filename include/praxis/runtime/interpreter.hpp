#pragma once
#include "praxis/runtime/value.hpp"
#include <map>
#include <ostream>
namespace praxis {
struct Scope {
    std::map<std::string,std::shared_ptr<Object>> names;
    std::vector<std::shared_ptr<Object>> owned;
    ~Scope();
};
struct Frame {
    std::vector<std::unique_ptr<Scope>> scopes;
    std::shared_ptr<Object> self;
    std::shared_ptr<Function> function;
};
struct Flow {enum Kind {Normal,Return,Break,Continue} kind=Normal;Value value;};
class Interpreter {
    using O=std::shared_ptr<Object>;
    Scope globals;
    std::vector<std::unique_ptr<Frame>> frames;
    std::map<std::string,std::shared_ptr<Function>> functions;
    std::map<std::string,std::shared_ptr<Class>> classes;
    std::vector<O> temporaries;
    std::ostream& out;
    std::uint64_t steps=0;
    int evalDepth=0,cleanupDepth=0;
    std::uint64_t cleanupReserve=0;
    size_t liveObjects=0;
    Limits limits;
    Profile profile;
    Scope& current();O lookup(const Token&);O allocate(Type,bool);
    Evaluation eval(const E&);Value value(const E&);
    Flow execute(const S&,bool=true);Flow executeInner(const S&,bool);Flow nested(const S&);
    void declare(const Decl&);
    Value call(const std::shared_ptr<Function>&,const std::vector<Evaluation>&,O self={});
    void tick(const Location&);
    O object(const Evaluation&,const Location&);void requireAlive(const O&,const Location&);
    void construct(const O&,const std::string&,const std::vector<Evaluation>&);
    O copyObject(const O&,Type,const std::string&,const Location&);
    void assignObject(const O&,const O&,const Location&);
    void destroy(const O&);void cleanup(Scope&);void cleanupTemporaries(size_t);void detachTemporary(const O&);
    void discardReturn(Flow&);
    void initializeMembers(const O&,const std::shared_ptr<Function>&,const std::vector<Evaluation>&);
    bool test(const E&);void discard(const E&);
public:
    explicit Interpreter(std::ostream& o,Limits l={}):out(o),limits(l){}
    int run(const Program&,size_t start=0);
    void finish();
    void resetBudget(){steps=0;cleanupReserve=0;}
    const Profile& statistics()const{return profile;}
    size_t liveObjectCount()const{return liveObjects;}
};
}
