#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <thread>

#define MAX_SEGMENT_SIZE 8000

using namespace ndn;
using namespace std;

mutex mtx;
unique_lock<mutex> lck(mtx);
condition_variable cv;
string sharedFileName;
int sharedSegment;

class FileServer
{
public:
  void
  run()
  {
    m_face.setInterestFilter("/File",
                             bind(&FileServer::onFileInterest, this, _1, _2, "./producer-files"),
                             nullptr,
                             bind(&FileServer::onRegisterFailed, this, _1, _2));
    m_face.setInterestFilter("/Segment",
                             bind(&FileServer::onSegmentInterest, this, _1, _2, "./producer-files"),
                             nullptr,
                             bind(&FileServer::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
  }

private:
  void
  onFileInterest(const InterestFilter&, const Interest& interest, string dir)
  {
    string fileName = interest.getName().getSubName(1, 1).toUri();
    sharedFileName = fileName;
    string fileFullName = dir + fileName;
    int fileSize = getFileSize(fileFullName);
    int segmentCount = fileSize/MAX_SEGMENT_SIZE;
    if (fileSize % MAX_SEGMENT_SIZE != 0)
      segmentCount += 1;
    sharedSegment = segmentCount;
    auto sliceNumber = to_string(segmentCount);
    auto data = make_shared<Data>(interest.getName());
    data->setContent(reinterpret_cast<const uint8_t*>(sliceNumber.data()), sliceNumber.size());
    m_keyChain.sign(*data);
    m_face.put(*data);
    cv.notify_all();
  }
  
  void
  onSegmentInterest(const InterestFilter&, const Interest& interest, string dir)
  {
    string fileName = interest.getName().getSubName(1, 1).toUri();
    std::cout<<"interest.getName() is "<<interest.getName()<<std::endl;
    int segmentNumber = stoi(interest.getName().getSubName(2, 1).toUri().substr(1));
    std::cout<<"segmentNumber is "<<segmentNumber<<std::endl;
    string fileFullName = dir + fileName;
    
    ifstream file(fileFullName, ios::in|ios::binary);
    if(file.is_open())
    {
      file.seekg(segmentNumber*MAX_SEGMENT_SIZE, ios::beg);
      vector<uint8_t> buffer(MAX_SEGMENT_SIZE);
      file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
      auto nCharsRead = file.gcount();
      auto name = Name("/Segment"+fileName+"/"+to_string(segmentNumber));
      auto data = make_shared<Data>(name);
      data->setContent(buffer.data(), static_cast<size_t>(nCharsRead));
      m_keyChain.sign(*data);
      m_face.put(*data);
      file.close();
    }
  }

  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix '" << prefix
              << "' with the local forwarder (" << reason << ")" << std::endl;
    m_face.shutdown();
  }
  
private:
  int
  getFileSize(string fileName)
  {
    ifstream file(fileName, ios::in|ios::binary);
    if(file.is_open())
    {
      file.seekg(0, ios::end);
      return file.tellg();
    }
    return -1;
  }

private:
  Face m_face;
  KeyChain m_keyChain;
};


class FilePusher
{
public:
  void
  run()
  {
    usleep(100000);
    for (int i=0; i < sharedSegment; i++)
    {
      if (i%10 == 9)
        usleep(1000);
      pushData(sharedFileName, i);
    }
  }
  
  void
  pushData(string fileName, int segment)
  {
    ifstream file("./producer-files"+fileName, ios::in|ios::binary);
    if(file.is_open())
    {
      file.seekg(segment*MAX_SEGMENT_SIZE, ios::beg);
      vector<uint8_t> buffer(MAX_SEGMENT_SIZE);
      file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
      auto nCharsRead = file.gcount();
      auto name = Name("/Segment"+fileName+"/"+to_string(segment));
      auto data = make_shared<Data>(name);
      data->setContent(buffer.data(), static_cast<size_t>(nCharsRead));
      m_keyChain.sign(*data);
      m_face.push(*data);
      file.close();
    }
  }

private:
  Face m_face;
  KeyChain m_keyChain;
};


static void server()
{
  FileServer fileServer;
  fileServer.run();
}

static void pusher()
{
  FilePusher filePusher;
  while (true)
  {
    cv.wait(lck);
    filePusher.run();
  }
}


int
main(int argc, char** argv)
{
  thread server_thread(server);
  thread pusher_thread(pusher);
  server_thread.join();
  pusher_thread.join();
  return 0;
}
