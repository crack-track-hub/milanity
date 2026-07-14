// #include<iostream>
// using namespace std;
// int age;
// void hello()
// {
//         cout<<"enter your age";
//         cin>>age;
//         if (age>=18)
//         {
//             cout<<"elligleble"<<endl;
//         }
//         else
//         {
//             cout<<"not elliglible"<<endl;
//         }
// }
// void aray()
// {
//       int marks[5] = {10,20,30,40,50};

//     cout << marks[0]<<endl;


// }
// int main()
// {

//     hello();
//     aray();
    
//     int i;
//     for(i=0;i<2;i++)
//     {
//         cout<<"hi"<<endl;
//     }
//     cout<<"your age is:" <<age<<endl;
//     cout<<"hello world"<<endl;
//     while(i<2)
//     {
//         cout<<"hello"<<endl;
//         i++;
//     }
//     return 0;
// }



// #include <iostream>
// using namespace std;

// int main()
// {
//     int age = 21;

//     int *pt = &age;

//     cout << age << endl;
//     cout << &age << endl;
//     cout << pt << endl;
//     cout << *pt << endl;

//     return 0;
// }






// #include <iostream>
// #include <unistd.h>

// using namespace std;

// int main()
// {
//     fork();

//     cout << "Hello"<< endl;

//     return 0;
// }











// #include <iostream>
// using namespace std;

// class Student
// {
// public:
//     string name;
//     int age;
// };

// int main()
// {
//     Student s1;

//     s1.name = "Abishake";
//     s1.age = 21;

//     cout << s1.name << endl;
//     cout << s1.age << endl;

//     return 0;
// }





// #include <iostream>
// using namespace std;

// // Class
// class Student
// {
// public:
//     string name;
//     int age;
// };

// int main()
// {
//     // 3 Objects
//     Student s1;
//     Student s2;
//     Student s3;

//     // Object 1 Data
//     s1.name = "Abishake";
//     s1.age = 21;

//     // Object 2 Data
//     s2.name = "Kumar";
//     s2.age = 22;

//     // Object 3 Data
//     s3.name = "Ravi";
//     s3.age = 20;

//     // Print Object 1
//     cout << "Student 1" << endl;
//     cout << "Name : " << s1.name << endl;
//     cout << "Age  : " << s1.age << endl;

//     cout << endl;

//     // Print Object 2
//     cout << "Student 2" << endl;
//     cout << "Name : " << s2.name << endl;
//     cout << "Age  : " << s2.age << endl;

//     cout << endl;

//     // Print Object 3
//     cout << "Student 3" << endl;
//     cout << "Name : " << s3.name << endl;
//     cout << "Age  : " << s3.age << endl;

//     return 0;
// }



// #include<iostream>
// using namespace std;
// class emp
// {
//     public:
//     emp()
//     {
//          cout<<"constructor"<<endl;
//     }
//     ~emp()
//     {
//         cout<<"destroyer"<<endl;
//     }
//     void write()
//     {
//         cout<<"hi"<<endl;    }
// };
// int main()
// {
//     emp e1;
//     emp e2;
//     e1.write();
//     e1.write();
//     e1.write();
//     e2.write();
// }



// #include <iostream>
// using namespace std;
// string name;
// class Bank
// {
// private:
//     int balance;

// public:
//     void deposit(int amount)
//     {
//         balance = amount;
//     }

//     void showBalance()
//     {
//         cout << balance;
//     }
// };
// int main()
// {

// Bank b1;

// b1.deposit(5000);

// b1.showBalance();
// }





// #include <iostream>
// using namespace std;

// class Bank
// {
// private:
//     int balance;

// public:

//     void deposit(int amount)
//     {
//         balance = amount;
//     }

//     void showBalance()
//     {
//         cout << "Balance = " << balance << endl;
//     }
// };

// int main()
// {
//     Bank b1;

//     b1.deposit(5000);

//     b1.showBalance();

//     return 0;
// }



// #include <iostream>
// using namespace std;

// class Vehicle
// {
// public:

//     void start()
//     {
//         cout << "Vehicle Started" << endl;
//     }
// };

// class Car : public Vehicle
// {
// };

// int main()
// {
//     Car c1;

//     c1.start();

//     return 0;
// }




// #include <iostream>
// using namespace std;

// class Father
// {
// protected:
//     int money = 10000;
//     void play()
//     {
//         cout << "money"<<endl;
//     }

// public:
//     void showMoney()
//     {
//         cout << money << endl;
//     }
// };

// class Son : public Father
// {
// public:
//     void display()
//     {
//         cout << money;   
//     }
// };

// int main()
// {
//     Son s1;
//     Father f1;
//     s1.showMoney();
//     s1.display();
//     f1.showMoney();

// }



// #include <iostream>
// using namespace std;
// class Student
// {
//     public:
//     void set()
//     {
//         static int count=0;
//         cout<<count<<endl;
//         count++;
//     }
// };
// int main()
// {
//     Student s1;
//     s1.set();
//     s1.set();
//     s1.set();
//     return 0;
// }


// #include<iostream>
// using namespace std;
// class student
// {
//     public:
//     template<typename T>
//     T add(T a,T b)
//     {
//         T c;
//         c = a+b;
//         return c;
//     }

// };
// student s1;
// int main()
// {
//     int c;
//     c = s1.add( 10,20);
//     cout<< c <<endl;
    
// }


// 


#include <iostream>  //input output operations ku use pandranga
#include <vector> // dynamic array ku use pandranga
#include <memory>  // smart pointerku use anuranga

using namespace std; //cout,cin,endl nu use pana payanpaduthu

class Device // parent class
{
protected://prottected part
    string name;
    bool status;

public://public part
    Device(string n)//constructor 
    {
        name = n;
        status = false;
    }

    virtual void turnOn()//
    {
        status = true;
        cout << name << " ON" << endl;
    }

    virtual void turnOff()
    {
        status = false;
        cout << name << " OFF" << endl;
    }

    virtual void showStatus()
    {
        cout << name << " : ";

        if(status)
            cout << "ON";
        else
            cout << "OFF";

        cout << endl;
    }

    virtual ~Device() {}//destructor
};

class Light : public Device// child class
{
public:
    Light(string n) : Device(n) {}

    void turnOn() override
    {
        status = true;
        cout << "Light " << name << " Turned ON" << endl;
    }
};

class Fan : public Device// child class
{
public:
    Fan(string n) : Device(n) {}//

    void turnOn() override
    {
        status = true;
        cout << "Fan " << name << " Turned ON" << endl;
    }
};

int main()
{
    vector<unique_ptr<Device>> devices;

    devices.push_back(make_unique<Light>("Hall"));

    devices.push_back(make_unique<Fan>("Bedroom"));

    for(auto &d : devices)
    {
        d->turnOn();
    }

    cout << endl;

    for(auto &d : devices)
    {
        d->showStatus();
    }

    return 0;
}