#include <iostream>
#include <string>
struct G {G(){std::cout<<"G";}~G(){std::cout<<"g";}};struct B {B(G g){int z=0;int x=1/z;}~B(){std::cout<<"b";}};int main(){G g;B b(g);}