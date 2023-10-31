# C++ concurrency
List of rules to be followed for thread safe development in C++.

### std::thread class
|Method||
|-|-|
|default (1)|thread() noexcept;|
|initialization (2)|template <class Fn, class... Args>explicit thread (Fn&& fn, Args&&... args);|
|copy [deleted] (3)|thread (const thread&) = delete;|
|move (4)|thread (thread&& x) noexcept;|

Copy constructor is deleted, there is the move constructor.
The initialization constructor gives the possibility to pass a function with arguments.

## How to launch a thread
- add function to be executed as parameter to thread class constructor
- using callable object
- using lambda expression
```
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
```
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

## Pass parameters to a thread
```
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


