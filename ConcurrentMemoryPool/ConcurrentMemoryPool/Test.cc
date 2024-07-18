#define _CRT_SECURE_NO_WARNING 1

#include <thread>
#include "ThreadCache.hpp"


void Test1()
{
	if (TLSthreadcache == nullptr)
	{
		TLSthreadcache = new ThreadCache;
	}
	
	void* p1 = TLSthreadcache->Allocate(1);
}


int main()
{
	Test1();
	
	return 0;
}