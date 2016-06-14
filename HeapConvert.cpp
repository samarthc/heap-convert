#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Driver/Options.h"
#include "clang/AST/ASTContext.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include <unordered_set>

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

unordered_set<Stmt*> visitedStmts;
unordered_set<Decl*> visitedDecls;

class HeapConvertVisitor : public RecursiveASTVisitor<HeapConvertVisitor>
{
    private:
        ASTContext *context;
        string head;
    public:
        explicit HeapConvertVisitor(CompilerInstance *CI) : context(&(CI->getASTContext()))
        {
            head = "";
        }
        virtual bool VisitVarDecl(VarDecl *D)
        {
            if(visitedDecls.count(D))
                return true;
            if(D->hasInit())
            {
                outs() << head << D->getNameAsString() << " = ";
                string oldhead = head;
                head = "";
                TraverseStmt(D->getInit());
                head = oldhead;
            }
            visitedDecls.insert(D);
            return true;
        }

        virtual bool VisitReturnStmt(ReturnStmt *S)
        {
            return true;
        }

        virtual bool VisitCompoundStmt(CompoundStmt *comp)
        {
            if(visitedStmts.count(comp))
            {
                for(Stmt::child_iterator it = comp->child_begin(); it != comp->child_end(); it++)
                    visitedStmts.insert(*it);
                return true;
            }
            for(CompoundStmt::body_iterator it = comp->body_begin(); it != comp->body_end(); it++)
            {
                if(!visitedStmts.count(*it))
                    TraverseStmt(*it);
            }
            visitedStmts.insert(comp);
            return true;
        } 
        virtual bool VisitExpr(Expr *S)
        {
            if(visitedStmts.count(S))
            {
                for(Stmt::child_iterator it = S->child_begin(); it != S->child_end(); it++)
                    visitedStmts.insert(*it);
                return true;
            }
            outs() << head;
            S->printPretty(outs(), nullptr, PrintingPolicy(context->getLangOpts()));
            outs() << '\n';
            visitedStmts.insert(S);
            for(Stmt::child_iterator it = S->child_begin(); it != S->child_end(); it++)
                visitedStmts.insert(*it);
            return true;
        }

        virtual bool VisitIfStmt(IfStmt *ifs)
        {
            if(visitedStmts.count(ifs))
            {
                for(Stmt::child_iterator it = ifs->child_begin(); it != ifs->child_end(); it++)
                    visitedStmts.insert(*it);
                return true;
            }

            string newhead;
            raw_string_ostream OS(newhead);
            Stmt *cond = ifs->getCond();
            cond->printPretty(OS, nullptr, PrintingPolicy(context->getLangOpts()));
            OS.str();

            string oldhead = head;
            head = oldhead + '(' + newhead + "): ";
            TraverseStmt(ifs->getThen());

            head = oldhead + "(!(" + newhead + ")): ";
            TraverseStmt(ifs->getElse());

            head = oldhead;
            visitedStmts.insert(cond);
            visitedStmts.insert(ifs);
            return true;
        }
        /*virtual bool VisitReturnStmt(ReturnStmt *ret)
        {
            ret->dump();
            return true;
        l}*/
};

class HeapConvertConsumer : public ASTConsumer
{
    private:
        HeapConvertVisitor *visitor;
    public:
        explicit HeapConvertConsumer(CompilerInstance *CI)
        {
            visitor = new HeapConvertVisitor(CI);
        }
        /*virtual void HandleTranslationUnit(ASTContext &context)
        {
            visitor->TraverseDecl(context.getTranslationUnitDecl());
        }*/
        virtual bool HandleTopLevelDecl(DeclGroupRef DG)
        {
            for(DeclGroupRef::iterator it = DG.begin(); it != DG.end(); it++)
            {
                if(NamedDecl *D = dyn_cast<NamedDecl>(*it))
                {
                    if(D->isFunctionOrFunctionTemplate() && D->getNameAsString() == "main")
                        visitor->TraverseDecl(D);
                }
            }
            return true;
        }
};

class HeapConvertAction : public ASTFrontendAction
{
    public:
        virtual unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)
        {
            return unique_ptr<ASTConsumer>(new HeapConvertConsumer(&CI));
        }
};

static cl::OptionCategory MyToolCategory("My tool options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

int main(int argc, const char **argv)
{
    CommonOptionsParser op(argc, argv, MyToolCategory);
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    return Tool.run(newFrontendActionFactory<HeapConvertAction>().get());
}
