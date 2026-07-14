#include <iostream>
#include <thread>
#include <chrono>
#include <atomic> // Thread safety-kaga

using namespace std;

// Thread 1: Continuous TCP Keep-Alive Loop (Ping requests)
void keepAliveNetworkLoop(atomic<bool>& running) {
    while(running) {
        cout << "sending from phase 1..." << endl;
        this_thread::sleep_for(chrono::seconds(2)); // Every 2 seconds ping
    }
    cout << " TCP Connection Closed cleanly." << endl;
}

// Thread 2: Handling Outgoing Publish Messages
void publishData(atomic<bool>& running) {
    int count = 1;
    while(running && count <= 3) {
        cout << "Sending Sensor Data " << count << endl;
        this_thread::sleep_for(chrono::milliseconds(1500));
        count++;
    }
}

// Thread 3: Handling Incoming Subscribe Messages
void receiveData(atomic<bool>& running) {
    int count = 1;
    while(running && count <= 3) {
        cout << "message received " << count << endl;
        this_thread::sleep_for(chrono::milliseconds(1200));
        count++;
    }
}

int main() {
    cout << "--- Multi-thread MQTT TCP Client Started ---" << endl;
    
    // Threads-ah control panna oru flag
    atomic<bool> isConnected(true);

    // 3 thani thani threads-ah parallel-ah run panrom over same TCP connection
    thread networkThread(keepAliveNetworkLoop, ref(isConnected));
    thread pubThread(publishData, ref(isConnected));
    thread subThread(receiveData, ref(isConnected));

    // Main thread 5 seconds appadiye HOLD (wait) pannum, parallel threads run aaga
    this_thread::sleep_for(chrono::seconds(5));
    
    // Ellா thread-aiyum nirutha solrom
    isConnected = false;

    // Neenga sonna maari, clean up panna holding state (Join)
    networkThread.join();
    pubThread.join();
    subThread.join();

    cout << "--- Multi-thread MQTT TCP Client Ended ---" << endl;
    return 0;
}