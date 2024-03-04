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

