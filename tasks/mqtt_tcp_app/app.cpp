#include <iostream>          // Includes the standard input/output stream library (for std::cout)
#include <string>            // Includes the string library so we can use std::string objects
#include <chrono>            // Includes time utilities (used here for setting connection timeouts)
#include <thread>            // Includes multi-threading utilities (used here to make the program pause/sleep)
#include <mqtt/async_client.h> // Includes the Eclipse Paho MQTT C++ asynchronous client library

// Define the network address of the MQTT Broker. "tcp://" specifies the TCP/IP transport protocol.
// Port 1883 is the standard default port for unencrypted MQTT network traffic.
const std::string SERVER_ADDRESS {"tcp://broker.hivemq.com:1883"};

// Define a unique identifier for this specific network client. 
// The broker uses this ID to track this specific TCP/IP connection socket.
const std::string CLIENT_ID      {"cpp_mqtt_test_client"};

// Define the "chat room" or topic name where messages will be sent and read.
const std::string TOPIC          {"cpp/test/topic"};

// Create a custom class that inherits from the MQTT library's callback interface.
// This class acts as a listener, waiting for network events to happen in the background.
class user_callback : public virtual mqtt::callback {
public:
    // This function automatically runs if the TCP/IP network connection to the broker breaks.
    void connection_lost(const std::string& cause) override {
        std::cout << "\n[Warning] Connection lost! Cause: " << cause << std::endl;
    }

    // This function automatically runs whenever a new MQTT message arrives from the network.
    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "\n[Received Message]" << std::endl;
        std::cout << "-> Topic: " << msg->get_topic() << std::endl;      // Prints the topic the message came from
        std::cout << "-> Payload: " << msg->to_string() << std::endl;   // Converts the raw text bytes to a string and prints it
    }

    // This function runs when the broker acknowledges it safely received a message we sent.
    void delivery_complete(mqtt::delivery_token_ptr token) override {
        // Used primarily for Quality of Service (QoS) 1 or 2 to confirm delivery success.
    }
};

int main() {
    // Instantiate the MQTT client object, telling it WHERE to connect (IP/URL) and WHO it is (Client ID).
    mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);

    // Create an instance of our custom listener class.
    user_callback cb;
    
    // Register our listener with the client so the background threads know where to route incoming network events.
    client.set_callback(cb);

    // Create a configuration options builder to tweak our TCP/IP and MQTT connection rules.
    auto connOpts = mqtt::connect_options_builder()
        // clean_session(true) tells the broker to forget any past history or missed messages from this client ID.
        .clean_session(true)
        // Keep-alive sends a tiny "ping" packet every 30 seconds over TCP to prove the connection isn't dead.
        .keep_alive_interval(std::chrono::seconds(30))
        // Finalize compiles all these settings into a single configuration object.
        .finalize();

    try {
        // Print a status message to the console showing where we are attempting to connect.
        std::cout << "Connecting to broker at " << SERVER_ADDRESS << "..." << std::endl;
        
        // Initiate the asynchronous connection request over the TCP/IP network.
        mqtt::token_ptr conntok = client.connect(connOpts);
        
        // This halts the main thread and blocks here until the TCP handshake and MQTT connection are fully successful.
        conntok->wait(); 
        std::cout << "Connected successfully!\n" << std::endl;

        // Print a message indicating we are subscribing to the topic.
        std::cout << "Subscribing to topic: " << TOPIC << "..." << std::endl;
        
        // Ask the broker to subscribe us to the topic. QoS 1 ensures "at least once" network delivery guarantee.
        // ->wait() blocks execution until the broker sends back a subscription confirmation packet.
        client.subscribe(TOPIC, 1)->wait(); 

        // Print a message indicating we are about to transmit data.
        std::cout << "Publishing data..." << std::endl;
        
        // Store our text string inside a variable.
        std::string payload = "Hello from C++ over MQTT/TCP!";
        
        // Create an official MQTT message packet containing our target destination topic and our text payload.
        auto pubmsg = mqtt::make_message(TOPIC, payload);
        
        // Set Quality of Service to 1, meaning the network layer will retry sending until the broker says "got it".
        pubmsg->set_qos(1); 
        
        // Push the message out onto the TCP network socket and wait for the broker's acknowledgment.
        client.publish(pubmsg)->wait();
        std::cout << "Message published." << std::endl;

        // Freeze the main thread for 3 seconds. This gives the background network thread time to receive 
        // the message we just sent (since we are subscribed to the exact same topic).
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Print status that we are shutting down the connection.
        std::cout << "\nDisconnecting..." << std::endl;
        
        // Send an elegant MQTT disconnect packet and gracefully tear down the underlying TCP/IP socket connection.
        client.disconnect()->wait();
        std::cout << "Disconnected." << std::endl;

    } catch (const mqtt::exception& exc) {
        // If anything fails (bad internet, wrong URL, broker down), catch the error here and print the reason code.
        std::cerr << "MQTT Error: " << exc.what() << " [" << exc.get_reason_code() << "]" << std::endl;
        return 1; // Return an error code of 1 to the Operating System
    }

    return 0; // Return a success code of 0 to the Operating System, ending the program
}