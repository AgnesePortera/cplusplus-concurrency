# C++ concurrency
List of rules to be followed for thread safe development in C++.

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

