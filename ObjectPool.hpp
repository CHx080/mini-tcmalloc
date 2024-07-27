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
		if (_freelist) //�������������ڴ棬ֱ�Ӵ�������ȡ
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
					throw std::bad_alloc(); //����ʧ�����쳣
				}
			}

			obj = (T*)_memory;
			size_t objsize= sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			//����������ʹ�СС��ָ�����ʹ�Сʱ������ָ���С����֤������Ŀռ���Դ���һ��ָ�����
			_memory += objsize;
			_leftbytes -= objsize;
		}
		mtx.unlock();
		new(obj)T; //��λnew��ʼ������
		return obj;
	}

	void Delete(T* obj)
	{
		obj->~T();
		*(void**)obj = _freelist;
		_freelist = obj;
	}
private:
	size_t _leftbytes = 0;//����ڴ�ʣ���ֽ���
	char* _memory = nullptr; //ָ�����ڴ�
	void* _freelist = nullptr;//ָ����������
};