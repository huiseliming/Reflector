set(LLVM_LINK_COMPONENTS support)

add_clang_executable(Reflector
  Reflector.cpp
  CodeGenerator.cpp
  ParsingClass.cpp
  Tool.cpp
  Reflect.cpp
  )

target_link_libraries(Reflector
  PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )
 
target_include_directories(Reflector PUBLIC
    ReflectorModule/Include
)

target_compile_definitions(Reflector PRIVATE REFLECT_CODE_GENERATOR)
target_compile_definitions(Reflector PRIVATE _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

#_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
