# Simple C++ wrapper for reading MatrixMarket files.

# MatrixMarketFile

The only public class in the repository, this provides a read-only interface to
a matrix market file. It will offset the row and column ids by 1 so that they
are 0-based. 

# Usage

```
#include <mmio/MatrixMarketFile.hpp>

std::filesystem::path path = "matrix.mmio"
mmio::MatrixMarketFile mm(path);

// Header properties.
printf("rows %d, columns %d, edges %d\n",
       mm.getNRows(), mm.getNCols(), mm.getNEdges());

// A single edge `int` property.
for (auto&& [u, v, w] : edges<int>(mm)) {}

// Two doubles.
for (auto&& [u, v, s, t] : edges<double, double>(mm)) {}

// Ignore edge properties.
for (auto&& [u, v] : edges(mm)) {}
```
