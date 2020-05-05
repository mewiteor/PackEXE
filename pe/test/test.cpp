#include<Windows.h>
#include<iostream>
EXTERN_C __declspec(dllimport) int add(int a, int b);
EXTERN_C __declspec(dllimport) int sub(int a, int b);
using namespace std;
int main(int argc, char *argv[])
{
    while(*argv)
        cout<<*argv++<<endl;
    cout<<"1+2="<<add(1,2)<<endl;
    cout<<"1-2="<<sub(1,2)<<endl;
    return 0;
}
