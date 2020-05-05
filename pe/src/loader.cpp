#include <windows.h>
#include <LzmaLib.h>
#include <memoryModule.h>

EXTERN_C unsigned int g_size;
EXTERN_C unsigned int g_data_size;
EXTERN_C unsigned int g_props_size;
EXTERN_C unsigned char g_props[];
EXTERN_C unsigned char g_data[];

EXTERN_C int Main()
{
    size_t size = g_size;
    void *data = HeapAlloc(GetProcessHeap(), 0, size);
    int n = LzmaUncompress((PBYTE)data, &size, g_data, &g_data_size, g_props, g_props_size);
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
    {
        MemoryModule mm;
        mm.Load(data, size);
    }
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
