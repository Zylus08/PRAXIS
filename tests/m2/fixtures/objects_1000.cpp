#include <iostream>
#include <string>
int created=0;int destroyed=0;struct G {G(){++created;}~G(){++destroyed;}};int main(){for(int i=0;i<1000;++i){G g;}std::cout<<created<<":"<<destroyed;}