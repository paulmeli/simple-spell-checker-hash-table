
/*
Simple spell checker using a hash table

This program implements a Map ADT using a hash table.
The entries to the map have string keys and NO value
Collisions in the hash table are handled by chaining into a list.
The program assumes that there exists a text file in the current directory:
input.txt

Change log:
2019-10-25 Boshen Wang initial version
11/8-11/12/2019 Melissa Paul implemented missing methods
*/

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <cctype> // Added by MP to get rid of tolower() error

using namespace std;

// Utility functions
void loadFile(string fname, ifstream& file)
{
    file.open(fname.c_str());
    if (file.fail())
    {
        cout << "Cannot open file " << fname << endl;
    }
}

// converts string to lowercase
string lowercase(string s)
{
    for (int i = 0; i < s.length(); i++)
    {
        s[i] = std::tolower(s[i]);
    }
    return s;
}

// Simple implementation of a Map ADT using a hash table.
// Entries consist of a string key (no whitespace), without a value.
// The table is represented using an array of linked lists in order to facilitate
// separate chaining collision handling.
class HashMap
{
public:
    HashMap();
    ~HashMap();
    // standard Map ADT functions
    int find(string key) const;
    void put(string key);
    void erase(string key);
    int size() const;
    void print() const;
    // additional functions
    void load(ifstream& file);
    void resizeTable(int s);
    void printStats() const;
    void setHashCodeMethod(string m);
private:
    enum HCM {poly, cyclic, simple, custom};
    HCM HashCodeMethod;
    int n;
    list <string>** table;
    int* inserts;
    int hashCodePoly(string key) const;
    int hashCodeSimple(string key) const;
    int hashCodeCyclic(string key) const;
    int hashCodeCustom(string key) const;
    int hashCompress(int code) const;
    int hash(string key) const;
    void deleteTable(list<string>** t, int s);
};

HashMap::HashMap()
{
    this->table = NULL;
    this->inserts = NULL;
    this->HashCodeMethod = simple;
    n = 0;
}

// NAME: Melissa Paul
// Hash code function using polynomial accumulation
// INPUT: a string key which needs to be hashed
// PRECONDITION: key is not null
// OUTPUT: An integer representing the input key. The same key must always
// produce the same output each time.
int HashMap::hashCodePoly(string key) const
{
    int sum = 0, a = 33, j = key.length() - 1; // a is the base, j is the exponent,
    // and key[i] - 96 is the coefficient
    for (int i = 0; i < key.length(); i++) {
        sum += (key[i] - 96) * pow(a, j);
        j--;
    }
    return sum;
}

// NAME: Melissa Paul
// Hash code function using a simple linear summation
// INPUT: a string key which needs to be hashed
// PRECONDITION: key is not null
// OUTPUT: An integer representing the input key. The same key must always
// produce the same output each time.
int HashMap::hashCodeSimple(string key) const
{
    int sum = 0;
    for (int i = 0; i < key.length(); i++) {
        sum += key[i] - 96; // Lowercase decimal value in ASCII - 96 = value in alphabet
    } // e.g., a = 1, b = 2,..., z = 26
    return sum;
}

// NAME: Melissa Paul
// Hash code function using a cyclic bit shift
// INPUT: a string key which needs to be hashed
// PRECONDITION: key is not null
// OUTPUT: An integer representing the input key. The same key must always
// produce the same output each time.
int HashMap::hashCodeCyclic(string key) const // Based off pseudocode from p. 379 in textbook
{
    unsigned int sum = 0;
    for (int i = 0; i < key.length(); i++) { // 5-bit cyclic shift we form bitwise or
        sum = (sum << 5) | (sum >> 27); // of a 5 - bit left shift and a 27 - bit right shift
        sum += (unsigned int) key[i]; // Add string character key[i]
    }
    return int(sum);
}

// NAME: Melissa Paul
// Hash code function using an exponential summation.
// INPUT: a string key which needs to be hashed
// PRECONDITION: key is not null
// OUTPUT: An integer representing the input key. The same key must always
// produce the same output each time.
int HashMap::hashCodeCustom(string key) const
{
    int sum = 0, j = key.length();
    for (int i = 0; i < key.length(); i++) {
        sum += pow((key[i] - 92), j); // Exponential sum
        j--;
    }
    return sum;
}

