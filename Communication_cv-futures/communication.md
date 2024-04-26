# Communication between thread using condition variables and futures
There is the need to comunicate between threads for specific event occurence, without a continuous check on condition for CPU saving.

## Condition variables
Condition variable is a basic mechanism for waiting for an event to be triggered by another thread.

Condition variable is associated with some event, and one or more threads can wait for that event to be happen. 
If some thread has determined that the event is satisfied, it can then notify one or more of the threads waiting for that condition variable, wake them up and allow them to continue processing.

To wait on a condition variable, it is required a *unique_lock* and a condition to be checked (if this condition is not satisfied, the thread is put on sleep waiting for conditional variable notification), also specified with lambda condition for example (row 44 in the following example). 
When a *cv.notify()* is called, all the threads that are sleeping on *cv.wait()*, wake up and check the condition, if not satisfied they returns to sleep, otherwise they continue their next statement.

Condition variable wake up can be due to:
- notification from another thread;
- spurious wake from OS (if the condition is evaluated to false, the thread continues sleeping time).

```cpp
#include <iostream>
#include <mutex>
#include <thread>
#include <string>
#include <thread>
#include <chrono>
#include <condition_variable>

bool have_i_arrived = false;
int total_distance = 5;
int distance_coverd = 0;
std::condition_variable cv;
std::mutex m;

void keep_moving()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		distance_coverd++;

		//notify the waiting threads if the event occurss
		if (distance_coverd == total_distance)
			cv.notify_one();
	}

}

void ask_driver_to_wake_u_up_at_right_time()
{
	std::unique_lock<std::mutex> ul(m);
	cv.wait(ul, [] {return distance_coverd == total_distance; });
	std::cout << "finally i am there, distance_coverd = " << distance_coverd << std::endl;;
}

int main()
{
	std::thread driver_thread(keep_moving);
	std::thread passener_thread(ask_driver_to_wake_u_up_at_right_time);
	passener_thread.join();
	driver_thread.join();
}
```
## Thread safe queue
Queue is a FIFO (first in first out) data structure. The client are served in their arrival order.
Usual queue interface:
- **Push** --> add new element 
- **Pop** --> remove the element
- **Front** --> retrieve the element that will be pop next
- **Empty** --> boolean flag wheter the queue is empty or not
- **Size** --> number of element in the queue

The queue can be implemented using a single linked list. Race condition can happen because multiple threads execute operations in parallel:
- *front* vs *pop*
- *empty* vs *pop*
- *pop* vs *pop*

The implementaion of a thread safe queue must include an additional *wait_pop*.

```cpp
#pragma once
#include <iostream>
#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>
#include <thread>

template<typename T>
class thread_safe_queue {
	std::mutex m;
	std::condition_variable cv;
	std::queue<std::shared_ptr<T>> queue;

public:
	thread_safe_queue()
	{}

	thread_safe_queue(thread_safe_queue const& other_queue)
	{
		std::lock_guard<std::mutex> lg(other_queue.m);
		queue = other_queue.queue;
	}

	void push(T& value)
	{
		std::lock_guard<std::mutex> lg(m);
		queue.push(std::make_shared<T>(value));
		cv.notify_one();
	}

	std::shared_ptr<T> pop()
	{
		std::lock_guard<std::mutex> lg(m);
		if (queue.empty())
		{
			return std::shared_ptr<T>();
		}
		else
		{
			std::shared_ptr<T> ref(queue.front());
			queue.pop();
			return ref;
		}
	}

	bool empty()
	{
		std::lock_guard<std::mutex> lg(m);
		return queue.empty();
	}

	std::shared_ptr<T> wait_pop()
	{
		std::unique_lock<std::mutex> lg(m);
		cv.wait(lg, [this] {
			return !queue.empty();
			});
		std::shared_ptr<T> ref = queue.front();
		queue.pop();
		return ref;
	}

	size_t size()
	{
		std::lock_guard<std::mutex> lg(m);
		return queue.size();
	}

	bool wait_pop(T& ref)
	{
		std::unique_lock<std::mutex> lg(m);
		cv.wait(lg, [this] {
			return !queue.empty();
			});

		ref = *(queue.front().get());
		queue.pop();
		return true;
	}

	bool pop(T& ref)
	{
		std::lock_guard<std::mutex> lg(m);
		if (queue.empty())
		{
			return false;
		}
		else
		{
			ref = queue.front();
			queue.pop();
			return true;
		}
	}
};

```

## Futures
Futures provide a mechanism to access the results on an asynchronous operation.
- A synchronous operation blocks a process until the operation completes.
- An asynchronous operation is non-blocking and only initiates the operation. The caller could discover completion by some other mechanism (for example the futures).

For example when the result of operation two is ready, it can transfer that result via futures to the operation one. The Operation one can proceed its work until it needs the result from operation two.
```
    SYNC                                                  ASYNC  

|Operation 1|                                         |Operation 1|
     |                                                      |
     |                                                      |
     |     |Operation 2|                                    |     |Operation 2|
     ------------->                                         ------------->
                  |                                         |            |
		  |                                         |            |
     <-------------                                         <------------- 
     |                                                      |
     |                                                      |
     |                                                      |
```

If there is an **asynchronous task** and a **creator** thread, this last one will acquire the future object upon initiating the async operation and then proceed to its execution until it needs the async operation. In this case the creator thread will call the *get* function on that future. If the asynchronous operation is not finished yet, the creator thread is blocked until the asynchronous operation is finished, instead if the async operation is already finished, there is no blocking and it can update his shared state.

