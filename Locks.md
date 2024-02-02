# Locking mechanisms
For sharing memory between different threads, locking mechanism must be provided, otherwise the result output would be invalid.

## Race condition
In concurrency race condition is anything where the outcome depends on the relative order of operations execution on two or more threads.

## Mutex
Mutex provides mutual exclusive access of shared data from multiple threads.
Mutex class has three functions:
|**Locking**||
|-|-|
|*lock*|locks the mutex, blocks if the mutex is not available|
|*try_lock*|try to lock the mutex, return if the mutex is not available|
|*unlock*|unlocks the mutex|

Here an example. Once a thread acquire a lock, the other must wait until the lock is released (it is important to call *unlock()*, otherwise any other thread cannot do any operation and remains in a waiting state forever).
In this way the access to the list is mutually exclusive, even if multiple threads are tried to access at the same time, only one thread will be allowed to proceed. 

```cpp
#include <iostream>
#include <mutex>
#include <list>
#include <thread>

std::list<int> my_list;
std::mutex m;

void add_to_list1(int const& x)
{
	m.lock();
	my_list.push_front(x);
	m.unlock();
}


void size1()
{
	m.lock();
	int size = my_list.size();
	m.unlock();
	std::cout << "size of the list is : " << size << std::endl;
}


void add_to_list2(int const& x)
{
	std::lock_guard<std::mutex> lg(m);
	my_list.push_back(x);
}

void size2()
{
	std::lock_guard<std::mutex> lg(m);
	int size = my_list.size();
	std::cout << "size of the list is : " << size << std::endl;
}

int  main()
{
	std::thread thread_1(add_to_list2, 4);
	std::thread thread_2(add_to_list2, 11);

	thread_1.join();
	thread_2.join();
}
```

### Lock_guard
The class **Lock_guard** is a mutex wrapper that provides a convenient RAII-style mechanism for owning a mutex for the duration of a scoped block.
When a *lock_guard* object is created, it attemps to take ownership of the mutex that it is given. When the control leaves the scope in which the *lock_guard* object was created, the *lock_guard* is destructed and the mutex is released.

```cpp
std::list<int> my_list;
std::mutex m;

void add_to_list1(int const& x)
{
	std::lock_guard<std::mutex> lg(m);
	my_list.push_front(x);
}
```

### Wrong mutex usage
- Returning pointer or reference to the protected data. In this way the pointer can be used without thread safety.
- Passing code to the protected data structure which you don't have control with, because you dont' know what that particular function do (for example the *maliscious_function* below in *example_2*, assign the data pointer to an external object that can then be used by other threads without mutex).

```cpp  
#include <iostream>
#include <mutex>
#include <list>
#include <thread>

/*********************** example 1 *********************/
class list_wrapper {
	std::list<int> my_list;
	std::mutex m;

public:
	void add_to_list(int const& x)
	{
		std::lock_guard<std::mutex> lg(m);
		my_list.push_front(x);
	}

	void size()
	{
		std::lock_guard<std::mutex> lg(m);
		int size = my_list.size();
		std::cout << "size of the list is : " << size << std::endl;
	}

	std::list<int>* get_data()
	{
		return &my_list;
	}
};

/*********************** example 2 *********************/
class data_object {

public:
	void some_operation()
	{
		std::cout << "this is some operation \n";
	}
};

class data_wrapper {

	data_object protected_data;
	std::mutex m;

public:
	template <typename function>
	void do_some_work(function f)
	{
		std::lock_guard<std::mutex> lg(m);
		f(protected_data);
	}
};

data_object* unprotected_data;

void maliscious_function(data_object& data)
{
	unprotected_data = &data;
}

void run_code()
{
	data_wrapper wrapper;
	wrapper.do_some_work(malisious_function);
}


int main()
{
	run_code();
}
```
