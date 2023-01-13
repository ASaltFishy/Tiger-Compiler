#include "derived_heap.h"
#include <algorithm>
#include <cstring>
#include <stack>
#include <stdio.h>

namespace gc {
// TODO(lab7): You need to implement those interfaces inherited from TigerHeap
// correctly according to your design.
char *DerivedHeap::Allocate(uint64_t size) {
  if (MaxFree() >= size) {
    char *old_free_head = (char *)free_head;
    free_head = (char *)free_head + size;
    return old_free_head;
  } else {
    return nullptr;
  }
}

uint64_t DerivedHeap::Used() const { return (char *)free_head - (char *)from; }

uint64_t DerivedHeap::MaxFree() const {
  char *from_end = (char *)from + heap_size;
  return (char *)from_end - (char *)free_head;
}

void DerivedHeap::Initialize(uint64_t size) {
  heap_begin = malloc(size);
  memset(heap_begin, 0, size);
  heap_size = size / 2;
  free_head = from = heap_begin;
  to = (char *)from + heap_size;
  getAllPtrMaps();
}

void DerivedHeap::GC() {
  uint64_t *sp;
  GET_TIGER_STACK(sp);
  getRoots(sp);
  scan = next = to;
  // 要先判断这些东东里面是不是都是指向堆的指针
  for (std::pair<uint64_t, uint64_t *> root : roots) {
    if (point2Heap(root.first)) {
      *root.second = forward(root.first);
    }
  }
  // Cheney算法
  while (scan < next) {
    uint64_t *temp = (uint64_t *)scan;
    bool isArray = false;
    if (*(temp) == 0) {
      // array 不含指针直接跳过
      scan = (uint64_t *)scan + *(temp + 1) + 2;
    } else {
      // record要利用descriptor中的metadata
      uint64_t size = getSizeofRecord(temp);
      std::string *str = (std::string *)(*temp);
      for (int i = 0; i < size-2; i++) {
        if(str->at(i)=='1'){
          *(temp+i+2) = forward(*(temp+i+2));
        }
      }
      scan = (uint64_t *)scan + size;
    }
  }
  std::swap(from,to);
  free_head = next;
}

uint64_t DerivedHeap::forward(uint64_t p) {
  // 将指针调整为真正的record起点
  uint64_t *pointer = (uint64_t *)p - 2;
  if (point2From((uint64_t)pointer)) {
    if (point2To(*pointer))
      return *pointer+2*WORD_SIZE;
    else {
      uint64_t size = getSizeofRecord(pointer);
      memcpy(next, pointer, size * WORD_SIZE);
      (*pointer) = (uint64_t)next;
      next = (uint64_t *)next + size;
      return *(pointer)+2*WORD_SIZE;
    }
  } else {
    return p;
  }
}
uint64_t DerivedHeap::getSizeofRecord(uint64_t *pointer) {
  bool isArray = false;
  if (*(pointer) == 0)
    isArray = true;
  uint64_t size = 0;
  if (isArray) {
    size = *(pointer + 1) + 2;
  } else {
    std::string *str = (std::string *)(*pointer);
    size = str->size() + 2;
  }
  return size;
}

bool DerivedHeap::point2From(uint64_t p) {
  char *from_end = (char *)from + heap_size;
  return p >= (uint64_t)from && p < (uint64_t)from_end;
}

bool DerivedHeap::point2To(uint64_t p) {
  char *to_end = (char *)to + heap_size;
  return p >= (uint64_t)to && p < (uint64_t)to_end;
}

bool DerivedHeap::point2Heap(uint64_t p) {
  return point2To(p) || point2From(p);
}

void DerivedHeap::getRoots(uint64_t *sp) {
  roots.clear();
  uint64_t returnAddress = *sp;
  bool terminate = false;
  while (!terminate) {
    for (pointerMap map : maps) {
      if (returnAddress == map.index) {
        if (map.isMain == 1) {
          terminate = true;
        }
        for (int64_t offset : map.frameOffset) {
          uint64_t *pointerAddress =
              (uint64_t *)(offset + (int64_t)sp + (int64_t)map.frameSize +
                           WORD_SIZE);
          roots.push_back(std::make_pair(*pointerAddress, pointerAddress));
        }
        for (int i = 0; i < 6; i++) {
          if (map.regBitmap[i] == 1) {
            unsigned long reg;
            switch (i) {
            case 0:
              GET_RBX(reg);
              break;
            case 1:
              GET_RBP(reg);
              break;
            case 2:
              GET_R12(reg);
              break;
            case 3:
              GET_R13(reg);
              break;
            case 4:
              GET_R14(reg);
              break;
            case 5:
              GET_R15(reg);
              break;
            default:
              break;
            }
            uint64_t *pointerAddress = (uint64_t *)(*(uint64_t *)reg);
            roots.push_back(std::make_pair(*pointerAddress, pointerAddress));
          }
        }
        sp += (map.frameSize / WORD_SIZE + 1);
        break;
      }
    }
  }
  return;
}

void DerivedHeap::getAllPtrMaps() {
  uint64_t *ptrmapBegin = &GLOBAL_GC_ROOTS;
  uint64_t *iter = ptrmapBegin;
  while ((int64_t)*iter != -2) {
    uint64_t retIndex = *iter++;
    uint64_t frameSize = *iter++;
    uint64_t isMain = *iter++;
    std::vector<int64_t> frameOffset;
    std::vector<uint32_t> regBitmap;
    uint64_t temp = *iter++;
    std::string bitmap = std::to_string(temp);
    reverse(bitmap.begin(), bitmap.end());
    int size = bitmap.size();
    for (int i = 0; i < 6; i++) {
      if (i >= size) {
        regBitmap.push_back(0);
      } else {
        regBitmap.push_back(bitmap[i] - 48);
      }
    }
    reverse(regBitmap.begin(), regBitmap.end());
    while (true) {
      int64_t offset = *iter++;
      if (offset == -1)
        break;
      frameOffset.push_back(offset);
    }
    maps.push_back(
        pointerMap(retIndex, frameSize, isMain, frameOffset, regBitmap));
  }
}

} // namespace gc
