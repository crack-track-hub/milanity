#include <iostream>
#include <map>       // multimap பயன்படுத்தவும் map ஹெடரே போதும்
#include <string>

using namespace std;

int main()
{
    //----------------------------------------------------
    // STEP 1 : Create a Multimap (உருவாக்குதல்)
    //----------------------------------------------------
    multimap<string, string> dict;

    //----------------------------------------------------
    // STEP 2 : insert() (மதிப்புகளை உள்ளே போடுதல்)
    //----------------------------------------------------
    dict.insert({"Smart", "Intelligent"});
    dict.insert({"Bark", "Sound of a dog"});
    dict.insert({"Smart", "Active"});          // Duplicate Key - 'Smart'
    dict.insert({"Bark", "Outer layer of tree"}); // Duplicate Key - 'Bark'

    //----------------------------------------------------
    // STEP 3 : Display Elements (மதிப்புகளைக் காட்டுதல்)
    //----------------------------------------------------
    cout << "--- All Dictionary Words (Sorted & Duplicates Allowed) ---" << endl;
    for(auto x : dict)
    {
        cout << x.first << " -> " << x.second << endl;
    }
    cout << endl;

    //----------------------------------------------------
    // STEP 4 : size() (ஜோடிகளின் எண்ணிக்கை)
    //----------------------------------------------------
    cout << "Total Pairs (Size) = " << dict.size() << endl;

    //----------------------------------------------------
    // STEP 5 : count() (ஒரே Key எத்தனை முறை உள்ளது என எண்ணுதல்)
    //----------------------------------------------------
    cout << "Number of meanings for 'Bark' = " << dict.count("Bark") << endl;
    cout << endl;

    //----------------------------------------------------
    // STEP 6 : find() (ஒரு Key-ஐத் தேடுதல்)
    //----------------------------------------------------
    auto it = dict.find("Smart");
    if(it != dict.end())
    {
        cout << "Found 'Smart'! First meaning = " << it->second << endl;
    }
    cout << endl;

    //----------------------------------------------------
    // STEP 7 : erase() - நீக்குதல்
    //----------------------------------------------------
    // find செய்த 'it' வேரியபிளை மட்டும் அனுப்புவதால் முதல் 'Smart' மட்டும் அழியும்
    dict.erase(it); 

    cout << "--- After Erasing Single 'Smart' Pair ---" << endl;
    for(auto x : dict) 
    {
        cout << x.first << " -> " << x.second << endl;
    }
    cout << endl;

    //----------------------------------------------------
    // STEP 8 : clear() (மொத்தமாக காலி செய்தல்)
    //----------------------------------------------------
    dict.clear();
    cout << "Size after clear() = " << dict.size() << endl;

    return 0;
}