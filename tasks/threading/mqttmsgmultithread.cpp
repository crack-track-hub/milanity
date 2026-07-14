#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <string>

using namespace std;

// ====================================================================
// THE BROKER STORAGE (Namma Server-oda Data Buffer)
// ====================================================================
// Real-world-la Sender anuppura msg-ah Broker intha buffer-la thaan vachiruppar.
queue<string> broker_topic_queue; 
mutex broker_mutex; // Rendu clients ore nerathula broker-ah access pannaama irukka LOCK

// ====================================================================
// SENDER CLIENT (Client 1 - Publisher)
// ====================================================================
void Sender_Client1() {
    for (int i = 1; i <= 10; i++) {
        // Step 1: Data-ve uruvakkurhom
        string message = "Temperature_Data_" + to_string(i * 10) + "C";
        
        // Step 2: Broker ulla data-ve podurathukku LOCK panrom
        broker_mutex.lock(); 
        
        broker_topic_queue.push(message); // Message-ah Broker Queue-kulla thallுறோம் (Publish)
        cout << "[Sender - Client 1] ---> PUBLISHED: " << message << " to Topic: 'home/temp'" << endl;
        
        broker_mutex.unlock(); // Safe-ah Unlock panrom
        
        // Adutha packet-ku munnadi chinna delay
        this_thread::sleep_for(chrono::milliseconds(1000)); 
    }
    cout << "[Sender - Client 1] Finished sending all packets." << endl;
}

// ====================================================================
// RECEIVER CLIENT (Client 2 - Subscriber)
// ====================================================================
void Receiver_Client2() {
    int received_count = 0;
    
    // Namma 10 messages anuppalaam, so 10 messages receive aagara varaikkum loop odum
    while (received_count < 10) {
        
        broker_mutex.lock(); // Broker queue-la data irukaanu check panna LOCK panrom
        
        // Broker kitta ethavathu message vandhu irukaanu check panrom
        if (!broker_topic_queue.empty()) {
            
            // Step 3: Queue-oda munnadi irukura message-ah Broker-lendhu vanguroam
            string received_msg = broker_topic_queue.front();
            broker_topic_queue.pop(); // Vangiyachhu, so queue-la irundhu delete panrom
            
            cout << "[Receiver - Client 2] <--- RECEIVED from Broker: " << received_msg << endl;
            received_count++;
        }
        
        broker_mutex.unlock(); // Unlock panrom
        
        // Continuous loop network-ah heavy-ah block pannaama irukka kutti delay
        this_thread::sleep_for(chrono::milliseconds(400)); 
    }
    cout << "[Receiver - Client 2] Finished receiving all subscribed packets." << endl;
}

// ====================================================================
// MAIN CONTROL (The Network Initialization)
// ====================================================================
int main() {
    cout << "=== Real-world MQTT Multithreading Simulation Started ===" << endl;
    cout << "Broker Network is Live. Clients are connecting...\n" << endl;

    // STEP 4: Sender matrum Receiver rendu peraiyum T0 and T1 parallel threads-ah start panrom
    // Rendu perume background-la ore nerathula (Parallel-ah) run aaguvanga
    thread t1(Sender_Client1);
    thread t2(Receiver_Client2);

    // STEP 5: Main thread hold panrom, background process mudiyura varaikkum
    t1.join();
    t2.join();

    cout << "\n=== Real-world MQTT Multithreading Simulation Ended ===" << endl;
    return 0;
}