// NAME: Melissa Paul
// INPUT: an integer hash code representing a string key
// OUTPUT: An integer in the range [0-n] where n is the size of the hash table.
// The same input hash code must produce the same output each time.
int HashMap::hashCompress(int code) const
{ // a ("scale") = 7, b ("shift") = 103, N = 109345121
    return (abs((7 * code) + 103) % 109345121) % this->n; // h(k) = | ak + b | mod N
}

// Function that consistently maps any given input string key to an integer corresponding to a bucket in the
// hash table.
// The hash code method used depends on the current value of this.HashCodeMethod
// INPUT: a string key which needs to be hashed
// OUTPUT: An integer in the range [0-n] where n is the size of the hash table.
// The same input string key must produce the same output each time.
int HashMap::hash(string key) const
{
    int code;
    if (this->HashCodeMethod == simple)
    {
        code = this->hashCodeSimple(key);
    }
    if (this->HashCodeMethod == poly)
    {
        code = this->hashCodePoly(key);
    }
    if (this->HashCodeMethod == cyclic)
    {
        code = this->hashCodeCyclic(key);
    }
    if (this->HashCodeMethod == custom)
    {
        code = this->hashCodeCustom(key);
    }
    return this->hashCompress(code) % this->n;
}

// INPUT: a string key
// OUTPUT: If the key exists in the table, return the index of the bucket containing the key
// Otherwise, return -1
int HashMap::find(string key) const
{
    // find the right bucket
    int bucketIdx = this->hash(key);

    // find the key inside bucket
    list<string>::iterator it;
    list<string>::iterator bucketBegin = this->table[bucketIdx]->begin();
    list<string>::iterator bucketEnd = this->table[bucketIdx]->end();
    it = std::find(bucketBegin, bucketEnd, key);
    if (it != bucketEnd)
    {
        return bucketIdx;
    }
    else
    {
        return -1;
    }
}

// NAME: Melissa Paul
// INPUT: a string key
// PRECONDITION: Key is not null and either exists in the table or needs to be inserted.
// POSTCONDITION: Key is hashed and placed at the bottom of the appropriate bucket in the hash table.
void HashMap::put(string key)
{
    int bucketIdx = this->find(key); // Look if key already in table
    if (bucketIdx == -1) { // If not found, insert
        bucketIdx = this->hash(key);
        this->table[bucketIdx]->push_back(key); // don't forget to update this->inserts
        this->inserts[bucketIdx]++;
    } // else, do nothing (no value to update)
}

// NAME: Melissa Paul
// INPUT: a string key
// PRECONDITION: Key is not null and either is or isn't in the table.
// POSTCONDITION: Key is removed from the table if it existed; otherwise, nothing happens if the key
// wasn't in the table.
void HashMap::erase(string key)
{
    int bucketIdx = this->find(key); // Look if key is in table
    if (bucketIdx) { // If found, remove and update this->inserts
        this->table[bucketIdx]->remove(key);
        this->inserts[bucketIdx]--;
    } // else, do nothing
}

// Resizes the array of lists representing the hash table, then rehashes all existing entries into the new table
// INPUT: new size s of the hash table
// PRECONDITION: s is positive
// POSTCONDITION: the hash table is now size s, and all previous entries exist in the new table
void HashMap::resizeTable(int s)
{
    // remember old table
    list<string>** oldTable = this->table;
    int old_n = this->n;
    // reset stats
    delete[] this->inserts;
    this->inserts = new int[s];
    for (int i = 0; i < s; i++)
    {
        this->inserts[i] = 0;
    }
    // initialize new table
    this->n = s;
    this->table = new list<string> * [s];
    for (int i = 0; i < s; i++)
    {
        this->table[i] = new list<string>;
    }
    // re-insert everything from the old table into the new one
    if (oldTable)
    {
        for (int i = 0; i < old_n; i++)
        {
            list<string>* curList = oldTable[i];
            for (list<string>::iterator it = curList->begin(); it != curList->end(); it++)
            {
                this->put(*it);
            }
        }
        this->deleteTable(oldTable, old_n);
    }
}

// C++ only: deletes the current hash table from memory
// INPUT: (optional) pointer to table to be deleted, (optional) size of that table
void HashMap::deleteTable(list<string>** t = NULL, int s = 0)
{
// default values
    if (!t)
    {
        t = this->table;
    }

    if (s == 0)
    {
        s = this->n;
    }

    for (int i = 0; i < s; i++)
    {
        delete t[i];
    }

    delete t;
}

// OUTPUT: size of the hash table
int HashMap::size() const
{
    return this->n;
}

