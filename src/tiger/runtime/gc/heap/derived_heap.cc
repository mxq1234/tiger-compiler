#include "derived_heap.h"
#include <stdio.h>
#include <stack>
#include <cstring>

namespace gc {
// TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.
struct string {
  int length;
  unsigned char chars[1];
};

void DerivedHeap::Initialize(uint64_t size) {
  size += 64;      // for copy GC and bigger tree
  heap_ = new char[size];
  heap_size_ = size / 2;
  from_space_ = heap_;
  to_space_ = heap_ + heap_size_;
  next_ = from_space_;
}

char* DerivedHeap::Allocate(uint64_t size) {
  uint64_t allocated_size_ = next_ - from_space_;
  if(allocated_size_ + size > heap_size_)
      return nullptr;
  char* next = next_;
  next_ += size;
  return next;
}

inline uint64_t DerivedHeap::Used() const {
  return next_ - from_space_;
}

inline uint64_t DerivedHeap::MaxFree() const {
  return heap_size_ - (next_ - from_space_);
}

void DerivedHeap::GC() {
  GET_TIGER_STACK(sp);
  uint64_t* scan = (uint64_t*)to_space_;
  next_ = to_space_;
  std::vector<Root> roots = GetRoots(sp, &GLOBAL_GC_ROOTS);
  for(Root root : roots)
    root.Set(Forward(root.Get()));
  while((char*)scan < next_) {
    if(scan[0] & 1) {
      string* descriptor = (string*)(scan[0] & -2);
      uint64_t size = descriptor->length + 1;
      for(uint64_t i = 1; i < size; ++i) {
        if(descriptor->chars[i - 1] != 'p')  continue;
        scan[i] = (long long)Forward((char*)scan[i]);
      }
      scan += (descriptor->length + 1);
      continue;
    }
    scan += (scan[0] / 2 + 1);
  }
  char* tmp = to_space_;
  to_space_ = from_space_;
  from_space_ = tmp;
}

bool DerivedHeap::FromSpace(char* p) const {
  int64_t offset = p - from_space_;
  return offset >= 0 && offset < (int64_t)heap_size_;
}

bool DerivedHeap::ToSpace(char* p) const {
  int64_t offset = p - to_space_;
  return offset >= 0 && offset < (int64_t)heap_size_;
}

char* DerivedHeap::Forward(char* p) {
  if(!FromSpace(p))   return p;
  long long* q = (long long*)p;
  if(ToSpace((char*)*q))  return (char*)*q;

  long long* a = (long long*)p - 1;
  long long* next = (long long*)next_;
  uint64_t size = ((a[0] & 1)? ((string*)(a[0] & -2))->length : a[0] / 2) + 1;
  for(uint64_t i = 0; i < size; ++i)
    next[i] = a[i];
  *q = (long long)(next + 1);
  next_ = next_ + size * sizeof(long long);

  return (char*)*q;
}

std::vector<Root> DerivedHeap::GetRoots(uint64_t* sp, uint64_t* head) {
  std::vector<Root> roots;
  uint64_t* callee_reg_[7];
  bool flag = true;
  while(true) {
    uint64_t ret_addr = *sp;
    uint64_t* p = head;
    while((int)*p != -1 && *p != ret_addr)   p += p[1];
    if((int)*p == -1)  break;
    RuntimePointerMap* pointer_map = RuntimePointerMap::GetMap(p);
    uint64_t* bp = sp + pointer_map->frame_size_ / 8 + 1;
    if(flag) {
      flag = false;
      for(int i = 0; i < 7; ++i)
        callee_reg_[i] = bp + pointer_map->callee_save_before_alloc_[i] / 8;
    }
    for(int r : pointer_map->roots_inreg_)
      roots.push_back(Root(callee_reg_[r]));
    for(int r : pointer_map->roots_onstack_)
      roots.push_back(Root(bp + r / 8));
    for(auto x : pointer_map->callee_save_onstack_)
      callee_reg_[x.first] = bp + x.second / 8;
    sp = bp;
    delete pointer_map;
  }
  return roots;
}

RuntimePointerMap* RuntimePointerMap::GetMap(uint64_t* p) {
  RuntimePointerMap* pointer_map = new RuntimePointerMap;
  uint64_t i = 2;
  pointer_map->frame_size_ = p[i++];
  uint64_t reg_map = p[i++];
  char* ptr = (char*)&reg_map;

  for(int j = 0; j < 7; ++j)
    if(ptr[j])  pointer_map->roots_inreg_.push_back(j);
  
  uint64_t root_num = p[i++];
  for(uint64_t tmp = i; i < tmp + root_num; ++i)
    pointer_map->roots_onstack_.push_back(p[i]);
  uint64_t spill_num = p[i++];
  for(uint64_t tmp = i; i < tmp + spill_num * 2; i += 2)
    pointer_map->callee_save_onstack_[p[i]] = p[i + 1];
  if(p[i++]) {
    uint64_t offset = p[i];
    for(int i = 0; i < 7; ++i) {
      pointer_map->callee_save_before_alloc_.push_back(offset);
      offset -= 8;
    }
  }
  return pointer_map;
}

} // namespace gc