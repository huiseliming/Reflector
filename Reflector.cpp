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

bool ParseFailed = false;

int main(int argc, const char **argv) {
    std::chrono::steady_clock::time_point Start = std::chrono::steady_clock::now();
    
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
    if(CompileCommands.size() > 0)
    {
        CCodeGenerator::Get().BuildPath = CompileCommands[0].Directory;
    }
    OptionsParser.getArgumentsAdjuster();
    ClangTool Tool(OptionsParser.getCompilations(),
                    OptionsParser.getSourcePathList());
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
    int Result = Tool.run(newFrontendActionFactory<FindCXXDeclAction>().get());
    if(Result == 0 && !ParseFailed){
        CCodeGenerator::Get().Generate();
        std::chrono::steady_clock::time_point End = std::chrono::steady_clock::now();
        llvm::outs() << std::format("Parsing reflect object in {:f} seconds\n", std::chrono::duration<double>(End - Start).count());
        return 0;
    }
    llvm::errs() << std::format("Parsing reflect object failed\n");
    std::chrono::steady_clock::time_point End = std::chrono::steady_clock::now();
    llvm::errs() << std::format("Parsing reflect object in {:f} seconds\n", std::chrono::duration<double>(End - Start).count());
    return 1;
}