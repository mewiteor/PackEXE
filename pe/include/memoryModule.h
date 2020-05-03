#pragma once

class MemoryModuleImpl;

class MemoryModule
{
public:
	MemoryModule();
	~MemoryModule();
	bool Load(const void *data, size_t size);
	void* GetProcAddress(const char *funcName);

	virtual void *OriLoadLibrary(const char *);
	virtual void *OriGetProcAddress(void*, const char *);
	virtual void OriFreeLibrary(void*);
private:
	void clear();
private:
	MemoryModuleImpl *impl;
};

EXTERN_C bool is_logout;

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
