#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

// Mocking MQTT TCP Connection
void connectTCPBroker() {
    cout << "Connecting to broker.hivemq.com:1883..." << endl;
    this_thread::sleep_for(chrono::milliseconds(1000)); // TCP Handshake delay
    cout << "Connected successfully!" << endl;
}

void publishPayload() {
    cout << "Sending Packet =1" << endl;
    this_thread::sleep_for(chrono::milliseconds(500));
}

void listenForIncomingPackets() {
    cout << "Listening for incoming subscribe messages..." << endl;
    // Inga infinite loop potta, main thread ingeye maasikiduvum, vera velai seiyathu
    for(int i=1; i<=3; i++) {
        cout << "Received Packet " << i << " from Broker." << endl;
        this_thread::sleep_for(chrono::milliseconds(800));
    }
}

int main() {
    cout << "--- Single Thread MQTT TCP Started ---" << endl;

    connectTCPBroker();
    publishPayload();
    listenForIncomingPackets(); // Ithu mudiyura varaikkum adutha line-ku pogaathu

    cout << "--- Single Thread MQTT TCP Ended ---" << endl;
    return 0;
}