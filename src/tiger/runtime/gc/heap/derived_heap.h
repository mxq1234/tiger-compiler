#pragma once

#include "heap.h"
#include <vector>
#include <map>

namespace gc {

class Root {
private:
  uint64_t* stack_pointer_;

public:
  Root(uint64_t* p) : stack_pointer_(p) {}
  char* Get() const { return (char*)*stack_pointer_; }
  void Set(char* p) { *stack_pointer_ = (uint64_t)p; }
};

class RuntimePointerMap {
public:
  uint64_t frame_size_;
  std::vector<uint64_t> roots_inreg_;
  std::vector<uint64_t> roots_onstack_;
  std::map<uint64_t, uint64_t> callee_save_onstack_;
  std::vector<uint64_t> callee_save_before_alloc_;

  RuntimePointerMap() = default;
  static RuntimePointerMap* GetMap(uint64_t* p);
};

class DerivedHeap : public TigerHeap {
  // TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.
public:
  char* Allocate(uint64_t size) override;
  uint64_t Used() const override;
  uint64_t MaxFree() const override;
  void Initialize(uint64_t size) override;
  void GC() override;

private:
  bool FromSpace(char* p) const;
  bool ToSpace(char* p) const;
  char* Forward(char* p);
  std::vector<Root> GetRoots(uint64_t* sp, uint64_t* map);

private:
  char* heap_;
  char* from_space_;
  char* to_space_;
  char* next_;
  uint64_t heap_size_;
  uint64_t* sp;
};

} // namespace gc