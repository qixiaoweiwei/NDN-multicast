#include "FileClient.hpp"

using namespace std;
using namespace ndn;

int main()
{
  FileClient client;
  client.getFile("/File/PDF", "./consumer-files");
  return 0;
}
