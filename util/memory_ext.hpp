#ifndef MEMORY_EXT_HPP
#define MEMORY_EXT_HPP

#include <cstdlib>

#include "engine/matrix.hpp"
#include "render/vk_context.hpp"

namespace LLShader {

inline void* aligned_malloc(size_t size, size_t alignment) {
  void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
  data = _aligned_malloc(size, alignment);
#else
  int res = posix_memalign(&data, alignment, size);
  if (res != 0) data = nullptr;
#endif
  return data;
}

inline void aligned_mfree(void* data) {
#if defined(_MSC_VER) || defined(__MINGW32__)
  _aligned_free(data);
#else
  free(data);
#endif
}

/// make sure vk_holder is init!
/// @deprecated
inline size_t getDynamicAlignedSize(size_t size) {
  size_t min_uniform_aligment =
      global_matrix_engine.vk_holder->getPhysicalDeviceProperties()
          .limits.minUniformBufferOffsetAlignment;

  size_t dynamic_aligment = size;
  if (min_uniform_aligment > 0) {
    // 这个算法是求 我们需要的大小 和 min_uniform_aligment 的 整数倍数
    // 最接近的那个值
    dynamic_aligment = (dynamic_aligment + min_uniform_aligment - 1) &
                       ~(min_uniform_aligment - 1);
  }

  return dynamic_aligment;
}

inline u_int32_t getDynamicAlignedSize2(u_int32_t size) {
  u_int32_t t = 1;
  u_int32_t cnt = 0;

  while (size) {
    if (size & 1) {
      cnt++;
    }
    size >>= 1;
    t <<= 1;
  }

  // 移动回来保证 得到的是最高位的值
  t >>= 1;
  // size 非 2 的幂, 用高一位的
  if (cnt > 1) t <<= 1;

  return t;
}

/// Array used for dynamic uniform data, only safe for explict data.
/// Allocate mem at runtime, cal size at compile time.
/// If your data contain some implict data such like pointer and
/// call destruct function, it cause memory leak!!!
template <typename _Tp, size_t _Size>
class AlignedArray final {
 public:
  typedef _Tp value_type;
  typedef value_type* value_type_ptr;
  typedef value_type& ref;
  typedef const value_type& const_ref;
  typedef size_t size_type;

  static constexpr size_t dynamic_algined_memory_size() {
    size_t size = sizeof(value_type);
    u_int32_t t = 1;
    u_int32_t cnt = 0;

    while (size) {
      if (size & 1) {
        cnt++;
      }
      size >>= 1;
      t <<= 1;
    }

    // 移动回来保证 得到的是最高位的值
    t >>= 1;
    // size 非 2 的幂, 用高一位的
    if (cnt > 1) t <<= 1;

    return t;
  }

  AlignedArray() : stride_(dynamic_algined_memory_size()), size_(_Size) {
    // the stride must be power of 2
    // TODO: for uniform buf, aligment may bigger or equal to
    // min_uniform_aligment, make a check.
    p_data_origin = aligned_malloc(size_ * stride_, stride_);
  }

  AlignedArray(const AlignedArray&) = delete;

  ~AlignedArray() { aligned_mfree(p_data_origin); }

  size_type size() const { return size_; }

  const void* data() const { return p_data_origin; }

  const_ref& at(size_type index) const {
    assert(index < size_);
    return static_cast<const_ref&>(calculate_target(index));
  }

  ref& at(size_type index) {
    assert(index < size_);
    return calculate_target(index);
  }

  const_ref operator[](size_type index) const {
    assert(index < size_);
    return static_cast<const_ref&>(calculate_target(index));
  }

  ref operator[](size_type index) {
    assert(index < size_);
    return calculate_target(index);
  }

 private:
  inline ref& calculate_target(size_type index) {
    // conver ptr to 64 bit address
    // I like reinterpret_cast, cool stuff.
    u_int64_t target_address = reinterpret_cast<u_int64_t>(p_data_origin) +
                               static_cast<u_int64_t>(stride_ * index);

    return *reinterpret_cast<value_type_ptr>(target_address);
    // return ;
  }
  // ptr to zero offset
  void* p_data_origin;
  // item count
  size_type size_;
  // single type size
  const u_int32_t stride_;
};

};  // namespace LLShader

#endif
