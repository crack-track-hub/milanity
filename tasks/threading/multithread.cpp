#include <iostream>
#include <thread> // Thread use panna ithu mukkiyam
#include <chrono>

using namespace std;

void taskA() {
    for (int i = 1; i <= 3; i++) {
        cout << "Task A running - Step " << i << endl;
        this_thread::sleep_for(chrono::seconds(15));
    }
}

void taskB() {
    for (int i = 1; i <= 3; i++) {
        cout << "Task B running - Step " << i << endl;
        this_thread::sleep_for(chrono::seconds(15));
    }
}

int main() {
    cout << "--- Multi-thread Program Started ---" << endl;

    // Rendu thani thani threads uruvaki parallel-ah start panrom
    thread t1(taskA);
    thread t2(taskB);

    // Neenga sonna maari, main thread-ah "Hold" panni vaikirom
    t1.join();
    t2.join();

    cout << "--- Multi-thread Program Ended ---" << endl;
    return 0;
}