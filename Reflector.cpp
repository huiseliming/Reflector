// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "FindCXXDeclAction.h"
#include "CodeGenerator.h"

using namespace clang::tooling;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("ReflectGenerator");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
  llvm::outs() << "Hello" << "\n";
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory, cl::NumOccurrencesFlag::OneOrMore);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser &OptionsParser = ExpectedParser.get();
  ArgumentsAdjuster Adjuster = OptionsParser.getArgumentsAdjuster();
  auto Files = OptionsParser.getSourcePathList();
  for (size_t i = 0; i < Files.size(); i++) {
    //llvm::outs() << "FILE : " << Files[i] << "\n";
    std::vector<CompileCommand> CompileCommands =
        OptionsParser.getCompilations().getCompileCommands(Files[i]);
    for (size_t j = 0; j < CompileCommands.size(); j++) {
      //llvm::outs() << "  CommandLine : ";
      for (size_t k = 0; k < CompileCommands[j].CommandLine.size(); k++) {
        llvm::outs() << CompileCommands[j].CommandLine[k] << " ";
      }
      llvm::outs() << "\n";
      //llvm::outs() << "  Directory   : " << CompileCommands[j].Directory << "\n";
      //llvm::outs() << "  Filename    : " << CompileCommands[j].Filename << "\n";
      //llvm::outs() << "  Heuristic   : " << CompileCommands[j].Heuristic << "\n";
    }
  }
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  int Result = Tool.run(newFrontendActionFactory<FindCXXDeclAction>().get());
  if(Result == 0){
    CCodeGenerator::Get().Generate();
  }
    return Result;
}