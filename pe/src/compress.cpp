#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <memory>
#include <LzmaLib.h>
#include <string>
#include <vector>
#include <set>
#include<Shlwapi.h>
#include<ImageHlp.h>
#include <sstream>

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

bool GetDlls(vector<string> &dlls, const string & img, const string & path);
vector<string> GetAllDlls(const char *img);
string GetTruePath(const char *img);

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        cerr<<"usage: "<<argv[1]<<" output.c input.exe"<<endl;
        return 1;
    }


    vector<char> buf;

    {
        auto allDlls = GetAllDlls(argv[2]);
        if(allDlls.size() < 1)
            return 1;
        {
            unsigned long long size = allDlls.size();
            const char *ps = (const char *)&size;
            buf.insert(buf.end(), ps, ps + sizeof(size));
        }
        for(const auto & dll : allDlls)
        {
            string s = PathFindFileNameA(dll.c_str());
            // cerr<<"\t\t!s: "<<s<<endl;
            buf.insert(buf.end(), s.begin(), s.end());
            buf.push_back(0);
            FILE *f = fopen(dll.c_str(), "rb");
            if(f)
            {
                fseek(f, 0, SEEK_END);
                unsigned long long size = ftell(f);
                fseek(f, 0, SEEK_SET);
                vector<char> t;
                t.resize(size);
                fread(&t[0], 1, size, f);
                fclose(f);
                const char *ps = (const char *)&size;
                buf.insert(buf.end(), ps, ps + sizeof(size));
                buf.insert(buf.end(), t.begin(), t.end());
            }
        }
    }

    size_t dest_size=buf.size() + buf.size() / 3 + 128;
    unique_ptr<unsigned char[]> dest(new unsigned char[dest_size]);
    unsigned char props[LZMA_PROPS_SIZE]={0};
    size_t propsSize = LZMA_PROPS_SIZE;
    int n = LzmaCompress(dest.get(), &dest_size,
            (const unsigned char*)&buf[0], buf.size(),
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
                    buf.size(), dest_size, propsSize);
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

string GetTruePath(const char *img)
{
    if(!img)
        return "";
    string s;
    HMODULE hModule = LoadLibraryA(img);
    if(hModule)
    {
        char buf[MAX_PATH] = {0};
        if(GetModuleFileNameA(hModule, buf, _countof(buf)))
            s = buf;
        FreeLibrary(hModule);
    }
    if(s.size() < 1)
        s = img;
    return s;
}

vector<string> GetAllDlls(const char *img)
{
	set<string> importSet;

	vector<string> newR;
    string simg = GetTruePath(img);
	newR.push_back(simg);

	vector<string> tmpR;

	auto p = PathFindFileNameA(simg.c_str());
    string path;
	if (p != simg.c_str())
        path = simg.substr(0, p - simg.c_str() - 1);

	while (!newR.empty())
	{
		importSet.insert(newR.begin(), newR.end());
		for (const auto & z : newR)
		{
			vector<string> r;
            if(!GetDlls(r, z,path))
                return vector<string>();
			for (const auto & x : r)
			{
				auto t = importSet.insert(x);
				if (t.second)
				{
					tmpR.push_back(x);
				}
			}
		}
		swap(newR, tmpR);
		tmpR.clear();
	}

    tmpR.push_back(simg);
    for(const auto dll : importSet)
    {
        if(dll == simg)
            continue;
        const char *p = PathFindFileNameA(dll.c_str());
        string ss = dll.substr(0, p - dll.c_str() - 1);
        if(path != ss)
            continue;
        tmpR.push_back(dll);
    }

	return tmpR;
}

bool GetDlls(vector<string> &dlls, const string & img, const string & path)
{
	LOADED_IMAGE li = { 0 };
    bool fLoadSuccess = false;
    // fLoadSuccess = fLoadSuccess || MapAndLoad(img.c_str(), path.c_str(), &li, FALSE, TRUE);
    fLoadSuccess = fLoadSuccess || MapAndLoad(img.c_str(), NULL, &li, FALSE, TRUE);
	if (!fLoadSuccess)
	{
        ostringstream oss;
        oss << "MapAndLoad(" << img << ")";
        E(oss.str().c_str());
		return false;
	}

	struct TMP
	{
		TMP(LOADED_IMAGE & _li) :li(_li) {}
		~TMP()
		{
			UnMapAndLoad(&li);
		}
	private:
		LOADED_IMAGE & li;
	} tmp(li);

	DWORD dwImportStart = li.FileHeader->OptionalHeader.DataDirectory[1].VirtualAddress;
	DWORD dwImportEnd = dwImportStart + li.FileHeader->OptionalHeader.DataDirectory[1].Size;
	PUCHAR pPointerOfData = nullptr;
	ULONG ulCount = 0;

	for (ULONG i = 0; i < li.NumberOfSections; i++)
	{
		if (li.Sections[i].VirtualAddress <= dwImportStart &&
			li.Sections[i].VirtualAddress + li.Sections[i].Misc.VirtualSize >= dwImportEnd)
		{
			pPointerOfData = li.MappedAddress + li.Sections[i].PointerToRawData + (dwImportStart - li.Sections[i].VirtualAddress);
			ulCount++;
		}
	}

	if (0 == ulCount)
	{
		return true;
	}
	else if (1 != ulCount)
	{
        cerr << "ulCount != 1" << endl;
		return false;
	}

	auto GetPoint = [&](DWORD dw) { return pPointerOfData + (dw - dwImportStart); };

	typedef struct _ImportDirectoryTable
	{
		DWORD dwImportLookupTableRVA;
		DWORD dwTimeStamp;
		DWORD dwForwarderChain;
		DWORD dwNameRVA;
		DWORD dwImportAddressTableRVA;
	}ImportDirectoryTable, *PImportDirectoryTable;

	PImportDirectoryTable pImportDirectoryTable = (PImportDirectoryTable)pPointerOfData;

	while (pImportDirectoryTable->dwNameRVA)
	{
        const char *dll = (char*)GetPoint(pImportDirectoryTable->dwNameRVA);
        string s = path + "\\" + dll;
        if(PathFileExistsA(s.c_str()))
            dlls.push_back(s);
        else
            dlls.push_back(GetTruePath(dll));

		pImportDirectoryTable++;
	}

	return true;
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
