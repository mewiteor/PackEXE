#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <memory>
#include <LzmaLib.h>

// 输出错误信息
#ifndef __FUNCTION__
#define __FUNCTION__ ""
#endif
#ifndef __FUNCSIG__
#define __FUNCSIG__ __FUNCTION__
#endif

#define ERR(fun,code) ErrorMessage(_T(__FILE__),__LINE__,_T(__FUNCSIG__),_T(fun),code)
#define E(fun)  do {\
                    DWORD dw=GetLastError();\
                    if(dw)\
                        ERR(fun,dw);\
                }while(0)

void ErrorMessage(LPCTSTR lpszFileName, DWORD dwLine, LPCTSTR lpszParentFunction, LPCTSTR lpszFunction, DWORD dwCode);       // 输出错误信息

using namespace std;
int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        cerr<<"usage: "<<argv[1]<<" output.c input.exe [dependence1.dll] [...]"<<endl;
        return 1;
    }
    if(argc > 3)
    {
        cerr<<"unsupported yet"<<endl;
    }
    size_t size = 0;
    unique_ptr<unsigned char[]> buf;
    {
        FILE *f = fopen(argv[2], "rb");
        if(f)
        {
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fseek(f, 0, SEEK_SET);
            buf = unique_ptr<unsigned char[]>(new unsigned char[size]);
            fread(buf.get(), 1, size, f);
            fclose(f);
        }
    }

    size_t dest_size=size + size / 3 + 128;
    unique_ptr<unsigned char[]> dest(new unsigned char[dest_size]);
    unsigned char props[LZMA_PROPS_SIZE]={0};
    size_t propsSize = LZMA_PROPS_SIZE;
    int n = LzmaCompress(dest.get(), &dest_size, buf.get(), size,
            props, &propsSize, 9, 0, -1,-1,-1,-1,-1);
    if(n != SZ_OK)
    {
        cerr << "LzmaCompress error: " << n << endl;
        return 1;
    }

    {
        FILE *f = fopen(argv[1], "w");
        if(f)
        {
            fprintf(f,
                    "extern unsigned int g_size = %u;\n"
                    "extern unsigned int g_data_size = %u;\n"
                    "extern unsigned int g_props_size = %u;\n"
                    "extern unsigned char g_props[] = {",
                    size, dest_size, propsSize);
            for(size_t i = 0; i < propsSize; i++)
                fprintf(f, !i + ",%hu", (unsigned short)props[i]);
            fputs("};\nextern unsigned char g_data[] = {\n", f);
            for(size_t i = 0; i < dest_size; i++)
                fprintf(f, 2*!i + ",\n%hu", (unsigned short)dest[i]);
            fputs("\n};", f);
            fclose(f);
        }
    }
    return 0;
}

// Retrieve the system error message for the last-error code
void ErrorMessage(LPCTSTR lpszFileName,DWORD dwLine,LPCTSTR lpszParentFunction,LPCTSTR lpszFunction,DWORD dwCode)
{
    // Retrieve the system error message for the last-error code
    LPTSTR lpMsgBuf = NULL;
    LPTSTR lpDisplayBuf = NULL;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS|
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        dwCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
    
    if(lpMsgBuf)
    {
        va_list vas[] =
        {
            (va_list)lpszFileName,
            (va_list)dwLine,
            (va_list)lpszParentFunction,
            (va_list)lpszFunction,
            (va_list)dwCode,
            (va_list)lpMsgBuf
        };
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_ARGUMENT_ARRAY |
            FORMAT_MESSAGE_FROM_STRING,
            _T("Filename:%1%n")
            _T("Line:%2!lu!%n")
            _T("Parent function:%3%n")
            _T("Function:%4%n")
            _T("Code:%5!lu!%n")
            _T("Description:%6"),
            0,
            0,
            (LPTSTR)&lpDisplayBuf,
            0,
            vas);
    }

    // Display the error message and exit the process
    if(lpDisplayBuf)
        cerr<<lpDisplayBuf<<endl;

    if(lpMsgBuf)
        LocalFree(lpMsgBuf);
    if(lpDisplayBuf)
        LocalFree(lpDisplayBuf);
}
