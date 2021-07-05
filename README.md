# MetadataGenerator
ClangTooling For DynamicReflection

compile llvm-project with clang-tools-extra use
# Ninja
cmake -G Ninja ../llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_BUILD_TESTS=ON
# MSBuild
cmake -G "Visual Studio 16 2019" ../llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_BUILD_TESTS=ON

clone the repository to clang-tools-extra folder

rebuild llvm

print AST use

clang -cc1 -ast-dump AST.cpp
