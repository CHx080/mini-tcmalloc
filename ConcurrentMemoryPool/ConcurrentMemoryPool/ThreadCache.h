#pragma once
#include "PageCache.h"
#include "CentralCache.h"

class ThreadCache
{
private:
	FreeList _freelists[NUM_LIST];
	
public:
	ThreadCache() {}
	void* Allocate(size_t bytes);
	void Deallocate(void* p);
private:
	void* SolveListTooLong(FreeList& freelist, size_t n);
	void* FetchFromCentralCache(size_t index, size_t bytes); //bytes为经过对齐后的字节数
};

static __declspec(thread) ThreadCache* TLSthreadcache = nullptr; //声明每一个线程独占一个threadcache