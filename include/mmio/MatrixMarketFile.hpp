#pragma once

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <tuple>

namespace mmio
{

/// A class to represent an .mmio file.
///
/// This provides a simple interface to the cardinality of the matrix (number of
/// rows, columns, and number of edges), as well as the ability to iterate over
/// the edges in the file.
///
/// Iteration requires that you identify the types of any edge attributes that
/// you would like to extract from the file. For instance:
///
/// ```
/// MatrixMarketFile mm(path);
/// for (auto&& [u, v, d] : edges<double>(mm)) {}      // <-- one double
/// for (auto&& [u, v, i, j] : edges<int, int>(mm)) {} // <-- two ints
/// for (auto&& [u, v] : edges(mm)) {}                 // <-- ignore attributes
/// ```
class MatrixMarketFile
{
  std::int32_t   n_ = 0;                        // number of rows
  std::int32_t   m_ = 0;                        // number of columns
  std::int32_t nnz_ = 0;                        // number of edges

  const char* base_ = nullptr;                  // base pointer to mmap-ed file
  std::ptrdiff_t i_ = 0;                        // byte offset of the first edge
  std::ptrdiff_t e_ = 0;                        // bytes in the mmap-ed file

  /// Find the nth edge in the file.
  const char* edge(std::ptrdiff_t n) const;

 public:
  MatrixMarketFile(std::filesystem::path);
  ~MatrixMarketFile();

  /// Release the memory mapping and file descriptor early.
  void release();

  std::int32_t getNRows() const {
    return n_;
  }

  std::int32_t getNCols() const {
    return m_;
  }

  std::int32_t getNEdges() const {
    return nnz_;
  }

  /// Iterator over edges in the file.
  ///
  /// The iterator class template is parameterized by the types of the
  /// attributes that are being extracted.
  template <class... Vs>
  class edge_iterator
  {
    const char* i_ = nullptr;

    /// Read the next token in the stream as a U, and update the pointer.
    template <class U>
    static U get(const char* (&i))
    {
      // This horrible code is used because normal stream processing is very
      // slow, while this direct version is just sort of slow.
      U u;
      char* e;
      if constexpr (std::is_same_v<std::int32_t, U>) {
        u = std::strtol(i, &e, 10);
      }
      if constexpr (std::is_same_v<std::uint32_t, U>) {
        u = std::strtoul(i, &e, 10);
      }
      else if constexpr (std::is_same_v<std::int64_t, U>) {
        u = std::strtol(i, &e, 10);
      }
      else if constexpr (std::is_same_v<std::uint64_t, U>) {
        u = std::strtoul(i, &e, 10);
      }
      else if constexpr (std::is_same_v<float, U>) {
        u = std::strtof(i, &e);
      }
      else {
        u = std::strtod(i, &e);
      }
      i = e;
      return u;
    }

   public:
    using value_type = std::tuple<std::int32_t, std::int32_t, Vs...>;

    edge_iterator() = default;

    edge_iterator(const char* i)
        : i_(i)
    {
    }

    value_type operator*() const
    {
      const char* i = i_;
      std::int32_t u = get<std::int32_t>(i) - 1;
      std::int32_t v = get<std::int32_t>(i) - 1;
      return value_type(u, v, get<Vs>(i)...);
    }

    edge_iterator& operator++()
    {
      ++(i_ = std::strchr(i_, '\n'));
      return *this;
    }

    edge_iterator operator++(int)
    {
      edge_iterator b = *this;
      ++(*this);
      return b;
    }

    bool operator==(const edge_iterator&) const = default;
    auto operator<=>(const edge_iterator&) const = default;
  };

  template <class... Vs>
  struct edge_range
  {
    edge_iterator<Vs...> begin_;
    edge_iterator<Vs...>   end_;

    edge_iterator<Vs...> begin() const { return begin_; }
    edge_iterator<Vs...>   end() const { return end_; }
  };

  /// Get a subset of edges from the .mmio file.
  template <class... Vs>
  friend edge_range<Vs...>
  edges(const MatrixMarketFile& mm, std::ptrdiff_t j, std::ptrdiff_t k)
  {
    return {
      .begin_ = mm.edge(j),
      .end_   = mm.edge(k)
    };
  }

  /// Get the full range of edges in the .mmio file.
  template <class... Vs>
  friend edge_range<Vs...> edges(const MatrixMarketFile& mm)
  {
    return {
      .begin_ = mm.edge(0),
      .end_   = mm.edge(mm.nnz_)
    };
  }
};
}
