#ifndef MEMORY_EXT_HPP
#define MEMORY_EXT_HPP

#include <cstdlib>

namespace LLShader {

void* aligned_malloc(size_t size, size_t alignment) {
  void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
  data = _aligned_malloc(size, alignment);
#else
  int res = posix_memalign(&data, alignment, size);
  if (res != 0) data = nullptr;
#endif
  return data;
}

void aligned_mfree(void* data) {
#if defined(_MSC_VER) || defined(__MINGW32__)
  _aligned_free(data);
#else
  free(data);
#endif
}

};  // namespace LLShader

#endif