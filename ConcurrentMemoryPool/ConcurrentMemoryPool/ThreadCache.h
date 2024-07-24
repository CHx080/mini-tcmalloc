#pragma once
#include "PageCache.h"
#include "CentralCache.h"

class ThreadCache
{
private:
	FreeList _freelists[NUM_LIST];	//每一个线程都拥有一组自由链表
	
public:
	ThreadCache() {}
	void* Allocate(size_t bytes); //申请空间
	void Deallocate(void* p);	//释放空间
private:
	void* SolveListTooLong(FreeList& freelist, size_t n);	//指定某自由链表归还n块空间
	void* FetchFromCentralCache(size_t index, size_t bytes); //bytes为经过对齐后的字节数
};

static __declspec(thread) ThreadCache* TLSthreadcache = nullptr; //声明每一个线程独占一个threadcache