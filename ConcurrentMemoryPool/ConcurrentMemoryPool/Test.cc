#define _CRT_SECURE_NO_WARNING 1

#include <thread>

#include "ThreadCache.hpp"


void Test1()
{
	if (TLSthreadcache == nullptr)
	{
		TLSthreadcache = new ThreadCache;
	}
	
	char* p1 = (char*)TLSthreadcache->Allocate(1);
	char* p2 = (char*)TLSthreadcache->Allocate(2);
	char* p3 = (char*)TLSthreadcache->Allocate(3);
	char* p4 = (char*)TLSthreadcache->Allocate(4);
	char* p5 = (char*)TLSthreadcache->Allocate(5);
	char* p6 = (char*)TLSthreadcache->Allocate(6);
	char* p7 = (char*)TLSthreadcache->Allocate(7);
	char* p8 = (char*)TLSthreadcache->Allocate(8);

	TLSthreadcache->Deallocate(p1, 1);
	TLSthreadcache->Deallocate(p2, 2);
	TLSthreadcache->Deallocate(p3, 3);
	TLSthreadcache->Deallocate(p4, 4);
	TLSthreadcache->Deallocate(p5, 5);
	TLSthreadcache->Deallocate(p6, 6);
	TLSthreadcache->Deallocate(p7, 7);
	TLSthreadcache->Deallocate(p8, 8);

	/*void* p1 =  TLSthreadcache->Allocate(1);
	void* p2 =  TLSthreadcache->Allocate(2);
	void* p3 =  TLSthreadcache->Allocate(3);
	void* p4 =  TLSthreadcache->Allocate(4);
	void* p5 =  TLSthreadcache->Allocate(5);
	void* p6 =  TLSthreadcache->Allocate(6);
	void* p7 =  TLSthreadcache->Allocate(7);
	void* p8 =  TLSthreadcache->Allocate(10);*/
	
	//cout << p1 << endl << p2 << endl << p3 << endl << p4 << endl << p5 << endl << p6 << endl << p7 << endl << p8 << endl;
	//cout << p2-p1 << endl << p3-p2 << endl << p4-p3 << endl << p5-p4 << endl << p6-p5 << endl << p7-p6 << endl << p8-p7 << endl;


}


int main()
{
	/*std::thread t1(Test1);
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
	t10.join();*/
	
	Test1();
	
	return 0;
}