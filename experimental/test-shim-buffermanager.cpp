#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <random>
#include <string.h>
//#include <bslstl_string.h>
//#include <bdlma_bufferedsequentialallocator.h>
#include <bdlma_buffermanager.h>
#include <string>
#include <cstring>
#include <memory>
#include <chrono>
#include <ratio>
#include <ctime>

#include "shim_buffermanager.hpp"
#include "litterer.hpp"

#include "cxxopts.hpp"

int main(int argc, char * argv[])
{
  cxxopts::Options options(argv[0], "");
  options.add_options()
    ("help","Print command-line options")
    ("shuffle","Shuffle ('litter') randomly before starting")
    ("buffer","Use the actual buffer implementation")
    ("shim","Use a shim buffer implementation")
    ("object-size","Size of objects to allocate", cxxopts::value<int>())
    ("working-set","Size of working set (in bytes)", cxxopts::value<int>())
    ("loops","Number of loops", cxxopts::value<int>())
    ("locality-iterations","Locality iterations (non-allocating)", cxxopts::value<int>());

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help({""}) << std::endl;
    return 0;
  }
  
  int ObjectSize = 64; // was 64
  if (result.count("object-size")) {
    ObjectSize = result["object-size"].as<int>();
  }

  int WorkingSet = 256000; // was 64000
  if (result.count("working-set")) {
    WorkingSet = result["working-set"].as<int>();
  }
  int Iterations = WorkingSet / ObjectSize;
  
  int Loops = 1000;
  if (result.count("loops")) {
    Loops = result["loops"].as<int>();
  }
  
  using namespace std::chrono;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  volatile Litterer frag (ObjectSize, ObjectSize, 10000000, 1, 10, result.count("shuffle"));
  
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  
  if (result.count("buffer")) {
    std::cout << "using BufferManager ";
  } else {
    std::cout << "using ShimBufferManager ";
  }

  int localityIterations = 1;
  if (result.count("locality-iterations")) {
    localityIterations = result["locality-iterations"].as<int>();
  }

  if (result.count("shuffle")) {
    std::cout << "(shuffled) ";
  }
  std::cout << "working set = " << WorkingSet << " bytes ";
  
  std::cout << std::endl;

  using namespace BloombergLP;
  auto buf = new char[Iterations * ObjectSize];
  auto ptrs = new volatile void * [Iterations];
  //    for (auto it = 0; it < 10000000; it++) {
  BloombergLP::bdlma::BufferManager mgr_buffer(buf, Iterations*ObjectSize);
  bdlma::ShimBufferManager mgr_shim(nullptr, Iterations);
  auto which_buf = result.count("buffer");
  
  for (volatile auto it = 0; it < Loops; it++) {
    
    for (volatile auto i = 0; i < Iterations; i++) {
      volatile char * ch;
      volatile int value = 13;
      if (which_buf) {
	ch = (char *) mgr_buffer.allocate(ObjectSize);
      } else {
	ch = (char *) mgr_shim.allocate(ObjectSize);
      }
      memset((void *) ch, value, ObjectSize);
      ptrs[i] = (void *) ch;
    }
    volatile char ch;
    for (volatile auto it = 0; it < localityIterations; it++) {
      for (volatile auto i = 0; i < Iterations; i++) {
	volatile char bufx[ObjectSize];
	for (auto j = 0; j < ObjectSize; j++) {
	  bufx[j] = * ((char *) ptrs[j]);
	}
	//	memcpy((void *) bufx, (void *) ptrs[i], ObjectSize);
	ch += buf[ObjectSize-1];
      }
    }

    mgr_buffer.release();
    mgr_shim.release();
  }
  delete [] ptrs;
  delete [] buf;
  high_resolution_clock::time_point t3 = high_resolution_clock::now();
  duration<double> time_span2 = duration_cast<duration<double>>(t3 - t1);
  std::cout << "Time elapsed = " << time_span2.count() - time_span.count() << std::endl;
  return 0;
}
