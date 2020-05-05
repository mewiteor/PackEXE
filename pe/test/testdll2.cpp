#include<Windows.h>
EXTERN_C __declspec(dllexport) int sub(int a, int b)
{
    return a-b;
}
