#include <thread>
#include "ThreadCache.h"
#include <time.h>



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
void ConcurrentFree(void* p)
{
	if (PageCache::GetInstance()->_bigmemory.find(p)!=PageCache::GetInstance()->_bigmemory.end())
	{
		PageCache::GetInstance()->BigFree(p);
	}
	else
	{
		TLSthreadcache->Deallocate(p);
	}
}

void test()
{
	//size_t size = 8*1024;
	srand(time(0) ^ 1111);
	std::thread t[15];
	/*int start = clock();
	for (int i = 0; i < 100; ++i)
	{
		t[i] = std::thread([&]()->void {
			for (int j = 0; j < 1000; ++j)
			{
				free(malloc(1 + rand() % size));
			}
			});
	}
	for (int i = 0; i < 100; ++i)
	{
		t[i].join();
	}
	int end = clock();
	std::cout << end - start << std::endl;*/

	for (int i = 0; i < 15; ++i)
	{

		t[i] = std::thread([&]()->void {
			for (int j = 0; j < 5; ++j)
			{
				ConcurrentFree(ConcurrentAlloc(rand()%(size_t)32157+1));
			}
		});
	}
	for (int i = 0; i < 15; ++i)
	{
		t[i].join();
	}
}
int main()
{	
	test();
	return 0;
}