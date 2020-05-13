#include <Windows.h>
#include <tchar.h>
#include <intrin.h>

#include <memoryModule.h>

#define XASSERT(expr, ret, message) \
    do{ \
        if(!(expr)) { \
            MessageBox(NULL, _T(message), _T(__FILE__) _T(": ") _T(__FUNCSIG__), MB_OK | MB_ICONERROR); \
            return ret; \
        } \
    }while(0)

#define OffsetPointer(type, base, offset) ((type)((size_t)(base) + (offset)))

#define AlignValueDown(value, alignment) ((size_t)(value) & ~((size_t)(alignment) - 1))

#define AlignValueUp(value, alignment) ((size_t)(value) + (size_t)(alignment) - 1 & ~((size_t)(alignment) - 1))

EXTERN_C bool is_logout = false;

#define LOGF if(is_logout) do { \
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), __FUNCSIG__, lstrlenA(__FUNCSIG__), NULL, NULL); \
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, NULL, NULL); \
} while(0)

struct LinkNode
{
    LinkNode *next;
    void *data;
    LinkNode(LinkNode *top, void *d) : next(top), data(d) {}
    ~LinkNode()
    {
        if(data)
            HeapFree(GetProcessHeap(), 0, data);
    }
};

class LinkList
{
    public:
        LinkList() : root(nullptr) {}
        ~LinkList()
        {
            while(root)
                Pop();
        }
        void Push(void * d)
        {
            root = new LinkNode(root, d);
        }
        void Pop()
        {
            if(!root)return;
            LinkNode *p = root->next;
            delete p;
            root = p;
        }
    private:
        LinkNode *root;
};

struct NameIdx
{
    const char *name;
    WORD idx;
};

class MemoryModuleImpl
{
public:
	MemoryModuleImpl(MemoryModule &p, const void *data, size_t size)
		: _(p)
		, data(data)
		, size(size)
		, image(nullptr)
		, init(false)
		, exportTables(nullptr)
	{
        LOGF;
	}
	~MemoryModuleImpl()
	{
        LOGF;
		if (init)
		{
			auto f = OffsetPointer(BOOL(__stdcall*)(HINSTANCE, DWORD, LPVOID), image, nt_header->OptionalHeader.AddressOfEntryPoint);
			(*f)((HINSTANCE)image, DLL_PROCESS_DETACH, NULL);
		}
        if(exportTables)
        {
            
            HeapFree(GetProcessHeap(), 0, exportTables);
            exportTables = nullptr;
        }
		if (image)
		{
			VirtualFree(image, 0, MEM_RELEASE);
			image = nullptr;
		}
	}
	bool LoadHeader();
	bool CopySections();
	bool PerformBaseRelocation();
	bool BuildImportTable();
	bool ChangeMemoryAccess();
	bool ExecuteTLS();
	bool CallEntryPoint();
	void *GetProcAddress(const char *funcName);

private:
	MemoryModule &_;
	const void *data;
	size_t size;
	void *image;
    LinkList blockedMemory;
	DWORD pageSize;
	PIMAGE_NT_HEADERS nt_header;
	LinkList modules;
	bool init;
	NameIdx* exportTables;
};

class MyLib : public Lib
{
    public:
        MyLib(HMODULE h) : h(h) {}
        virtual ~MyLib();
        virtual void *GetProcAddress(const char *);
    private:
        HMODULE h;
};

MemoryModule::MemoryModule()
	: impl(nullptr)
{
        LOGF;
}

MemoryModule::~MemoryModule()
{
    LOGF;
	clear();
}

bool MemoryModule::Load(const void * data, size_t size)
{
    LOGF;
	impl = new MemoryModuleImpl(*this, data, size);

	if (!impl)
		return false;

	if (!impl->LoadHeader()) { clear(); return false; }
	if (!impl->CopySections()) { clear(); return false; }
	if (!impl->PerformBaseRelocation()) { clear(); return false; }
	if (!impl->BuildImportTable()) { clear(); return false; }
	if (!impl->ChangeMemoryAccess()) { clear(); return false; }
	if (!impl->ExecuteTLS()) { clear(); return false; }
	if (!impl->CallEntryPoint()) { clear(); return false; }

	return true;
}

