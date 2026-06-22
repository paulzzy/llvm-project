// COM: Force Comgr's embedded libc++ fallback path.
// RUN: compile-hip-cxx-headers --mode=distroless %s | %FileCheck %s

// CHECK: Embedded libc++ headers: active
// CHECK: RESULT: PASS

#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>

static_assert(std::is_integral<int>::value, "int is integral");
static_assert(std::numeric_limits<std::int32_t>::digits == 31, "int32 digits");
static_assert(std::tuple_size<std::tuple<int, float>>::value == 2,
              "tuple size");

extern "C" __attribute__((global)) void test_kernel(int *out) {
  std::tuple<int, float> t{42, 3.14f};
  out[0] = std::get<0>(t);
}
