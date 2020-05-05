#include<Windows.h>
EXTERN_C __declspec(dllexport) int add(int a, int b)
{
    return a+b;
}
