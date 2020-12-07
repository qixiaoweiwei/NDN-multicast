%module client
%include "std_string.i"

%{
#define SWIG_FILE_WITH_INIT
#include "client.hpp"
%}

class FileClient
{
public:
  void getFile(std::string name, std::string dir);
  
private:
  void onFile(const Interest&, const Data&, std::string);
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
