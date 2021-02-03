add_library(mmio_lib STATIC mmio.c MatrixMarketFile.cpp)
target_compile_features(mmio_lib PUBLIC cxx_std_20)
target_include_directories(mmio_lib PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
