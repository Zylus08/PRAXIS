#include <iostream>
#include <string>
struct G {int n;G(int x):n(x){}~G(){std::cout<<n;}};int main(){int i=0;while(i<3){G g(i);++i;if(i<2)continue;break;}}