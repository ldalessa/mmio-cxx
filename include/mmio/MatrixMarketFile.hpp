// BSD 3-Clause License
//
// Copyright (c) 2020, 2021 Trustees of Indiana University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#pragma once

#include <compare>
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

  /// Find the nth edge in the file.
  const char* edge(std::ptrdiff_t n) const;

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
};


/// Get a subset of edges from the .mmio file.
template <class... Vs>
inline static MatrixMarketFile::edge_range<Vs...>
edges(const MatrixMarketFile& mm, std::ptrdiff_t j, std::ptrdiff_t k)
{
  return {
    .begin_ = mm.edge(j),
    .end_   = mm.edge(k)
  };
}

/// Get the full range of edges in the .mmio file.
template <class... Vs>
inline static MatrixMarketFile::edge_range<Vs...>
edges(const MatrixMarketFile& mm)
{
  return {
    .begin_ = mm.edge(0),
    .end_   = mm.edge(mm.getNEdges())
  };
}
}