// OUTPUT: the contents of every bucket in the hash table are printed to the screen, one line per bucket
void HashMap::print() const
{
    for (int i = 0; i < this->n; i++)
    {
        list<string>* curList = this->table[i];
        cout << i << ":\t";
        for (list<string>::iterator it = curList->begin(); it != curList->end(); it++)
        {
            cout << *it << "\t";
        }
        cout << endl;
    }
}

// INPUT: a text file containing input string keys, one per line (no whitespace)
// PRECONDITION: the current hash table has been initalized (resized)
// POSTCONDITION: all keys in the input file are inserted into the hash table
void HashMap::load(ifstream& file)
{
    string line;
    while (getline(file, line))
    {
        line = lowercase(line);
        // trim whitespace
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        this->put(line);
    }
}

// OUTPUT: the following values are printed to the screen:
// size: size of the hash table
// inserts: # of insertions into the hash table
// load factor: load factor of the table (inserts/size)
// collisions: # of collisions encountered during insertions
// max. bucket: # of keys in the largest bucket
void HashMap::printStats() const
{
    int sumIns = std::accumulate(this->inserts, this->inserts + this->n, 0);
    int* collisions = new int[this->n];
    for (int i = 0; i < this->n; i++)
    {
        collisions[i] = std::max(this->inserts[i] - 1, 0);
    }
    int sumColl = std::accumulate(collisions, collisions + this->n, 0);
    cout << "size:\t\t\t" << this->n << endl;
    cout << "inserts:\t\t" << sumIns << endl;
    cout << "load factor:\t" << double(sumIns) / double(this->n) << endl;
    cout << "collisions:\t\t" << sumColl << endl;
    cout << "max. bucket:\t" << *std::max_element(this->inserts, this->inserts + this->n) << endl;
}

// INPUT: a string m representing one of the hash code implementations
// PRECONDITION: m must be one of {"poly", "simple", "cyclic", "custom"}
// POSTCONDITION: the hash table will use the specified hash code function when hashing
void HashMap::setHashCodeMethod(string m)
{
    if (m == "poly")
    {
        this->HashCodeMethod = poly;
    }

    if (m == "simple")
    {
        this->HashCodeMethod = simple;
    }

    if (m == "cyclic")
    {
        this->HashCodeMethod = cyclic;
    }

    if (m == "custom")
    {
        this->HashCodeMethod = custom;
    }
}

HashMap::~HashMap()
{
    this->deleteTable();
}

int main()
{
    string inputFilename = "input.txt";
    string line;
    HashMap H = HashMap();

    // open input file
    ifstream inputFile;
    loadFile(inputFilename, inputFile);
    while (getline(inputFile, line))
    {
        // echo input
        cout << line << endl;

        // parse input using a stringstream
        stringstream lineSS(line);
        string token;
        string command;

        while (getline(lineSS, token, ' '))
        {
            // trim whitespace
            token.erase(token.find_last_not_of(" \n\r\t") + 1);

            // first token is the command
            if (command.empty())
            {
                token = lowercase(token);
                command = token;
                if (command == "check")
                {
                    cout << "misspelled:";
                }
                continue;
            }

            // subsequent tokens are associated with that command
            if (command == "resize")
            {
                H.resizeTable(atoi(token.c_str()));
            }
            if (command == "load")
            {
                ifstream wordsFile;
                loadFile(token, wordsFile);
                H.load(wordsFile);
                wordsFile.close();
            }
            if (command == "put")
            {
                token = lowercase(token);
                H.put(token);
            }
            if (command == "find")
            {
                token = lowercase(token);
                int bucketIdx = H.find(token);
                cout << token << ": ";
                if (bucketIdx >= 0)
                {
                    cout << "found " << bucketIdx << endl;
                }
                else
                {
                    cout << "not found" << endl;
                }
            }
            if (command == "erase")
            {
                token = lowercase(token);
                H.erase(token);
            }
            if (command == "check")
            {
                token = lowercase(token);
                int bucketIdx = H.find(token);
                if (bucketIdx < 0)
                {
                    cout << "\t" << token;
                }
            }
            if (command == "hash_code")
            {
                token = lowercase(token);
                H.setHashCodeMethod(token);
            }
        }

        // print doesn't have additional tokens
        if (command == "print")
        {
            H.print();
        }
        if (command == "stats")
        {
            H.printStats();
        }
        if (command == "rehash")
        {
            H.resizeTable(H.size());
        }
        if (command == "check")
        {
            cout << endl;
        }
    }

    inputFile.close();
    system("pause"); // Added by MP
    return EXIT_SUCCESS;
}