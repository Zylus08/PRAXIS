#include <iostream>
#include <string>
struct G {int n;G(int v):n(v){std::cout<<n;}~G(){std::cout<<n;}};int f(int n){G g(n);if(n==0)return 0;return f(n-1);}int main(){f(3);}