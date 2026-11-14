#pragma once
#include "praxis/ast/substitution.hpp"
#include <map>
#include <set>
namespace praxis {
class Analyzer {
    using F=std::shared_ptr<Function>;
    std::vector<std::map<std::string,Type>> scopes;
    std::map<std::string,std::vector<F>> functions,templates;
    std::map<std::string,std::shared_ptr<Class>> classPatterns;
    std::set<std::string> checked,checking,called,buildingClasses;
    Program* program=nullptr;
    Type result;
    std::string currentClass;
    bool constMethod=false,inFunction=false;
    int loops=0;
    size_t templateLevel=0;
    Limits limits;
    Type expr(const E&);void stmt(const S&,bool=true);void declare(Decl&);
    void bind(const Token&,Type);Type lookup(const Token&);
    void convert(const E&,Type);void condition(const E&);void modifiable(const E&);bool returns(const S&)const;
    Type resolve(Type,const Location&);
    std::shared_ptr<Class> record(Type,const Location&);
    void defineClass(const std::shared_ptr<Class>&);
    void registerFunction(const F&);void checkFunction(const F&);
    F instantiateFunction(const F&,const std::vector<Type>&,const Location&);
    F select(std::vector<F>,const std::vector<E>&,const Location&,const std::vector<Type>& explicitTypes={},const E& object={});
    int rank(Type,const E&)const;
    F constructor(Type,const std::vector<E>&,const Location&);
    void checkAccess(const F&,const Location&);
    void checkDestructor(Type,const Location&);
    void planCopy(const E&,Type);
    void classDeclaration(Decl&);
    Type callExpression(const E&);
    Type memberExpression(const E&);
    std::string signature(const F&)const;
public:
    explicit Analyzer(Limits l={}):limits(l){}
    void check(Program&);
};
}
