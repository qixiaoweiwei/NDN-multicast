#include <ndn-cxx/face.hpp>

#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>


#include<chrono>

#define MAX_SEGMENT_SIZE 8000


using namespace std;
using namespace ndn;
using namespace chrono;

class FileClient
{
public:
  void
  getFile(string name, string dir)
  {
    //Record start_time:
    m_start = steady_clock::now();
    
    //Get file:
    Name interestName(name);
    string fileName = interestName.getSubName(1,1).toUri();
    Interest interest(interestName);
    interest.setCanBePrefix(false);
    interest.setInterestLifetime(100_s);
    m_face.expressInterest(interest,
                           bind(&FileClient::onFile, this,  _1, _2, fileName),
                           bind(&FileClient::onNack, this, _1, _2),
                           bind(&FileClient::onTimeout, this, _1));
    m_face.processEvents();
    
    //Write out filestream:
    ofstream file(dir+fileName, ios::out|ios::binary);
    if (file.is_open())
      file.write(m_buffer, m_segment*MAX_SEGMENT_SIZE+*m_size);
    file.close();

    //Calculate transmission time:
    auto end = steady_clock::now();
    auto duration = duration_cast<microseconds>(end-m_start);
  }

  void
  onFile(const Interest&, const Data& data, string fileName)
  { 
    //Allocate needed variables:
    int segmentNumber = stoi(reinterpret_cast<const char*>(data.getContent().value()));
    char* buffer = (char*)malloc(sizeof(char)*segmentNumber*MAX_SEGMENT_SIZE);
    int* lastSize = (int*)malloc(sizeof(int));
    m_buffer = buffer;
    m_size = lastSize;
    m_segment = segmentNumber-1;
    
    //Send interests except the last one:
    for (int segment=0; segment<m_segment; segment++)
    {
      Name interestName = Name("/Segment"+fileName+"/"+to_string(segment));
      Interest interest(interestName);
      interest.setCanBePrefix(false);
      interest.setInterestLifetime(100_s);
      m_face.expressDumbInterest(interest,
                           bind(&FileClient::onSegment, this,  _1, _2, segment, buffer),
                           bind(&FileClient::onNack, this, _1, _2),
                           bind(&FileClient::onTimeout, this, _1));
    }
    
    //Send last interest:
    Name interestName = Name("/Segment"+fileName+"/"+to_string(segmentNumber-1));
    Interest interest(interestName);
    interest.setCanBePrefix(false);
    interest.setInterestLifetime(100_s);
    m_face.expressDumbInterest(interest,
                         bind(&FileClient::onLastSegment, this,  _1, _2, m_segment, buffer, lastSize),
                         bind(&FileClient::onNack, this, _1, _2),
                         bind(&FileClient::onTimeout, this, _1));
  }
  
  void
  onSegment(const Interest&, const Data& data, int segment, char* buffer)
  {
    //Increase segment count:
    m_count += 1;
    
    //Calculate delay:
    auto end = steady_clock::now();
    auto duration = duration_cast<microseconds>(end-m_start);
    cout<<"Segment "<<segment<<" received at "<<(double)duration.count()/1000000 <<" seconds."<<endl;
    cout<<m_count<<" out of "<<m_segment+1<<" segments received."<<endl;
    
    //Store segment into buffer:
    const char* content = reinterpret_cast<const char*>(data.getContent().value());
    int bias = segment*MAX_SEGMENT_SIZE;
    for (int i=0; i<data.getContent().value_size(); i++)
      buffer[i+bias] = content[i];
  }
  
  void
  onLastSegment(const Interest&, const Data& data, int segment, char* buffer, int* lastSize)
  {
    //Increase segment count:
    m_count += 1;
    
    //Calculate delay:
    auto end = steady_clock::now();
    auto duration = duration_cast<microseconds>(end-m_start);
    cout<<"Last segment received at "<<(double)duration.count()/1000000 <<" seconds."<<endl;
    cout<<m_count<<" out of "<<m_segment+1<<" segments received."<<endl;
    
    //Store segment into buffer:
    const char* content = reinterpret_cast<const char*>(data.getContent().value());
    int bias = segment*MAX_SEGMENT_SIZE;
    for (int i=0; i<data.getContent().value_size(); i++)
      buffer[i+bias] = content[i];
      
    //Store size of the last segment:
    *lastSize = data.getContent().value_size();
  }

  void
  onNack(const Interest&, const lp::Nack& nack)
  {
    std::cout << "Received Nack with reason " << nack.getReason() << std::endl;
  }
  
  void
  onTimeout(const Interest& interest)
  {
    std::cout << "Timeout for " << interest << std::endl;
  }
  
private:
  char* m_buffer;
  int* m_size;
  int m_segment;
  int m_count = 0;
  steady_clock::time_point m_start;

public:
  Face m_face;
};
