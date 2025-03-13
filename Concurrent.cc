#include <thread>
#include "ThreadCache.h"
#include <time.h>

void* ConcurrentAlloc(size_t bytes){
	if (bytes > MAX_BYTES){
		//直接向堆区申请空间
		size_t align = SizeMap::RoundUp(bytes);
		return PageCache::GetInstance()->BigAlloc(align);
	}
	else{
		//从内存池中获取
		static ObjectPool<ThreadCache> t_obj;
		if (TLSthreadcache == nullptr) TLSthreadcache = t_obj.New();
		return TLSthreadcache->Allocate(bytes);
	}
}
void ConcurrentFree(void* p){
	TLSthreadcache->Deallocate(p);
}

void test1(){
	size_t size = 8*1024;
	srand(time(0) ^ 1111);
	std::thread t[15];

	int start = clock();
	/*for (int i = 0; i < 15; ++i)
	{
		t[i] = std::thread([&]()->void {
			for (int j = 0; j < 5; ++j)
			{
				free(malloc(1 + rand() % size));
			}
			});
	}
	for (int i = 0; i < 15; ++i)
	{
		t[i].join();
	}*/
	int end = clock();
	//std::cout << end - start << std::endl;


	start = clock();
	for (int i = 0; i < 15; ++i)
	{

		t[i] = std::thread([&]()->void {
			for (int j = 0; j < 50; ++j)
			{
				ConcurrentFree(ConcurrentAlloc(rand()%(size_t)rand()%size+1));
			}
		});
	}
	for (int i = 0; i < 15; ++i)
	{
		t[i].join();
	}
	end = clock();
	std::cout << end - start << std::endl;
}

void alloc(){ ConcurrentFree(ConcurrentAlloc(MAX_BYTES + 1)); }
void test2()
{
	std::thread t[3];
	t[0] = std::thread(alloc);
	t[2] = std::thread(alloc);
	t[1] = std::thread(alloc);
	t[0].join();
	t[2].join();
	t[1].join();
}
int main()
{	
	test1();
	//test2();
	return 0;
}
