#include "praxis/ast/substitution.hpp"
namespace praxis {
Type substitute(Type t,const Substitution& m){
    auto it=m.find(t.name);
    if((t.base==Base::Parameter||(t.base==Base::Record&&t.arguments.empty()))&&it!=m.end()){
        auto r=it->second;r.constant=r.constant||t.constant;r.reference=r.reference||t.reference;r.pointer=r.pointer+t.pointer;r.pointeeConstant=r.pointeeConstant||t.pointeeConstant;r.extent=t.extent;return r;
    }
    for(auto& a:t.arguments)a=substitute(a,m);return t;
}
E cloneExpr(const E& e,const Substitution& m){if(!e)return {};auto c=std::make_shared<Expr>(*e);c->target.clear();c->copyTarget.clear();c->implicitMember=false;c->args.clear();c->type=substitute(e->type,m);for(auto a:e->args)c->args.push_back(cloneExpr(a,m));for(auto& a:c->typeArguments)a=substitute(a,m);if(c->kind==Expr::Kind::Name&&m.contains(c->op))c->type=m.at(c->op);return c;}
Decl cloneDecl(const Decl& d,const Substitution& m){auto c=d;c.type=substitute(d.type,m);c.init=cloneExpr(d.init,m);c.elements.clear();c.arguments.clear();c.constructor.clear();for(auto e:d.elements)c.elements.push_back(cloneExpr(e,m));for(auto e:d.arguments)c.arguments.push_back(cloneExpr(e,m));return c;}
S cloneStmt(const S& s,const Substitution& m){if(!s)return {};auto c=std::make_shared<Stmt>(*s);c->children.clear();for(auto a:s->children)c->children.push_back(cloneStmt(a,m));c->expr=cloneExpr(s->expr,m);c->condition=cloneExpr(s->condition,m);c->step=cloneExpr(s->step,m);c->decl=cloneDecl(s->decl,m);return c;}
std::shared_ptr<Function> cloneFunction(const std::shared_ptr<Function>& f,const Substitution& m){auto c=std::make_shared<Function>(*f);c->id.clear();c->result=substitute(f->result,m);c->params.clear();for(auto d:f->params)c->params.push_back(cloneDecl(d,m));c->body=cloneStmt(f->body,m);c->initializers.clear();for(auto i:f->initializers){MemberInit n;n.name=i.name;for(auto e:i.args)n.args.push_back(cloneExpr(e,m));c->initializers.push_back(n);}return c;}
std::shared_ptr<Class> cloneClass(const std::shared_ptr<Class>& c,const Substitution& m){auto r=std::make_shared<Class>(*c);r->fields.clear();r->methods.clear();for(auto d:c->fields)r->fields.push_back(cloneDecl(d,m));for(auto f:c->methods)r->methods.push_back(cloneFunction(f,m));return r;}
}
