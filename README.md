# MetadataGenerator
ClangTooling For DynamicReflection

compile llvm-project with clang-tools-extra use

cmake -G Ninja ../llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_BUILD_TESTS=ON

clone the repository to clang-tools-extra folder

rebuild llvm
