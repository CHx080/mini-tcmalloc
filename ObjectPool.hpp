#pragma once
#include <stdlib.h>
#include <exception>
#include <mutex>
template<typename T> class ObjectPool
{
private:
	std::mutex mtx;
public:
	T* New()
	{
		
		T* obj = nullptr;
		mtx.lock();
		if (_freelist) //自由链表中有内存，直接从链表中取
		{
			obj = (T*)_freelist;
			_freelist = *(void**)_freelist;
		}
		else
		{
			if (_leftbytes<sizeof(T))
			{
				_leftbytes = 128 * 1024;
				_memory = (char*)malloc(_leftbytes);
				if (_memory == nullptr)
				{
					throw std::bad_alloc(); //申请失败抛异常
				}
			}

			obj = (T*)_memory;
			size_t objsize= sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			//当申请的类型大小小于指针类型大小时，分配指针大小，保证所申请的空间可以存下一个指针变量
			_memory += objsize;
			_leftbytes -= objsize;
		}
		mtx.unlock();
		new(obj)T; //定位new初始化对象
		return obj;
	}

	void Delete(T* obj)
	{
		obj->~T();
		*(void**)obj = _freelist;
		_freelist = obj;
	}
private:
	size_t _leftbytes = 0;//大块内存剩余字节数
	char* _memory = nullptr; //指向大块内存
	void* _freelist = nullptr;//指向自由链表
};