#define _CRT_SECURE_NO_WARNING 1

#include <thread>

#include "ThreadCache.hpp"


void Test1()
{
	if (TLSthreadcache == nullptr)
	{
		TLSthreadcache = new ThreadCache;
	}
	
	void* p1 = TLSthreadcache->Allocate(9);
	void* p2 = TLSthreadcache->Allocate(2);
	void* p3 = TLSthreadcache->Allocate(3);
	void* p4 = TLSthreadcache->Allocate(4);
	void* p5 = TLSthreadcache->Allocate(278);
	void* p6 = TLSthreadcache->Allocate(6);
	void* p7 = TLSthreadcache->Allocate(7);
	void* p8 = TLSthreadcache->Allocate(8);

}


int main()
{
	std::thread t1(Test1);
	std::thread t2(Test1);
	std::thread t3(Test1);
	std::thread t4(Test1);
	std::thread t5(Test1);
	std::thread t6(Test1);
	std::thread t7(Test1);
	std::thread t8(Test1);
	std::thread t9(Test1);
	std::thread t10(Test1);
	
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();
	t7.join();
	t8.join();
	t9.join();
	t10.join();
	
	return 0;
}