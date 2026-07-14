Basic C++ Libraries

#include <iostream>     
// Console-la input (cin) and output (cout) use panna library.
// Example:
cout << "Hello World";
cin >> age;

#include <string>       
// String (text) data type use panna library.
// Example: string name = "Abishake";
// Example:
string name = "Abishake";
cout << name;

#include <vector>       
// Dynamic array.
// Runtime-la size increase/decrease panna mudiyum.
// Example:
vector<int> num; vector-இல் integer type data store panna mudiyum. vector muthalil kaliyaga irukum
num.push_back(10);push_back() என்பது vector-இன் கடைசி (end) பகுதியில் ஒரு புதிய element-ஐ சேர்க்கும் function.
num.push_back(20);

#include <array>        
// Fixed size array.
// Size compile time-la decide pannuvom.
// Example:
array<int,5> arr = {10,20,30,40,50};

#include <map>          
// Key-Value pair store panna.
// Keys automatic-ah sorted-ah irukkum.
// Example: ID -> Name.
// Example:
map<int,string> student;
student[101] = "Abishake";

// diff between map and unordered_map is that map is ordered and unordered_map is unordered.
//  In map, the elements are stored in a sorted order based on the keys, 
// while in unordered_map, the elements are stored in an arbitrary order.


#include <unordered_map>
// Key-Value pair store panna.
// Fast access.
// Sorting irukkadhu.
// Example:
unordered_map<int,string> student;
student[101] = "Abishake";

#include <set>          
// Duplicate values not allowed.
// Values automatic-ah sorted-ah irukkum.
// Example:
set<int>num;
num={10,20,20,30};
num.insert(40);
cout<<num <<endl;
// set<int> num = {10,20,20,30};

#include <list>         
// Doubly Linked List.
// Middle-la insert/delete fast-ah pannalaam.
// Example:
list<int> marks;
marks.push_back(90);
marks.push_front(80);

#include <queue>        
// FIFO (First In First Out).
// First vandhadhu first veliya pogum.
// Example: Printer Queue.
// Example:
// queue<int> q;
// q.push(10);
// q.push(20);
// q.pop();

#include <stack>        
// LIFO (Last In First Out).
// Last vandhadhu first veliya varum.
// Example: Plate Stack.
// Example:
// stack<int> s;
// s.push(10);
// s.push(20);
// s.pop();

#include <algorithm>    
// Ready-made algorithms.
// sort(), find(), reverse(), max(), min() etc.
// Example:
// sort(arr, arr+5);
// reverse(arr, arr+5);
// find(arr, arr+5, 20);

#include <cmath>        
// Mathematical functions.
// sqrt(), pow(), sin(), cos(), log() etc.
// Example:
// sqrt(25);
// pow(2,3);

#include <cstdlib>      
// General utility functions.
// rand(), srand(), exit(), malloc() etc.
// Example:
// rand();
// exit(0);

#include <ctime>        
// Date and Time related functions.
// time(), clock(), localtime() etc.
// Example:
// time_t now = time(0);

#include <fstream>      
// File handling.
// File read/write/open/close panna use pannuvom.
// Example:
// ofstream file("data.txt");
// file << "Hello";
// file.close();

#include <sstream>      
// String-a stream maari use panna.
// String parsing and conversion-ku use pannuvom.
// Example:
// stringstream ss("100");
// int x;
// ss >> x;

#include <iomanip>      
// Output formatting.
// setw(), setprecision(), fixed() use pannuvom.
// Example:
// cout << fixed << setprecision(2);
// cout << 3.14159;


//C++11 Libraries (Most Important)

#include <thread>       // Multithreading
#include <mutex>        // Lock
#include <chrono>       // Time
#include <future>       // Async
#include <atomic>       // Atomic variables
#include <condition_variable> // Thread synchronization
#include <memory>       // Smart pointers
#include <functional>   // std::function
#include <tuple>        // Tuple
#include <utility>      // move(), pair()
#include <type_traits>  // Type checking


// Linux Libraries (IoT)

#include <unistd.h>     // sleep(), fork(), read(), write()
#include <fcntl.h>      // File control
#include <sys/socket.h> // Socket
#include <arpa/inet.h>  // IP Address
#include <netinet/in.h> // Internet protocols
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>     // Directory
#include <signal.h>     // Signals
#include <pthread.h>    // POSIX Threads


// Socket Programming (TCP/IP)

socket()
bind()
listen()
accept()
connect()
send()
recv()
close()
inet_addr()
htons()
ntohs()


//keywords
TCP
UDP
Socket
Port
IP Address
Client
Server
Packet
Buffer

//MQTT (Mosquitto / Paho)


connect()
disconnect()
publish()
subscribe()
unsubscribe()
loop_start()
loop_stop()
loop_forever()

//keywords

Broker
Client
Topic
Payload
QoS
Retain
Publish
Subscribe
Session
Keep Alive
Will Message


//Modbus
Read Coils
Read Holding Registers
Read Input Registers
Write Coil
Write Register
CRC
RTU
ASCII
TCP
Slave
Master

//Registers
00001 Coils

10001 Inputs

30001 Input Registers

40001 Holding Registers

//BACnet
BACnet/IP
BACnet MSTP
Device
Object
Object Identifier
Property
Read Property
Write Property
Who-Is
I-Am
Device Discovery
Object List
Present Value

//Objects
Analog Input

Analog Output

Analog Value

Binary Input

Binary Output

Binary Value

Schedule

Trend Log

//CAN Bus

CAN

CAN FD

Arbitration

Identifier

Frame

RTR

DLC

Baud Rate

Bus

Node

//Serial Communication

termios
serial

//Keywords
UART

RS232

RS485

Baud Rate

Parity

Stop Bit

Data Bit

TX

RX

//I2C


Master

Slave

SDA

SCL

ACK

NACK

Address

Clock Stretching


//SPI

Master

Slave

MOSI

MISO

SCK

SS

Clock

Mode


//Embedded / RTOS

Task

Thread

Mutex

Semaphore

Queue

ISR

Interrupt

Scheduler

Tick

Priority

Context Switch

Deadlock

Race Condition


// STL Keywords

vector
map
set
queue
stack
pair
tuple
iterator
auto
nullptr


// C++ Keywords (Most Asked)

class
struct
public
private
protected
friend
virtual
override
template
typename
const
constexpr
static
inline
explicit
mutable
volatile
namespace
using
typedef
auto
decltype
this
new
delete
throw
try
catch