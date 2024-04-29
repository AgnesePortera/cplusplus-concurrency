# C++ concurrency

- [C++ concurrency](#c---concurrency)
    + [std::thread class](#std--thread-class)
  * [How to launch a thread](#how-to-launch-a-thread)
  * [Join and detach](#join-and-detach)
    + [Detach](#detach)
  * [Pass parameter to a thread](#pass-parameter-to-a-thread)
  * [Ownership of a thread](#ownership-of-a-thread)
  * [Useful functions](#useful-functions)
    + [get_id()](#get-id--)
    + [sleep_for()](#sleep-for--)
    + [yield()](#yield--)
    + [hardware_concurrency()](#hardware-concurrency--)
  * [Thread local storage](#thread-local-storage)


List of rules to be followed for thread safe development in C++.

### std::thread class
|Constructors||
|-|-|
|default (1)|thread() noexcept;|
|initialization (2)|template <class Fn, class... Args>explicit thread (Fn&& fn, Args&&... args);|
|copy [deleted] (3)|thread (const thread&) = delete;|
|move (4)|thread (thread&& x) noexcept;|

> [!NOTE]
> Copy constructor is deleted, there is the move constructor.

> [!NOTE]
> The initialization constructor gives the possibility to pass a function with arguments.

## How to launch a thread
- add function to be executed as parameter to thread class constructor
- using callable object
- using lambda expression
```cpp
void func1()
{
	std::cout << "Hello from function \n";
}

class my_class {

public:
	void operator()()
	{
		std::cout << "hello from the class with function call operator \n";
	}
};

int main()
{
	//create thread using fuction
	std::thread thread1(func1);

	//create thread using class with function call operator
	my_class mc;
	std::thread thread2(mc);

	//create thread using lambda expression
	std::thread thread3([] {
		std::cout << "hello from the lambda \n";
		});

	thread1.join();
	thread2.join();
	thread3.join();

	std::cout << "This is main thread \n";
}

std::cout << "\n\nAllowed max number of parallel threads : "
		<< std::thread::hardware_concurrency() << std::endl;
```

## Join and detach
Properly constructed (means created with function, object constructor or lambda) thread object represent an active thread of execution in hardware level. This thread object is **joinable**.
For any joinable thread, it must be called **join** or **detach** function. After calling these functions, the thread become non-joinable.

If you forgot to join or detach on joinable thread, then at the time of destructor call to that thread object, **std::terminate** function will be called.
If any program have *std::terminate* call, it is considered **unsafe**.

Defaulted constructed thread does not represent an execution and it is not joinable:
```cpp
std::thread thread1;
if(thread1.joinable()){
 printf("Never print this \n");
}
```
### Detach
Separates the launched thread from the thread object which it launched from, allowing execution to continue independently.
Any allocated resources will be freed one the thread exits.
The main thread cannot wait the detached thread for finishing and the secondary thread can continue the execution also if the main thread is terminate.

After a call to **join** or **detach**, the thread object become non joinable and can be destroyed safely.

## Pass parameter to a thread
```cpp
void func_2(int& x)
{
	while (true)
	{
		std::cout << "Thread_1 x value : " << x << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

int main()
{
	int x = 9;
	std::cout << "Main thread current value of X is : " << x << std::endl;
	std::thread thread_1(func_2, std::ref(x));
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	x = 15;
	std::cout << "Main thread X value changed to : " << x << std::endl;
	thread_1.join();
}

```
## Ownership of a thread
For transferring the ownership of a thread, the move constructor can be used.
- Thread object cannot be copied (no copy constructor present). The assignment in *Example1* generate a compile time error.
- Using explicitly *std::move*, move one thread to another one (*Example2*), transferring the ownership from *thread_1* to *thread_2* variable.
  
> [!IMPORTANT]
> **Owner of the thread object is responsible to manage all the thread lifecycle and must manage it calling join or detach explicitly.**

- In *Example3* there is no compile error, because at time of assignment, the variable *thread_1* does not own any thread object (previously moved on variable *thread_2*).
- *Example4* is wrong, exception is raised at runtime, because the variable *thread_1* is assigned with *std::move(thread_3)*, when it owns another thread already.
  
> [!IMPORTANT]
> **Cannot be tranferred ownership when leftside variable owning a thread.**

```cpp
#include <iostream>
#include <thread>

void func_1()
{
	std::cout << "This is a function 1";
}

void func_2()
{
	std::cout << "This is a function 2";
}

int main()
{
	std::thread thread_1(func_1);

	//Example1: try to assigne one thread to another
	std::thread thread_2 = thread_1;  //compile error

	//Example2: move one thread form another
	std::thread thread_2 = std::move(thread_1);

	//Example3: implicit call to move constructor
	thread_1 = std::thread(func_2);

	//Example4: this is wrong
	std::thread thread_3 = std::move(thread_2);	
	thread_1 = std::move(thread_3);

	thread_1.join();
	thread_3.join();

}
```

## Useful functions
### get_id()
- Return unique thread id for each active thread of execution
- Return 0 for all non active threads
```cpp
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
	std::thread thread_1(func_1);

	std::cout << "thread_1 id before joining : " << thread_1.get_id() << std::endl;
	thread_1.join();

	std::thread thread_2;

	std::cout << "default consturcted thread id : " << thread_2.get_id() << std::endl; // 0 for non active thread
	std::cout << "thread_1 id after joining : " << thread_1.get_id() << std::endl; // become 0 after join
	std::cout << "Main thread id : " << std::this_thread::get_id() << std::endl;

	std::cout << "\n\nAllowed max number of parallel threads : "
		<< std::thread::hardware_concurrency() << std::endl;
}
```

### sleep_for()
- Blocks the execution of the current thread for at least the specified sleep duration
- This function may block for longer than sleep duration due to scheduling or resource contention delays

### yield()
- Yield will give up the current time slice and re-insert the thread into the scheduling queue.
- The amount of time that expires until the thread is executed again is usually entirely dependent upon the scheduler.
- This allow the other threads that are available, to perform their tasks.

### hardware_concurrency()
- Return the number of concurrent threads supported by the implementation.
- The value should be considered only a hint.
- Using task switching or round robin, the number of threads is higher than the cores number.

## Thread local storage
- if a variable is declared as *thread_local*, then each thread is going to have its own, distinct object.
- The storage duration is the entire execution of the thread in which it was created.
- The value stored in the object is initialized when the thread is started.
- When the thread is finished, there is a memory clean up of the variable.

```cpp
#include <iostream>
#include <thread>
#include <atomic>


thread_local std::atomic<int> i = 0;

void foo() {
    ++i;
    std::cout << i;
}

int main() {
    std::thread t1(foo);
    std::thread t2(foo);
    std::thread t3(foo);

    t1.join();
    t2.join();
    t3.join();

    std::cout << std::endl;

    // output is --> 111
}
```
