#pragma once
#include "praxis/ast/ast.hpp"
namespace praxis {
using Substitution=std::map<std::string,Type>;
Type substitute(Type,const Substitution&);
E cloneExpr(const E&,const Substitution&);
S cloneStmt(const S&,const Substitution&);
Decl cloneDecl(const Decl&,const Substitution&);
std::shared_ptr<Function> cloneFunction(const std::shared_ptr<Function>&,const Substitution&);
std::shared_ptr<Class> cloneClass(const std::shared_ptr<Class>&,const Substitution&);
}
