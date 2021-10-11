// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "CodeGenerator.h"
#include "ReflectMatcher.h"
#include <thread>

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
//"\n"
//"-p <build-path> is used to read a compile command database.\n"
//"\n"
//"\tFor example, it can be a CMake build directory in which a file named\n"
//"\tcompile_commands.json exists (use -DCMAKE_EXPORT_COMPILE_COMMANDS=ON\n"
//"\tCMake option to get this output). When no build path is specified,\n"
//"\ta search for compile_commands.json will be attempted through all\n"
//"\tparent paths of the first input file . See:\n"
//"\thttps://clang.llvm.org/docs/HowToSetupToolingForLLVM.html for an\n"
//"\texample of setting up Clang Tooling on a source tree.\n"
//"\n"
//"<source0> ... specify the paths of source files. These paths are\n"
//"\tlooked up in the compile command database. If the path of a file is\n"
//"\tabsolute, it needs to point into CMake's source tree. If the path is\n"
//"\trelative, the current working directory needs to be in the CMake\n"
//"\tsource tree and the file must be in a subdirectory of the current\n"
//"\tworking directory. \"./\" prefixes in the relative files will be\n"
//"\tautomatically removed, but the rest of a relative path must be a\n"
//"\tsuffix of a path in the compile command database.\n"
//"\n";

const char ReflectGeneratorHelpMessage[] = 
"\n"
"HuiSeLiMing:\n\n"
"  -p <BuildPath>\n"
"    set path to read compile_commands.json.\n"
"    设置构建路径读取 compile_commands.json.\n\n"
"  <Source0> [ ... <SourceN> ]\n"
"    generate reflect-source files from these source files\n"
"    反射源文件从这些源文件中生成\n\n"
;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("ReflectGenerator");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(ReflectGeneratorHelpMessage);



// A help message for this specific tool can be added afterwards.
//static cl::extrahelp MoreHelp("\n  \n");

int main(int argc, const char **argv) {
    std::chrono::steady_clock::time_point Start = std::chrono::steady_clock::now();
    llvm::outs() << "start parsing reflect object\n";

    auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory, cl::NumOccurrencesFlag::OneOrMore);
    if (!ExpectedParser) {
        // Fail gracefully for unsupported options.
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = ExpectedParser.get();
    //auto Files = OptionsParser.getSourcePathList();
    //for (size_t i = 0; i < Files.size(); i++) {
    //    //llvm::outs() << "FILE : " << Files[i] << "\n";
    //    std::vector<CompileCommand> CompileCommands =
    //        OptionsParser.getCompilations().getCompileCommands(Files[i]);
    //    for (size_t j = 0; j < CompileCommands.size(); j++) {
    //        //llvm::outs() << "  CommandLine : ";

    //        for (size_t k = 0; k < CompileCommands[j].CommandLine.size(); k++) {
    //        llvm::outs() << CompileCommands[j].CommandLine[k] << " ";
    //        }
    //        llvm::outs() << "\n";
    //        llvm::outs() << "  Directory   : " << CompileCommands[j].Directory << "\n";
    //        llvm::outs() << "  Filename    : " << CompileCommands[j].Filename << "\n";
    //        llvm::outs() << "  Heuristic   : " << CompileCommands[j].Heuristic << "\n";
    //    }
    //}
    std::vector<CompileCommand> CompileCommands = OptionsParser.getCompilations().getAllCompileCommands();
    std::string BuildPath;
    if(CompileCommands.size() > 0)
    {
        BuildPath = CompileCommands[0].Directory;
    }
    OptionsParser.getArgumentsAdjuster();
    const std::vector<std::string> SourcePathList = OptionsParser.getSourcePathList();
    std::vector<std::thread> Threads;
    bool MT = false;
    for (size_t i = 0; i < SourcePathList.size(); i++)
    {
        std::function<void()> Task = [&] {
            std::vector<std::string> Source;
            Source.push_back(SourcePathList[i]);
            ClangTool Tool(OptionsParser.getCompilations(), Source);
            CCodeGenerator CG;
            CG.BuildPath = BuildPath;
            clang::ast_matchers::MatchFinder Finder;
            ReflectEnumMatcher EnumMatcher;
            EnumMatcher.CodeGenerator = &CG;
            Finder.addMatcher(enumDecl(hasAttr(attr::Annotate)).bind("Enum"), &EnumMatcher);
            ReflectClassMatcher ClassMatcher;
            ClassMatcher.CodeGenerator = &CG;
            Finder.addMatcher(cxxRecordDecl(hasAttr(attr::Annotate)).bind("Class"), &ClassMatcher);

            Tool.appendArgumentsAdjuster([](const CommandLineArguments& CmdArg, StringRef Filename)
                -> CommandLineArguments
                {
                    auto NewCmdArg = CmdArg;
                    NewCmdArg.erase(std::remove(NewCmdArg.begin(), NewCmdArg.end(), "/MP"), NewCmdArg.end());
                    std::vector<std::string> AddArgs;
                    //AddArgs.push_back("-Wdeprecated-enum-enum-conversion");
                    //AddArgs.push_back("-Wpessimizing-move");
                    //AddArgs.push_back("-Wunused-const-variable");
                    bool NoWarningIsSet = false;;
                    std::for_each(NewCmdArg.begin(), NewCmdArg.end(), [&](std::string& Str) {
                        if (0 == strncmp(Str.data(), "/W", 2)) {
                            Str[2] = '0';
                            NoWarningIsSet = true;
                        }
                        for (auto Iterator = AddArgs.begin(); Iterator != AddArgs.end(); ) {
                            if (Str == *Iterator) {
                                Iterator = AddArgs.erase(Iterator);
                            }
                            else {
                                Iterator++;
                            }
                        }
                    });
                    std::for_each(AddArgs.begin(), AddArgs.end(), [&] (std::string& Str) { NewCmdArg.insert(++NewCmdArg.begin(),Str); });
                    NewCmdArg.insert(++NewCmdArg.begin(), "-D__REFLECTOR__");
                    if(!NoWarningIsSet){
                        NewCmdArg.insert(++NewCmdArg.begin(), "/W0");
                    }
                    for (size_t i = 0; i < NewCmdArg.size(); i++) {
                        llvm::outs() << NewCmdArg[i] << " ";
                    }
                    llvm::outs() << "\n";
                    return NewCmdArg;
                });
            int Result = Tool.run(newFrontendActionFactory(&Finder).get());
            if(Result != 0){
                llvm::errs() << std::format("Parsing reflect object failed\n");
                std::chrono::steady_clock::time_point End = std::chrono::steady_clock::now();
                llvm::errs() << std::format("Parsing reflect object in {:f} seconds\n", std::chrono::duration<double>(End - Start).count());
                return 1;
            }
            CG.Generate();
        };
        if(MT){
            Threads.emplace_back(std::thread(std::move(Task)));
        }
        else
        {
            Task();
        }
    }
    if (MT) {
        for (size_t i = 0; i < Threads.size(); i++)
        {
            Threads[i].join();
        }
    }
    std::chrono::steady_clock::time_point End = std::chrono::steady_clock::now();
    llvm::outs() << std::format("Parsing reflect object in {:f} seconds\n", std::chrono::duration<double>(End - Start).count());
    return 0;
}