#include <mmio/MatrixMarketFile.hpp>
#include <cassert>
#include <cstdio>

int main(int argc, char* const argv[])
{
  if (argc != 2) {
    fprintf(stderr, "usage: mmio <path>\n");
    return EXIT_FAILURE;
  }
  std::filesystem::path path = argv[1];
  mmio::MatrixMarketFile mm(path);
  printf("rows %d, cols %d, non-zeros %d\n", mm.getNRows(), mm.getNCols(), mm.getNEdges());

  for (auto&& [u, v, w] : edges<int>(mm)) {
    printf("%d %d %d\n", u, v, w);
  }

  return 0;
}
