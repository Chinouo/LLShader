#ifndef GAME_OBJECT
#define GAME_OBJECT

#include <cstdlib>

namespace LLShader {

class GameObject {
 public:
  GameObject() : id(reinterpret_cast<u_int64_t>(this)) {}

  inline u_int64_t getGID() { return id; }

 private:
  const u_int64_t id;
};

};  // namespace LLShader

#endif