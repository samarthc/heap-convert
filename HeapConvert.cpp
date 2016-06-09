#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Driver/Options.h"
#include "clang/AST/ASTContext.h"
#include "clang/Tooling/CommonOptionsParser.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

class EchoReturnsVisitor : public RecursiveASTVisitor<EchoReturnsVisitor>
{
    private:
        ASTContext *context;
    public:
        explicit EchoReturnsVisitor(CompilerInstance *CI) : context(&(CI->getASTContext()))
        {}
        virtual bool VistDeclStmt(DeclStmt *decl)
        {
            decl->dump();
            return true;
        }
        virtual bool VisitReturnStmt(ReturnStmt *ret)
        {
            ret->dump();
            return true;
        }
};

class EchoReturnsConsumer : public ASTConsumer
{
    private:
        EchoReturnsVisitor *visitor;
    public:
        explicit EchoReturnsConsumer(CompilerInstance *CI)
        {
            visitor = new EchoReturnsVisitor(CI);
        }
        virtual void HandleTranslationUnit(ASTContext &context)
        {
            visitor->TraverseDecl(context.getTranslationUnitDecl());
        }
};

class EchoReturnsAction : public ASTFrontendAction
{
    public:
        virtual unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)
        {
            return unique_ptr<ASTConsumer>(new EchoReturnsConsumer(&CI));
        }
};

static cl::OptionCategory MyToolCategory("My tool options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

int main(int argc, const char **argv)
{
    CommonOptionsParser op(argc, argv, MyToolCategory);
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    return Tool.run(newFrontendActionFactory<EchoReturnsAction>().get());
}
