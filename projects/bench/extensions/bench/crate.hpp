#ifndef BENCH_CRATE_HPP
#define BENCH_CRATE_HPP

namespace bench
{
  /// A crate with no behaviour: a body alone, which nothing runs over. It is
  /// what the others are measured against.
  struct Crate
  {
    bool spawned = true;
  };
} // bench

#endif //BENCH_CRATE_HPP
