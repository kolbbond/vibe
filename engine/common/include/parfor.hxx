// parfor implementation taken from Van Nugteren
// 240501

#ifndef PARFOR_HH
#define PARFOR_HH

#include <atomic>
#include <future>
#include <thread>
#include <vector>

// parfor loop can be made as such
// from 10 to 20, boolean for parallel
// [&](){} is lambda function
// parfor(10,20,true,[&](int idx, int cpu) {
//
//  printf("task %d running on cpu %d\n", idx, cpu);
//
//      }
//    );
// get number of available cpus for
// int parfor_num_cpus(){
// 	return std::thread::hardware_concurrency();
// }

// template class - not armadillo specific
template <class F> void parfor(int begin, int end, bool enable_parallel, F fn) {

  // make index
  std::atomic<int> idx;
  idx = begin;

  // fire asynchronous threads
  // except when loop only has one walk
  if (enable_parallel == true && end - begin > 0) {

    // num cpus
    int num_cpus = std::thread::hardware_concurrency();
    //printf("num_cpus: %i\n",num_cpus);

    // override with environment variable
    // char* str = std::getenv("VIBE_NUM_THREADS");
    // if(str!=NULL)num_cpus = std::stoi(std::string(str));

    // calculate actual num threads
    const int num_futures = int(std::min(num_cpus, end - begin + 1));

    //printf("num_futures: %i\n",num_futures);

    // get the futures
    std::vector<std::future<void>> futures(num_futures);

    // walk over cpus
    for (int cpu = 0; cpu != num_futures; cpu++) {
        printf("cpu: %i\n",cpu);

      futures[cpu] = std::async(std::launch::async, [cpu, &idx, end, &fn]() {

        // keep firing until we run out of idx
        // forever loop
        for (;;) {
          // atomic increment of index so
          // no two threads can do same task
          int i = idx++;

          // check if last index
          if (i >= end) {
            // early out
            break;
          }

          // run lamba function with i index
          fn(i, cpu);
        }
      }); // async function end

    } // for loop end

    // confirm threads are done
    for (int cpu = 0; cpu != num_futures; cpu++) {
      futures[cpu].get();
    }
  }

  // else just run as regular loop
  else {
      printf("non-parallel loop\n");
    for (int i = begin; i < end; i++) {
      fn(i, 0);
    }
  }
}

#endif
