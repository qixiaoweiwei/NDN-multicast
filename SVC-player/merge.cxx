#include<iostream>
#include<fstream>
#include<cstring>
#include<cstdlib>
#include<map>
#include<vector>
#include<algorithm>

using namespace std;

unsigned char sep[4] = {'\x00', '\x00', '\x00', '\x01'};

int countNalus(string inFileName, int type=20)
{
  int cnt = 0;
  ifstream fpIn(inFileName, ios::in|ios::binary);
  fpIn.seekg(0, fpIn.end);
  int inLength = fpIn.tellg();
  fpIn.seekg(0, fpIn.beg);
  char* inContent = (char*)malloc(sizeof(char)*inLength);
  fpIn.read(inContent, inLength);
  fpIn.close();
  
  vector<unsigned char> stream(inContent, inContent+inLength);
  free(inContent);
  auto last = search(stream.begin(), stream.end(), sep, sep+4);
  int count = 0;
  while (last != stream.end())
  {
    auto next = search(last+4, stream.end(), sep, sep+4);
    unsigned char hdr = stream[last-stream.begin()+4];
    int naluType = hdr & 0x1f;
    if (naluType == type)
      cnt = cnt + 1;
    last = next;
  }
  
  return cnt;
}   

int mux(ofstream& fpOut, int nLayers, map<int, string>& fileNames, map<int, int>& naluCounts, int sepNaluType)
{
  map<int, map<int, vector<unsigned char>>> naluStreams;
  map<int, int> positions;
  for (int i=0; i<nLayers; i++)
  {
    ifstream fpIn(fileNames[i], ios::in|ios::binary);
    fpIn.seekg(0, fpIn.end);
    int inLength = fpIn.tellg();
    fpIn.seekg(0, fpIn.beg);
    char* inContent = (char*)malloc(sizeof(char)*inLength);
    fpIn.read(inContent, inLength);
    fpIn.close();
    vector<unsigned char> stream(inContent, inContent+inLength);
    free(inContent);
    auto last = search(stream.begin(), stream.end(), sep, sep+4);
    int count = 0;
    while (last != stream.end())
    {
      auto next = search(last+4, stream.end(), sep, sep+4);
      naluStreams[i][count].assign(last+4, next);
      last = next;
      count += 1;
    }
    positions[i] = 0;
  }
    
  bool active = true;
  int baseLayerAUCount = naluCounts[0];
  int frm = 0;
  int nal_ref_idc = 0;
  while (active)
  {
    active = false;
    for (int i=0; i<nLayers; i++)
    {
      int naluPerAU = naluCounts[i] / baseLayerAUCount;
      bool eos = false;
      bool found = false;
      bool first = true;
      if (i == 0)
      {
        int cnt = 0;
        while ((!eos) && (!found))
        {
          int pos = positions[i];
          if (pos >= naluStreams[i].size())
          {
            eos = true;
          } else {
            vector<unsigned char> n = naluStreams[i][pos];
            int naluType = 0;
            if (n.size() > 0)
            {
              nal_ref_idc = n[0] >> 5;
              naluType = n[0] & 0x1f;
              if (naluType == sepNaluType)
              {
                if (!first && nLayers > 1)
                {
                  found = true;
                  active = true;
                }
              }
              first = false;
            }
            if (naluType != 14)
              cnt += 1;
            if (!found)
            {
              fpOut.write((char*)sep, 4);
              fpOut.write((char*)n.data(), n.size());
              positions[i] += 1;
              if (nLayers == 1 && naluType == 1 && (nal_ref_idc == 2 || (nal_ref_idc == 0 && cnt >= naluPerAU)))
              {
                active = true;
                found = true;
              }
            }
          }
        }
        frm += 1;
      } else {
        int cnt = 0;
        while (!eos && cnt < naluPerAU)
        {
          int pos = positions[i];
          if (pos >= naluStreams[i].size())
          {
            eos = true;
            active = false;
          } else {
            vector<unsigned char> n = naluStreams[i][pos];
            cnt += 1;
            nal_ref_idc = n[0] >> 5;
            int naluType = n[0] & 0x1f;
            fpOut.write((char*)sep, 4);
            fpOut.write((char*)n.data(), n.size());
            positions[i] += 1;
            active = true;
          }
        }
      }
    }
  }
  return 0;
}

int merge(string* layerGroup, int numLayers, string outFile, string initLayer)
{ 
  ifstream fpInit(initLayer, ios::in|ios::binary);
  ofstream fpOut(outFile, ios::out|ios::binary);
  fpInit.seekg(0, fpInit.end);
  int initLayerLength = fpInit.tellg();
  fpInit.seekg(0, fpInit.beg);
  char* initLayerContent = (char*)malloc(sizeof(char)*initLayerLength);
  fpInit.read(initLayerContent, initLayerLength);
  fpInit.close();
  fpOut.write(initLayerContent, initLayerLength);
  free(initLayerContent);

  map<int, string> fileNames;
  map<int, int> naluCounts;
  int separatorNaluType = 6;
  for (int i=0; i<numLayers; i++)
  {
    string curFile = layerGroup[i];
    int naluCount = 0;
    if (i == 0)
    {
      naluCount = countNalus(curFile, 6);
      if (naluCount == 0)
      {
        naluCount = countNalus(curFile, 14);
        separatorNaluType = 14;
      }
    } else
      naluCount = countNalus(curFile, 20);
    fileNames[i] = curFile;
    naluCounts[i] = naluCount;
  }
  mux(fpOut, numLayers, fileNames, naluCounts, separatorNaluType);
  
  fpOut.close();
  
  return 0;
}
