#pragma once

#include "heap.h"
#include <vector>

namespace gc {

class DerivedHeap : public TigerHeap {
  // TODO(lab7): You need to implement those interfaces inherited from TigerHeap
  // correctly according to your design.
  char *Allocate(uint64_t size) override {
    // int size = 0;
    // // record
    // if (str->front() == '1') {
    //   size = str->size();
    // }
    // // array
    // else {
    //   std::string temp(str->begin() + 1, str->end());
    //   size = atoi(temp.data());
    // }
    // int *p, *a;
    // p = a = (int *)malloc(size);
    // *p++ = (int)str;
    // for (int i = 1; i < size; i += sizeof(int))
    //   *p++ = 0;
    // a++;
    // return (char *)a;
  }
  uint64_t Used() const override {}
  uint64_t MaxFree() const override {}
  void Initialize(uint64_t size) override {}
  void GC() override {}
};

} // namespace gc
