#include <iostream>
#include <string>
struct G {int n;G(int x):n(x){std::cout<<"C"<<n;}~G(){std::cout<<"D"<<n;}};int main(){for(int i=0;i<3;++i){G g(i);if(i==0)continue;if(i==1)break;}}