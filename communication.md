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
template<typename T>
class sequential_queue5 {

	struct node
	{
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;

		node()
		{}

		node(T _data) : data(std::move(_data))
		{
		}
	};

	std::unique_ptr<node> head;
	node* tail;

	std::mutex head_mutex;
	std::mutex tail_mutex;

	std::condition_variable cv;

	node* get_tail()
	{
		std::lock_guard<std::mutex> lg(tail_mutex);
		return tail;
	}

	std::unique_ptr<node> wait_pop_head()
	{
		//? protect head node with mutex and unique_lock
		std::unique_lock<std::mutex> lock(head_mutex);

		//? Need to wait for the condion variable
		//* (maybe someone pushing to the queue at the moment)
		//? and also check if there is something in the queue
		//* (head.get() == get_tail()) - when head and tail points to the dummy node
		//! do we stick in the loop until one element will appear in the queue?
		//! do we need this?
		head_condition.wait(lock, [&] { return head.get() != get_tail(); });

		//! 'const' cast issue here, remvoe const (copy ellision won't work on const)
		// std::unique_ptr<node> const old_head = std::move(head);
		std::unique_ptr<node> old_head = std::move(head);

		head = std::move(old_head->next);
		return old_head;	//! be carefull, non const variable needed to allow copy ellision
	}

	public:
	sequential_queue5() :head(new node), tail(head.get())
	{}

	void push(T value)
	{
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(value)));
		std::unique_ptr<node> p(new node);
		node* const new_tail = p.get();
		{
			std::lock_guard<std::mutex> lgt(tail_mutex);
			tail->data = new_data;
			tail->next = std::move(p);
			tail = new_tail;
		}

		cv.notify_one();
	}

	std::shared_ptr<T> pop()
	{
		std::lock_guard<std::mutex> lg(head_mutex);
		if (head.get() == get_tail())
		{
			return std::shared_ptr<T>();
		}
		std::shared_ptr<T> const res(head->data);
		std::unique_ptr<node> const old_head = std::move(head);
		head = std::move(old_head->next);
		return res;
	}

	std::shared_ptr<T> wait_pop()
	{
		//! std::unique_ptr and not std::unique_lock
		std::unique_ptr<node> old_head = wait_pop_head();	 //! no need std::move() because of copy ellision
		return old_head ? old_head->data : std::shared_ptr<T>();
	}

	// Printer method: prints cells from top to bottom
	void printData();
};

template <typename T>
inline void sequential_queue5<T>::printData()
{
	if (head.get() == get_tail())
	{
		std::cout << "Queue is empty...\n";
		return;
	}

	std::lock_guard<std::mutex> hlg(head_mutex);

	node* current = head.get();
	std::cout << "Queue from top to bottom...\n";
	int index{};
	while (current->data != nullptr)
	{
		std::cout << "current: " << current << ", value [" << index++ << "]: " << *(current->data) << std::endl;
		current = (current->next).get();
	}
	std::cout << "End of the queue...\n";
}

```

