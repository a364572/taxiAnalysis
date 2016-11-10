#include "PthreadPool.h"
#include <iostream>
class A{
public:
void run(){}
};
using namespace std;
int main()
{
    PthreadPool<A> pool;

    return 0;
}
