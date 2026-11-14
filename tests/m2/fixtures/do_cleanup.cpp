#include <iostream>
#include <string>
struct G {int n;G(int x):n(x){}~G(){std::cout<<n;}};int main(){int i=0;do{G g(i);++i;continue;}while(i<3);}