Summary of steps:
- The creator of the asynchronous task have to obtain the future associate with the asynchronous task.
- When the creator of the async task need the result, it call the *get* method on the future.
- *get* method may block if the asynchronous operation has not yet complete its execution.
- When the asynchronous operation is ready to send a result to the creator, it can do so by modifying shared state that is linked to the creator's future.

In the next example the main thread can *do_other_calculation* in parallel to the async task *find_answer_how_old_universe_is*. Only calling the *get()* function, it remains in a blocking state, if the async task is not finished yet.

```cpp
#include <iostream>
#include <future>

int find_answer_how_old_universe_is()
{
	//this is not the ture value
	return 5000;
}

void do_other_calculations()
{
	std::cout << "Doing other stuff " << std::endl;
}

int main()
{
	std::future<int> the_answer_future = std::async(find_answer_how_old_universe_is);
	do_other_calculations();
	std::cout << "The answer is " << the_answer_future.get() << std::endl;
}
```
### Async Task
In C++ you can create asynchronous operation using **std::async** class.
Its constructor allows you to specify the launch policy, the function to run on the asynchronous task and the arguments for that particular function.
- *async( std::launch policy, Function&& f, Args&&... args)*
  
For the launch policy, you can specify:
- std::launch::async --> tell the compilar to run the function on a separate thread
- std::launch::deferred --> it is runned in the creator thread
- std::launch::async | std::launch::deferred --> the compiler decide how to run this task

In the template parameter of the std::future, must be specified the return type of the function associated to the async task (f1 return type is void, for f2 and f3 is int) or it can be used the *auto* keyword. In this case the async task f2 will run always in the same thread of the main one, because it is used a deferred policy (one the get function is called, it is executed the addition function). The async task f1 is instead always executed in a separate thread. For f3 the decision on thread execution is released to the compiler.

```cpp
#include <iostream>
#include <future>
#include <string>

void printing()
{
	std::cout << "printing runs on-" << std::this_thread::get_id() << std::endl;
}

int addition(int x, int y)
{
	std::cout << "addition runs on-" << std::this_thread::get_id() << std::endl;
	return x + y;
}

int substract(int x, int y)
{
	std::cout << "substract runs on-" << std::this_thread::get_id() << std::endl;
	return x - y;
}

int main()
{
	std::cout << "main thread id -" << std::this_thread::get_id() << std::endl;

	int x = 100;
	int y = 50;

	std::future<void> f1 = std::async(std::launch::async, printing);
	std::future<int> f2 = std::async(std::launch::deferred, addition, x, y);
	std::future<int> f3 = std::async(std::launch::deferred | std::launch::async,
		substract, x, y);

	f1.get();
	std::cout << "value recieved using f2 future -" << f2.get() << std::endl;
	std::cout << "value recieved using f2 future -" << f3.get() << std::endl;

}
```
### std::packaged_task
*Package_task* is another way of creating asynchronous operation using the standard library.

- The class template **std::packaged_task** wraps any callable target so it can be invoked asynchronously. Callable target can be a function, a lambda expression or any other function object.
- It's return value or exception thrown, is stored in a shared state which can be accessed through *std::future* objects.

With the *std::package_task*, the template parameter must be specified in a particular way: first indicate the return type of the callable object and then inside the parentesis must be specified the type of the argument for the callable object. For example for function that takes two integer type arguments and returns integer type value, the template parameter would be like this:
```cpp
std::package_task<int(int,int)> task(callable object)
```
In the following example, the *add* function is wrapped using *package_task*. The *add* function takes two integer arguments and return an integer type, so the template parameter must be specified as "<int(int,int)>". Then the associated *std::future* is acquired. The *package_task* will not start automatically with the constructor, must be called explicitly, before calling the *get()* function on the future, otherwise the *future.get()* will not work properly. 

When it is needed to run the task asynchrounously, a thread must be created, pass this *package_task* to the thread and detach it (example in the *task_thread()* function).
With the thread constructed object, the *package_task* must be moved because it is not copiable. Then in the constructor of the thread must be passed the parameters value needed by the function of the packaged_test. Calling the *detach()* function, the thread will run asynchronously. 

The function *task_normal()* will run in the main thread, the fucntion *task_thread()* will run in a separate thread because it is made asynchronously.
 
```cpp
#include <iostream>
#include <future>
#include <numeric>
#include <thread>
#include <functional>

int add(int x, int y)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	std::cout << "add function runs in : " << std::this_thread::get_id() << std::endl;
	return x + y;
}

void task_thread()
{
	std::packaged_task<int(int, int)> task_1(add);
	std::future<int> future_1 = task_1.get_future();

	std::thread thread_1(std::move(task_1), 5, 6);
	thread_1.detach();

	std::cout << "task thread - " << future_1.get() << "\n";
}

void task_normal()
{
	std::packaged_task<int(int, int)> task_1(add);
	std::future<int> future_1 = task_1.get_future();
	task_1(7, 8);
	std::cout << "task normal - " << future_1.get() << "\n";
}
int main()
{
	task_thread();
	task_normal();
	std::cout << "main thread id : " << std::this_thread::get_id() << std::endl;
}
```

### std::promises

### Exception using std::futures

### std::shared_futures
