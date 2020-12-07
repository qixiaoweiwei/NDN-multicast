#include <ndn-cxx/face.hpp>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

using namespace std;
using namespace ndn;
using namespace chrono;

class FileClient
{
public:
  void getFile(string name, string dir);
  
private:
  void onFile(const Interest&, const Data&, string);
  void onSegment(const Interest&, const Data&, int, char*);
  void onLastSegment(const Interest&, const Data&, int, char*, int*);
  void onNack(const Interest&, const lp::Nack&);
  void onTimeout(const Interest&);
  
private:
  char* m_buffer;
  int* m_size;
  int m_segment;
  int m_count = 0;
  steady_clock::time_point m_start;
  Face m_face;
};
