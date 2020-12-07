#include "client.hpp"

#define MAX_SEGMENT_SIZE 8000


using namespace std;
using namespace ndn;
using namespace chrono;

void FileClient::getFile(string name, string dir)
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
    cout<<"All segments received at "<<(double)duration.count()/1000000 <<" seconds."<<endl;
}

void FileClient::onFile(const Interest&, const Data& data, string fileName)
{
    int segmentNumber = stoi(reinterpret_cast<const char*>(data.getContent().value()));
    char* buffer = (char*)malloc(sizeof(char)*segmentNumber*MAX_SEGMENT_SIZE);
    int* lastSize = (int*)malloc(sizeof(int));
    m_buffer = buffer;
    m_size = lastSize;
    m_segment = segmentNumber-1;
    
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
    
    Name interestName = Name("/Segment"+fileName+"/"+to_string(segmentNumber-1));
    Interest interest(interestName);
    interest.setCanBePrefix(false);
    interest.setInterestLifetime(100_s);
    m_face.expressDumbInterest(interest,
                         bind(&FileClient::onLastSegment, this,  _1, _2, m_segment, buffer, lastSize),
                         bind(&FileClient::onNack, this, _1, _2),
                         bind(&FileClient::onTimeout, this, _1));
}
  
void FileClient::onSegment(const Interest&, const Data& data, int segment, char* buffer)
{
    m_count += 1;
    
    const char* content = reinterpret_cast<const char*>(data.getContent().value());
    int bias = segment*MAX_SEGMENT_SIZE;
    for (int i=0; i<data.getContent().value_size(); i++)
      buffer[i+bias] = content[i];
}
  
void FileClient::onLastSegment(const Interest&, const Data& data, int segment, char* buffer, int* lastSize)
{
    m_count += 1;
    
    const char* content = reinterpret_cast<const char*>(data.getContent().value());
    int bias = segment*MAX_SEGMENT_SIZE;
    for (int i=0; i<data.getContent().value_size(); i++)
      buffer[i+bias] = content[i];
    
    *lastSize = data.getContent().value_size();
}

void FileClient::onNack(const Interest&, const lp::Nack& nack)
{
    std::cout << "Received Nack with reason " << nack.getReason() << std::endl;
}
  
void FileClient::onTimeout(const Interest& interest)
{
    std::cout << "Timeout for " << interest << std::endl;
}
