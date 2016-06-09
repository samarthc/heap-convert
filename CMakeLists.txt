add_clang_executable(heap-convert HeapConvert.cpp)

target_link_libraries(heap-convert clangTooling clangAST clangFrontend
    clangDriver)
