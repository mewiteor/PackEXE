#include <Windows.h>
#include <tchar.h>
#include <compressapi.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <memory>
#include <vector>
#include <string>
#include <iomanip>
#include <cassert>

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
    unique_ptr<char[]> buf;
    {
        ifstream ifs(argv[2], ios::binary);
        size = (size_t)filesystem::file_size(filesystem::path(argv[2]));
        buf = unique_ptr<char[]>(new char[size]);
        ifs.read(buf.get(), size);
    }

    DWORD finalAlgo = 0;
    vector<unsigned char> finalData;
    for(DWORD algo = 2; algo <= 5; algo++)
    {
        // cout<<"algo: "<<algo<<endl;
        COMPRESSOR_HANDLE h = nullptr;
        if(!CreateCompressor(algo, nullptr, &h))
        {
            E("CreateCompressor");
            continue;
        }
        // cout<<"\tCreateCompressor: success"<<endl;
        SIZE_T compSize = 0;
        assert(!Compress(h, buf.get(), size, nullptr, 0, &compSize));
        if(!compSize)
        {
            CloseCompressor(h);
            cerr<<"algo("<<algo<<"): compSize is zero!"<<endl;
            continue;
        }
        // cout<<"\tCompress 1: success"<<endl;
        unique_ptr<unsigned char[]> tmp_buf(new unsigned char[compSize]);
        auto b = Compress(h, buf.get(), size, tmp_buf.get(), compSize, &compSize);
        CloseCompressor(h);
        if(!b)
        {
            cerr<<"algo: "<<algo<<endl;
            E("Compress");
            continue;
        }
        // cout<<"\tCompress 2: success"<<endl;
        if(!finalAlgo || finalData.size() > compSize)
        {
            // cout<<"\tupdate final data"<<endl;
            finalData.clear();
            finalData.insert(finalData.end(), tmp_buf.get(), tmp_buf.get() + compSize);
            finalAlgo = algo;
        }
    }
    if(!finalAlgo)
        return 1;

    ofstream ofs(argv[1]);
    ofs << "extern unsigned int g_algo = " << finalAlgo << ";\n"
        << "extern unsigned int g_size = " << (unsigned int)finalData.size() << ";\n"
        << "extern unsigned char g_data[] = {";
    for(auto it = finalData.begin(); it != finalData.end(); ++it)
    {
        if(it != finalData.begin())
            ofs << ",";
        ofs<<(unsigned int)*it;
    }
    ofs<<"};";
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