void * MemoryModule::GetProcAddress(const char *funcName)
{
    LOGF;
	return impl->GetProcAddress(funcName);
}

Lib * MemoryModule::LoadLibrary(const char *moduleName)
{
    HMODULE h = ::LoadLibraryA(moduleName);
    Lib *r = new MyLib(h);
	return r;
}

void * MyLib::GetProcAddress(const char *funcName)
{
	return ::GetProcAddress(h, funcName);
}

MyLib::~MyLib()
{
	::FreeLibrary(h);
}

Lib::~Lib()
{}

void MemoryModule::clear()
{
    LOGF;
	if (impl)
	{
		delete impl;
		impl = nullptr;
	}
}

#ifdef _WIN64
#define HOST_MACHINE IMAGE_FILE_MACHINE_AMD64
#else
#define HOST_MACHINE IMAGE_FILE_MACHINE_I386
#endif

bool MemoryModuleImpl::LoadHeader()
{
    LOGF;
	// DOS头
	auto dos_header = OffsetPointer(const IMAGE_DOS_HEADER *, data, 0);
	XASSERT(size >= sizeof(*dos_header), false, "dos头大小太小");
	XASSERT(dos_header->e_magic == IMAGE_DOS_SIGNATURE, false, "dos magic不正确");
	XASSERT(dos_header->e_lfanew, false, "没有NT头");

	// NT头
	auto nt_header = OffsetPointer(const IMAGE_NT_HEADERS *, data, dos_header->e_lfanew);
	XASSERT(size >= dos_header->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), false, "nt文件头大小太小");
	XASSERT(nt_header->Signature, false, "nt signature不正确");
	auto & file_header = nt_header->FileHeader;
	XASSERT(file_header.Machine == HOST_MACHINE, false, "32/64位不一致");
	auto & opt_header = nt_header->OptionalHeader;
	XASSERT(OffsetPointer(void*, data, size) >= OffsetPointer(void*,&opt_header,file_header.SizeOfOptionalHeader), false, "nt可选头大小太小");
	XASSERT(size >= opt_header.SizeOfHeaders, false, "头大小太小");
	// !(opt_header.SectionAlignment & 1)

	// 节
	size_t lastSectionEnd = 0;
	auto section = IMAGE_FIRST_SECTION(nt_header);
	for (size_t i = 0; i < file_header.NumberOfSections; i++, section++)
	{
		size_t sectionEnd;
		if (section->SizeOfRawData)
			sectionEnd = section->VirtualAddress + section->SizeOfRawData;
		else
			sectionEnd = section->VirtualAddress + opt_header.SectionAlignment;
		if (lastSectionEnd < sectionEnd)
			lastSectionEnd = sectionEnd;
	}

	// 对齐的大小
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	pageSize = systemInfo.dwPageSize;
	size_t alignedImageSize = AlignValueUp(opt_header.SizeOfImage, pageSize);
	XASSERT(alignedImageSize == AlignValueUp(lastSectionEnd, pageSize), false, "映像大小与节大小不匹配");

	// 分配code
	image = VirtualAlloc((LPVOID)nt_header->OptionalHeader.ImageBase, alignedImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!image)
	{
		image = VirtualAlloc(nullptr, alignedImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		XASSERT(image, false, "内存不足");
	}

#ifdef _WIN64
	// 4GB对齐
	while (((size_t)image >> 32) < (((size_t)image + alignedImageSize) >> 32))
	{
        blockedMemory.Push(image);
		image = VirtualAlloc(nullptr, alignedImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		XASSERT(image, false, "内存不足");
	}
#endif

	// 复制image
	auto header = (PBYTE)VirtualAlloc(image, nt_header->OptionalHeader.SizeOfHeaders, MEM_COMMIT, PAGE_READWRITE);
    __movsb(header, (PBYTE)data, nt_header->OptionalHeader.SizeOfHeaders);
	this->nt_header = OffsetPointer(PIMAGE_NT_HEADERS, header, dos_header->e_lfanew);
	this->nt_header->OptionalHeader.ImageBase = (size_t)image;
	return true;
}

bool MemoryModuleImpl::CopySections()
{
    LOGF;
	auto section = IMAGE_FIRST_SECTION(nt_header);
	const auto count = nt_header->FileHeader.NumberOfSections;
	for (size_t i = 0; i < count; i++, section++)
	{
		PBYTE dest;
		if (!section->SizeOfRawData)
		{
			auto _size = nt_header->OptionalHeader.SectionAlignment;
			if (_size)
			{
				dest = (PBYTE)VirtualAlloc(OffsetPointer(PBYTE, image, section->VirtualAddress), _size, MEM_COMMIT, PAGE_READWRITE);
				XASSERT(dest, false, "内存不足");
				dest = OffsetPointer(PBYTE, image, section->VirtualAddress);
				section->Misc.PhysicalAddress = (size_t)dest & 0xFFFFFFFF;
                __stosb(dest, 0, _size);
			}

			continue;
		}
		XASSERT(size >= section->PointerToRawData + section->SizeOfRawData, false, "节大小太小");
		dest = (PBYTE)VirtualAlloc(OffsetPointer(PBYTE, image, section->VirtualAddress), section->SizeOfRawData, MEM_COMMIT, PAGE_READWRITE);
		XASSERT(dest, false, "内存不足");
		dest = OffsetPointer(PBYTE, image, section->VirtualAddress);
		__movsb(dest, (PBYTE)data + section->PointerToRawData, section->SizeOfRawData);
		section->Misc.PhysicalAddress = (size_t)dest & 0xFFFFFFFF;
	}
	return true;
}

bool MemoryModuleImpl::PerformBaseRelocation()
{
    LOGF;
	auto dos_header = OffsetPointer(const IMAGE_DOS_HEADER *, data, 0);
	auto origin_nt_header = OffsetPointer(const IMAGE_NT_HEADERS *, data, dos_header->e_lfanew);
	ptrdiff_t delta = nt_header->OptionalHeader.ImageBase - origin_nt_header->OptionalHeader.ImageBase;
	if (!delta) return true;
	auto &dir = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	if (!dir.Size) return true;
	auto relocation = OffsetPointer(PIMAGE_BASE_RELOCATION, image, dir.VirtualAddress);
	while (relocation->VirtualAddress > 0)
	{
		auto dest = OffsetPointer(PBYTE, image, relocation->VirtualAddress);
		auto relInfo = OffsetPointer(PWORD, relocation, sizeof(IMAGE_BASE_RELOCATION));
		for (size_t i = (relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2; i --> 0; relInfo++)
		{
			switch (*relInfo >> 12)
			{
			case IMAGE_REL_BASED_ABSOLUTE:
				break;
			case IMAGE_REL_BASED_HIGHLOW:
			{
				*OffsetPointer(LPDWORD, dest, *relInfo & 0xFFF) += (DWORD)delta;
				break;
			}
#ifdef _WIN64
			case IMAGE_REL_BASED_DIR64:
			{
				*OffsetPointer(ULONGLONG*, dest, *relInfo & 0xFFF) += delta;
				break;
			}
#endif
			default:
				XASSERT(false, false, "未知的reloc类型");
			}
		}
		relocation = OffsetPointer(PIMAGE_BASE_RELOCATION, relocation, relocation->SizeOfBlock);
	}
	return true;
}

bool MemoryModuleImpl::BuildImportTable()
{
    LOGF;
	auto &dir = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (!dir.Size) return true;
	auto importDesc = OffsetPointer(PIMAGE_IMPORT_DESCRIPTOR, image, dir.VirtualAddress);
	for (; !IsBadReadPtr(importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR)) && importDesc->Name; importDesc++)
	{
		Lib *handle = _.LoadLibrary(OffsetPointer(const char *, image, importDesc->Name));
		XASSERT(handle, false, "LoadLibrary失败");
        modules.Push(handle);
		auto funcRef = OffsetPointer(FARPROC*, image, importDesc->FirstThunk);
		auto thunkRef = (uintptr_t *)funcRef;
		if (importDesc->OriginalFirstThunk)
			thunkRef = OffsetPointer(uintptr_t*, image, importDesc->OriginalFirstThunk);
		for (; *thunkRef; thunkRef++, funcRef++)
		{
			if (IMAGE_SNAP_BY_ORDINAL(*thunkRef))
				*funcRef = (FARPROC)handle->GetProcAddress((LPCSTR)IMAGE_ORDINAL(*thunkRef));
			else
			{
				auto thunkData = OffsetPointer(PIMAGE_IMPORT_BY_NAME, image, *thunkRef);
				*funcRef = (FARPROC)handle->GetProcAddress((LPCSTR)&thunkData->Name);
			}
			XASSERT(*funcRef, false, "导入函数失败");
		}
	}
	return true;
}

bool MemoryModuleImpl::ChangeMemoryAccess()
{
    LOGF;
	auto section = IMAGE_FIRST_SECTION(nt_header);
	const auto count = nt_header->FileHeader.NumberOfSections;
#ifdef _WIN64
	uintptr_t imageOffset = (uintptr_t)nt_header->OptionalHeader.ImageBase & 0xFFFFFFFF00000000;
#else
	const uintptr_t imageOffset = 0;
#endif
	LPVOID oldSectionAddress = NULL;
	LPVOID oldAlignedAddress = NULL;
	size_t oldRealSectionSize = 0;
	DWORD oldCharacteristics = 0;
	for (size_t i = 0; i <= count; i++, section++)
	{
		LPVOID sectionAddress = NULL;
		LPVOID alignedAddress = NULL;
		size_t realSectionSize = 0;
		if (i < count)
		{
			sectionAddress = (LPVOID)(section->Misc.PhysicalAddress | imageOffset);
			alignedAddress = (LPVOID)AlignValueDown(sectionAddress, pageSize);
			realSectionSize = section->SizeOfRawData;
			if (!realSectionSize)
			{
				if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
					realSectionSize = nt_header->OptionalHeader.SizeOfInitializedData;
				else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
					realSectionSize = nt_header->OptionalHeader.SizeOfUninitializedData;
			}
		}
		if (i)
		{
			if (i < count && (oldAlignedAddress == alignedAddress ||
				OffsetPointer(LPVOID, oldSectionAddress, oldRealSectionSize) > alignedAddress))
			{
				oldCharacteristics =
					((oldCharacteristics | section->Characteristics) & ~IMAGE_SCN_MEM_DISCARDABLE) |
					(oldCharacteristics & section->Characteristics);

				size_t newRealSectionSize = OffsetPointer(size_t, sectionAddress, realSectionSize) - (size_t)oldSectionAddress;
				if (newRealSectionSize > oldRealSectionSize)
					oldRealSectionSize = newRealSectionSize;
				continue;
			}

			if (oldRealSectionSize)
			{
				if (oldCharacteristics & IMAGE_SCN_MEM_DISCARDABLE)
				{
					if (oldSectionAddress == oldAlignedAddress &&
						(i == count || nt_header->OptionalHeader.SectionAlignment == pageSize || !(oldRealSectionSize % pageSize)))
						VirtualFree(oldSectionAddress, oldRealSectionSize, MEM_DECOMMIT);
				}
				else
				{
					static const int flags[2][2][2] = {
						{
							// not executable
							{PAGE_NOACCESS, PAGE_WRITECOPY},
							{PAGE_READONLY, PAGE_READWRITE},
						}, {
							{PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY},
							{PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE},
						}
					};
					int executable = !!(oldCharacteristics & IMAGE_SCN_MEM_EXECUTE);
					int readable = !!(oldCharacteristics & IMAGE_SCN_MEM_READ);
					int writeable = !!(oldCharacteristics & IMAGE_SCN_MEM_WRITE);
					DWORD protect = flags[executable][readable][writeable];
					if (oldCharacteristics & IMAGE_SCN_MEM_NOT_CACHED)
						protect |= PAGE_NOCACHE;
					DWORD dw;
					if (!VirtualProtect(oldSectionAddress, oldRealSectionSize, protect, &dw))
					{
						E("VirtualProtect");
						return false;
					}
				}
			}
		}
		if (i < count)
		{
			oldSectionAddress = sectionAddress;
			oldAlignedAddress = alignedAddress;
			oldRealSectionSize = realSectionSize;
			oldCharacteristics = section->Characteristics;
		}
	}
	return true;
}

bool MemoryModuleImpl::ExecuteTLS()
{
    LOGF;
	auto &dir = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
	if (!dir.VirtualAddress) return true;
	auto tls = OffsetPointer(PIMAGE_TLS_DIRECTORY, image, dir.VirtualAddress);
	auto callback = (PIMAGE_TLS_CALLBACK *)tls->AddressOfCallBacks;
	if (callback)
		while (*callback)
			(*callback++)(image, DLL_PROCESS_ATTACH, NULL);
	return true;
}

bool MemoryModuleImpl::CallEntryPoint()
{
    LOGF;
	if (!nt_header->OptionalHeader.AddressOfEntryPoint)
		return true;
	if (!(nt_header->FileHeader.Characteristics & IMAGE_FILE_DLL))
	{
		auto f = OffsetPointer(int(*)(), image, nt_header->OptionalHeader.AddressOfEntryPoint);
		(*f)();
	}
	else
	{
		auto f = OffsetPointer(BOOL(__stdcall*)(HINSTANCE,DWORD,LPVOID), image, nt_header->OptionalHeader.AddressOfEntryPoint);
		if (!(*f)((HINSTANCE)image, DLL_PROCESS_ATTACH, NULL))
			return false;
		init = true;
	}
	return true;
}

void sort(NameIdx *arr, size_t size)
{
    LOGF;
}

NameIdx *find(NameIdx *arr, size_t size, const char *target)
{
    LOGF;
    for(size_t i = 0; i < size; ++i)
        if(!lstrcmpA(arr[i].name, target))
            return arr + i;
    return nullptr;
}

void * MemoryModuleImpl::GetProcAddress(const char *funcName)
{
    LOGF;
	auto &dir = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	XASSERT(dir.Size, nullptr, "此dll没有导出表");
	auto exports = OffsetPointer(PIMAGE_EXPORT_DIRECTORY, image, dir.VirtualAddress);
	XASSERT(exports->NumberOfNames && exports->NumberOfFunctions, nullptr, "此dll导出表为空");

	WORD idx;
	if (!HIWORD(funcName))
	{ // ordinal value
		XASSERT(LOWORD(funcName) >= exports->Base, nullptr, "索引超出范围");
		idx = WORD(LOWORD(funcName) - exports->Base);
	}
	else
	{
		XASSERT(exports->NumberOfNames, nullptr, "此dll导出表中的名称表为空");
		XASSERT(init, nullptr, "GetProcAddress 未成功初始化!");
		if(!exportTables)
		{
			auto nameRef = OffsetPointer(LPDWORD, image, exports->AddressOfNames);
			auto ordinal = OffsetPointer(LPWORD, image, exports->AddressOfNameOrdinals);
            exportTables = (NameIdx*)HeapAlloc(GetProcessHeap(), 0, sizeof(NameIdx) * exports->NumberOfNames);
            XASSERT(exportTables, false, "内存不足");
			for (size_t i = 0; i < exports->NumberOfNames; i++, nameRef++, ordinal++)
            {
                exportTables[i].name = OffsetPointer(LPCSTR, image, *nameRef);
                exportTables[i].idx = *ordinal;
            }
            sort(exportTables, exports->NumberOfNames);
		}
        auto it = find(exportTables, exports->NumberOfNames, funcName);
		XASSERT(it, nullptr, "无此名称的函数");
		idx = it->idx;
	}
	return OffsetPointer(void*, image, *OffsetPointer(LPDWORD, image, exports->AddressOfFunctions + idx * 4));
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
        MessageBox(NULL, lpDisplayBuf, TEXT("Error"), MB_OK);

    if(lpMsgBuf)
        LocalFree(lpMsgBuf);
    if(lpDisplayBuf)
        LocalFree(lpDisplayBuf);
}
