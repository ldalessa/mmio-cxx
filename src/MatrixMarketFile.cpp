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
#include "mmio/MatrixMarketFile.hpp"

#include <cassert>
#include <cstdio>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

extern "C" {
#include "mmio.h"
}

mmio::MatrixMarketFile::MatrixMarketFile(std::filesystem::path path)
{
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "open failed, %d: %s\n", errno, strerror(errno));
    std::exit(EXIT_FAILURE);
  }

  FILE* f = fdopen(fd, "r");
  if (f == nullptr) {
    fprintf(stderr, "fdopen failed, %d: %s\n", errno, strerror(errno));
    close(fd);
    std::exit(EXIT_FAILURE);
  }

  MM_typecode type;
  switch (mm_read_banner(f, &type)) {
   case MM_PREMATURE_EOF:    // if all items are not present on first line of file.
   case MM_NO_HEADER:        // if the file does not begin with "%%MatrixMarket".
   case MM_UNSUPPORTED_TYPE: // if not recongizable description.
    fclose(f);
    std::exit(EXIT_FAILURE);
  }

  if (!mm_is_coordinate(type)) {
    fclose(f);
    std::exit(EXIT_FAILURE);
  }

  switch (mm_read_mtx_crd_size(f, &n_, &m_, &nnz_)) {
   case MM_PREMATURE_EOF:    // if an end-of-file is encountered before processing these three values.
    fclose(f);
    std::exit(EXIT_FAILURE);
  }

  i_ = ftell(f);
  fseek(f, 0L, SEEK_END);
  e_ = ftell(f);

  base_ = static_cast<char*>(mmap(nullptr, e_, PROT_READ, MAP_PRIVATE, fd, 0));
  if (base_ == MAP_FAILED) {
    fprintf(stderr, "mmap failed, %d: %s\n", errno, strerror(errno));
    fclose(f);
    std::exit(EXIT_FAILURE);
  }

  fclose(f);
}

mmio::MatrixMarketFile::~MatrixMarketFile() {
  release();
}

void
mmio::MatrixMarketFile::release()
{
  if (base_ && munmap((char*)base_, e_)) {
    fprintf(stderr, "munmap failed, %d: %s\n", errno, strerror(errno));
  }
  base_ = nullptr;
}

const char*
mmio::MatrixMarketFile::edge(std::ptrdiff_t n) const
{
  assert(0 <= n && n <= nnz_);

  if (n == 0) {
    return base_ + i_;
  }

  if (n == nnz_) {
    return base_ + e_;
  }

  // Compute an approximate byte offset for this edge.
  std::ptrdiff_t  bytes = e_ - i_;
  std::ptrdiff_t approx = i_ + (n * bytes) / nnz_;

  // Search backward to find the beginning of the edge that we landed in.
  while (i_ <= approx && base_[approx] != '\n') {
    --approx;
  }
  ++approx;
  return base_ + approx;
}
