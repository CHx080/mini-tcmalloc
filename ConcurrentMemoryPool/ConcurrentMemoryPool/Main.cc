#include "ObjectMemoryPool.hpp"
#include <time.h>
#include <iostream>
typedef struct TreeNode
{
	size_t _val = 0;
	struct TreeNode* _left = nullptr;
	struct TreeNode* _right = nullptr;
}TreeNode;

constexpr unsigned long long NUM = 200000; //编译是初始化(代替define)

int main()
{
	ObjectPool<TreeNode>* pool = new ObjectPool<TreeNode>;
	
	size_t begin1 = clock();
	for (int i = 0; i < NUM; ++i)
	{
		delete (new TreeNode);
	}
	size_t end1 = clock();

	size_t begin2 = clock();
	for (int i = 0; i < NUM; ++i)
	{
		try
		{
			pool->Delete(pool->New());
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}
	size_t end2 = clock();

	std::cout << end1 - begin1 << std::endl;
	std::cout << end2 - begin2 << std::endl;
	return 0;
}