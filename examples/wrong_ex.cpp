#include <thread>
#include <iostream>

void threadB() {
    std::cout << "Start thread B\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Bye thread B\n";
}

void threadA(std::thread &B) {
    B = std::thread(threadB);
    //B.join(); //--> removing this join, will give error at runtime:
                //Exit Thread A
                //terminate called without an active exception
                //Start thread B

    std::cout << "Exit Thread A\n";
}

int main() {
    std::thread B;
    std::thread A(threadA, std::ref(B));
    A.join();
}
