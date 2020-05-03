#include <windows.h>
#include <compressapi.h>
#include <memoryModule.h>

EXTERN_C unsigned int g_algo;
EXTERN_C unsigned int g_size;
EXTERN_C unsigned char g_data[];

EXTERN_C int Main()
{
    DECOMPRESSOR_HANDLE h;
    if(!CreateDecompressor(g_algo, nullptr, &h))
        return 1;
    SIZE_T size;
    Decompress(h, g_data, g_size, nullptr, 0, &size);
    void *data = HeapAlloc(GetProcessHeap(), 0, size);
    auto b = Decompress(h, g_data, g_size, data, size, &size);
    CloseDecompressor(h);
    if(b)
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
