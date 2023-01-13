#pragma once

#include "heap.h"
#include <vector>

namespace gc {

class DerivedHeap : public TigerHeap {
  // TODO(lab7): You need to implement those interfaces inherited from TigerHeap
  // correctly according to your design.
public:
  DerivedHeap() {}
  ~DerivedHeap() {}
  char *Allocate(uint64_t size) override;
  uint64_t Used() const override;
  uint64_t MaxFree() const override;
  void Initialize(uint64_t size) override;
  void GC() override;

  struct pointerMap {
    uint64_t index;
    uint64_t frameSize;
    uint64_t isMain;
    std::vector<int64_t> frameOffset;
    std::vector<uint32_t> regBitmap;
    pointerMap(uint64_t index, uint64_t frameSize,uint64_t isMain,
               std::vector<int64_t> frameOffset,
               std::vector<uint32_t> regBitmap)
        : index(index), frameSize(frameSize), isMain(isMain),frameOffset(frameOffset),
          regBitmap(regBitmap) {}
  };

private:
  void *heap_begin;
  void *from, *to;
  void *free_head;
  void *scan, *next;
  uint64_t heap_size;
  std::vector<pointerMap> maps;
  std::vector<std::pair<uint64_t,uint64_t *>> roots;

  void getAllPtrMaps();
  void getRoots(uint64_t *sp);
  uint64_t forward(uint64_t p);
  bool point2From(uint64_t p);
  bool point2To(uint64_t p);
  bool point2Heap(uint64_t p);
  uint64_t getSizeofRecord(uint64_t *pointer);
};

} // namespace gc
