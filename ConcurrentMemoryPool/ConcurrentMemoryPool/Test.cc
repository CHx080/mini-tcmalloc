#define _CRT_SECURE_NO_WARNING 1
#include <iostream>
#include <thread>
#include "ThreadCache.hpp"
using std::cout;
using std::endl;


void CreateThreadCache()
{
	if (TLSthreadcache == nullptr)
	{
		TLSthreadcache = new ThreadCache;
	}
	for (int i = 0; i < 5; ++i) cout << std::this_thread::get_id()<<" : "<<TLSthreadcache << endl;
}


int main()
{
	/*std::thread t1(CreateThreadCache);
	t1.join();
	std::thread t2(CreateThreadCache);
	t2.join();*/

	
	
	return 0;
}