#include <windows.h>
#include <LzmaLib.h>
#include <memoryModule.h>

EXTERN_C unsigned int g_size;
EXTERN_C unsigned int g_data_size;
EXTERN_C unsigned int g_props_size;
EXTERN_C unsigned char g_props[];
EXTERN_C unsigned char g_data[];

class MyMemoryModule;

struct Files
{
    char *name;
    void *data;
    size_t size;
    MyMemoryModule *mm;
} *files = nullptr;
size_t filesCount = 0;

bool Load(size_t i);

class MyMemoryModule : public MemoryModule
{
    public:
        virtual Lib *LoadLibrary(const char *name);
};

class MyLib2 : public Lib
{
    public:
        MyLib2(MyMemoryModule *h) : h(h) {}
        virtual ~MyLib2(){}
        virtual void *GetProcAddress(const char *fn)
        {
            return h->GetProcAddress(fn);
        }
    private:
        MyMemoryModule *h;
};

Lib *MyMemoryModule::LoadLibrary(const char *name)
{
    for(size_t i = 0; i < filesCount; i++)
    {
        if(!lstrcmpiA(name, files[i].name))
        {
            if(!::Load(i))
                return nullptr;
            return new MyLib2(files[i].mm);
        }
    }
    return MemoryModule::LoadLibrary(name);
}

bool Load(size_t i)
{
    if(i >= filesCount)
        return false;

    if(files[i].mm)
        return true;

    files[i].mm = new MyMemoryModule;
    return files[i].mm->Load(files[i].data, files[i].size);
}

EXTERN_C int Main()
{
    is_logout = true;
    size_t size = g_size;
    HANDLE hHeap = GetProcessHeap();
    void *data = HeapAlloc(hHeap, 0, size + 1);
    int n;
    ((char*)data)[size] = 0;
    if(size < 1)
        return 0;
    n = LzmaUncompress((PBYTE)data, &size, g_data, &g_data_size, g_props, g_props_size);
    if(n)
    {
        char buf[0x100];
        wsprintfA(buf, "n: %d", n);
        MessageBoxA(NULL, buf, "LzmaUncompress", MB_ICONERROR);
        return 1;
    }
    if(size != g_size)
    {
        MessageBoxA(NULL, "size error", NULL, MB_ICONERROR);
    }
    filesCount = *(unsigned long long*)data;
    files = (Files*)HeapAlloc(hHeap, 0, sizeof(Files) * filesCount);
    char *p = (char*)data;
    size_t cur = sizeof(unsigned long long);
    for(size_t i = 0; i < filesCount; i++)
    {
        if(is_logout)
        {
            char buf[0x100];
            wsprintfA(buf, "i: %u\n", (unsigned int)i);
            WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf, lstrlenA(buf), NULL, NULL);
        }
        files[i].name = p + cur;
        cur += lstrlenA(files[i].name) + 1;
        if(is_logout)
        {
            char buf[0x100];
            wsprintfA(buf, "\tname: %s\n", files[i].name);
            WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf, lstrlenA(buf), NULL, NULL);
        }
        if(cur + sizeof(unsigned long long) > size)
            return 2;
        files[i].size = *(unsigned long long*)(p + cur);
        cur += sizeof(unsigned long long);
        if(is_logout)
        {
            char buf[0x100];
            wsprintfA(buf, "\tsize: %u\n", (unsigned int)files[i].size);
            WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf, lstrlenA(buf), NULL, NULL);
        }
        if(cur + files[i].size > size)
            return 3;
        files[i].data = p + cur;
        cur += files[i].size;
        files[i].mm = nullptr;
    }
    
    Load(0);
    return 0;
}

void *operator new(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void operator delete(void* p, size_t)
{
    HeapFree(GetProcessHeap(), 0, p);
}
