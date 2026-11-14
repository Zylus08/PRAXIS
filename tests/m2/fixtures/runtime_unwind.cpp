#include <iostream>
#include <string>
struct G {int n;G(int v):n(v){}~G(){std::cout<<n;}};int f(){G a(2);int z=0;return 1/z;}int main(){G a(1);f();}