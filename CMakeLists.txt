set(LLVM_LINK_COMPONENTS support)

add_clang_executable(MetadataGenerator
  MetadataGenerator.cpp
  )
target_link_libraries(MetadataGenerator
  PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )