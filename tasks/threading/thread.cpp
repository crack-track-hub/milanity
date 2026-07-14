#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

void taskA() {
    for (int i = 1; i <= 3; i++) {
        cout << "Task A running - Step " << i << endl;
        this_thread::sleep_for(chrono::milliseconds(500)); // 0.5 second delay
    }
}

void taskB() {
    for (int i = 1; i <= 3; i++) {
        cout << "Task B running - Step " << i << endl;
        this_thread::sleep_for(chrono::milliseconds(500)); // 0.5 second delay
    }
}

int main() {
    cout << "--- Single Thread Program Started ---" << endl;

    // Line by Line execution (Sequential)
    taskA(); // Ithu muthala run aagi mudiyum
    taskB(); // Ithu adutha thaan start aagum

    cout << "--- Single Thread Program Ended ---" << endl;
    return 0;
}