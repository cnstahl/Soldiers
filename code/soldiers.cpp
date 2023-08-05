#include <bitset>
#include <iostream>
#include <cassert>
#include <fstream>
#include <cmath>
#include <random>

typedef std::size_t length_t, position_t;
typedef unsigned long ulong;
//typedef std::bitset<length> bitstring;

const int length  = 6000;    // System size
const int steps   = 100;    // Time limit (not used in relax)
const int runs    = 100;    // Number of runs
float prob  = .0;          // Probability of failure
const int doHist  = 0;     // Bool: print history (for images)
const int doStats = 0;     // Bool: print statistics
const int doRelax = 1;     // Bool: just print the relaxation time
const length_t critical = length / 2 - llround( sqrt(length));
// assert(initState.length() == length)

// Keep state and buffer global
std::bitset<length> state; // initial state all 0s
int buffer = 0;
int firstThree = 0;  // Need to keep the values of the first three bits

// declare random number generator outside of main
std::random_device rd;  // Will be used to obtain a seed for the random number engine
std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
std::uniform_real_distribution<float> dis(0.0, 1.0);

// Lookup table
bool lookup[1<<7];
void fillLookup() {
  for (int i = 0; i < 1<<7; i++) {
    //    std::cout << i << ", " << (i>>3) << ", " << (!(i>>2)) << ", " << (!i) << ", " << ( (i>>3) & ~(i>>2) & ~(i)    & 1) << ", ";
    // if current bit is 1 and its right and third right neighbor are 0, change to 0
    if      ( (i>>3) & ~(i>>2) & ~(i)    & 1) {lookup[i]=0;}
    // if current bit is 0 and its left  and third left  neighbor are 1, change to 1
    else if (~(i>>3) &  (i>>4) &  (i>>6) & 1) {lookup[i]=1;}
    // else just stay
    else                          {lookup[i] = ((i>>3) &1);}
    //std::cout << lookup[i] << "\n";
  }
}

// Update the state using a moving window buffer
void update() {

  // Build a 7-bit buffer to feed to the lookup table
  // buffer += state[         3] << 6; // Do this in the loop instead
  buffer = 0;
  buffer += state[         2] << 6;
  buffer += state[         1] << 5;
  buffer += state[         0] << 4;
  buffer += state[length - 1] << 3;
  buffer += state[length - 2] << 2;
  buffer += state[length - 3] << 1;
  // std::cout << buffer << "\n";

  // Build a 3-bit buffer of the first three bits to use when we reach the end
  firstThree = 0;
  firstThree += state[2] << 2;
  firstThree += state[1] << 1;
  firstThree += state[0];
  
  for (int i = 0; i < length-3; i++) {
    buffer = (buffer>>1) + (state[i+3]<<6);
    //random update
    if (dis(gen) < prob) {
      state.flip(position_t(i));
    }
    else {
      state.set(position_t(i), lookup[buffer]);
    }
  }
  //std::cout << "\n";

  // Last three bits
  for (int i = 0; i < 3; i++) {
    buffer = (buffer>>1) + ((firstThree & (1<<i)) << (6-i));
    if (dis(gen) < prob) {
      state.flip(position_t(length-3+i));
    }
    else {
      state.set(position_t(length-3+i), lookup[buffer]);
    }
  }
}

void printHistory() {
  std::ofstream myfile;
  myfile.open ("../data/history.bin");
  
  for (int i = (length / 3); i < (2 * length / 3); i++) {
   state.flip(position_t(i));
  }

  myfile << state << "\n";

  for (int i = 1; i < steps; i++) { //start at 1 so there are #time lines
    update();
    myfile << state << "\n";
  }
  
  myfile.close();
}

void printStats() {
  std::ofstream myfile;
  myfile.open("../data/stats.dat");
  
  for (int j = 0; j < runs; j++) {
    state.reset();
    myfile << state.count();

    for (int i = 1; i < steps; i++) {
      update();
      myfile << ", " << state.count();
    }
    myfile << "\n";
  }
  myfile.close();
}

int findRelax() {
  for (int i = 1; i != 0; i++) {
    update();
    if (state.count() > critical) { return i;}
  }
  return 0;
}
  
void printRelax() {
  std::ofstream myfile;

  for (int j = 0; j < runs; j++) {
    myfile.open("../data/relaxL" + std::to_string(length) +
		"p" + std::to_string(prob) + ".dat", std::ios_base::app);
    state.reset();
    myfile << findRelax() << ", ";
    myfile << "\n";
    myfile.close();
  }
  std::cout << "Finished p = " << prob << "\n";
}

void bigTest() {
  for (int j = 0; j < (1<<7); j++) {
    state = std::bitset<length>(j);

    std::cout << state << ", ";

    for (int i = 1; i < steps; i++) { //start at 1 so there are #time lines
    
      update();

      std::cout << state << "\n";
    }
  }
}


int main(int argc, char *argv[]) {
  // assert(initState.length() == length)
  assert(doStats + doHist + doRelax == 1);

  // If there is a command line arg, assume it is a probability percent as an integer
  if (argc > 1) {
    prob = atof(argv[1]) / 100;
    // std::cout << prob << "\n";
  }

  fillLookup();

  if (doStats) {printStats();}
  if (doHist)  {printHistory();}
  if (doRelax) {printRelax();}
  //  bigTest();
}
