/*
 * This is just a quick program to help with testing the python implementation.
 *
 * It writes the random number sequence used by FAST-TrIPs to a file for the python
 * implementation to use in order to produce consistent results.
 *
 * The output file is rand.txt and the first number written is RAND_MAX, followed by the
 * given number of rand() numbers.
 */

#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char *argv[]) {
    string USAGE("Usage: rand.exe num_rands\n\nCreates file rand.txt\n");
    string filename("rand.txt");

    if (argc != 2) {
        cerr << USAGE << endl;
        return 2;
    }

    int num_rand = atoi(argv[1]);
    cout << "Writing RAND_MAX followed by " << num_rand << " rand() numbers into " << filename << endl;

    ofstream randfile;
    randfile.open(filename.c_str(), ios::out);
    randfile << RAND_MAX << endl;
    for (int i=0; i<num_rand; ++i) {
        randfile << rand() << endl;
    }
    randfile.close();
    return 0;
 }