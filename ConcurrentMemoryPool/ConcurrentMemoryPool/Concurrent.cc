#include <thread>
#include "ThreadCache.hpp"




void* ConcurrentAlloc(size_t bytes)
{
	if (bytes > MAX_BYTES)
	{
		//直接向堆区申请空间
		size_t align = SizeMap::RoundUp(bytes);
		return PageCache::GetInstance()->BigAlloc(align);
	}
	else
	{
		//从内存池中获取
		static ObjectPool<ThreadCache> t_obj;
		if (TLSthreadcache == nullptr) TLSthreadcache = t_obj.New();
		return TLSthreadcache->Allocate(bytes);
	}
}
void ConcurrentFree(void* p,size_t bytes)
{
	if (bytes > MAX_BYTES)
	{
		PageCache::GetInstance()->BigFree(p);
	}
	else
	{
		TLSthreadcache->Deallocate(p, bytes);
	}
}
int main()
{	
	void* p = ConcurrentAlloc(21366);
	void* p1 = ConcurrentAlloc(366);
	void* p2 = ConcurrentAlloc(23666);
	void* p3 = ConcurrentAlloc(21366);
	void* p4 = ConcurrentAlloc(6);
	void* p5 = ConcurrentAlloc(2366);
	void* p6 = ConcurrentAlloc(36);
	void* p7 = ConcurrentAlloc(1366);
	void* p8 = ConcurrentAlloc(266);
	ConcurrentFree(p, 21366);
	void* p0 = ConcurrentAlloc(MAX_BYTES + 1);
	ConcurrentFree(p0, MAX_BYTES + 1);
	ConcurrentFree(p1, 366);
	ConcurrentFree(p2, 23666);
	
	return 0;
}