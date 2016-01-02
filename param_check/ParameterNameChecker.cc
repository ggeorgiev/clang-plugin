#include "clang/AST/ASTConsumer.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

namespace
{
// RecursiveASTVisitor does a pre-order depth-first traversal of the AST. We override
// VisitFunctionDecl() methods for the types of nodes we are interested in.
class FuncDeclVisitor : public RecursiveASTVisitor<FuncDeclVisitor>
{
public:
    explicit FuncDeclVisitor(DiagnosticsEngine& d)
        : m_diag(d)
    {
    }

    // This function gets called for each FunctionDecl node in the AST.
    // Returning true indicates that the traversal should continue.
    bool VisitFunctionDecl(FunctionDecl* funcDecl)
    {
        // If has no prototype we cannot make the check.
        if (!funcDecl->hasPrototype())
            return true;

        auto prevDecl = funcDecl->getPreviousDecl();

        // If no previous function declaration we are OK.
        if (prevDecl == nullptr)
            return true;

        // If previous declaration has no prototype, we can't compare them.
        if (!prevDecl->hasPrototype())
            return true;

        for (unsigned i = 0; i != funcDecl->getNumParams(); ++i)
        {
            auto paramDecl = funcDecl->getParamDecl(i);

            // Anonymous parameters are OK.
            if (paramDecl->getName().empty())
                continue;

            auto previousParamDecl = prevDecl->getParamDecl(i);

            // Anonymous parameters are OK.
            if (previousParamDecl->getName().empty())
                continue;

            // Stl seems to be very bad of matching the names of the parameters. Luckily
            // they use underscores - something that you should not do. Ignore all the
            // parameters that start with underscore.
            if (paramDecl->getIdentifier()->getNameStart()[0] == '_' ||
                previousParamDecl->getIdentifier()->getNameStart()[0] == '_')
                continue;

            if (paramDecl->getIdentifier() == previousParamDecl->getIdentifier())
                continue;

            unsigned warn = m_diag.getCustomDiagID(DiagnosticsEngine::Warning,
                                                   "parameter name mismatch");
            m_diag.Report(paramDecl->getLocation(), warn);

            unsigned note = m_diag.getCustomDiagID(
                DiagnosticsEngine::Note,
                "parameter in previous function declaration was here");
            m_diag.Report(previousParamDecl->getLocation(), note);
        }

        return true;
    }

private:
    DiagnosticsEngine& m_diag;
};

// An ASTConsumer is a client object that receives callbacks as the AST is built, and
// "consumes" it.
class FuncDeclConsumer : public ASTConsumer
{
public:
    explicit FuncDeclConsumer(DiagnosticsEngine& d)
        : m_visitor(FuncDeclVisitor(d))
    {
    }

    // Called by the parser for each top-level declaration group. Returns true to continue
    // parsing, or false to abort parsing.
    virtual bool HandleTopLevelDecl(DeclGroupRef dg)
    {
        for (DeclGroupRef::iterator i = dg.begin(), e = dg.end(); i != e; ++i)
            m_visitor.TraverseDecl(*i);

        return true;
    }

private:
    FuncDeclVisitor m_visitor;
};

class ParameterNameChecker : public PluginASTAction
{
protected:
    // Create the ASTConsumer that will be used by this action. The StringRef parameter is
    // the current input filename (which we ignore).
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, llvm::StringRef)
    {
        return std::unique_ptr<ASTConsumer>(new FuncDeclConsumer(ci.getDiagnostics()));
    }

    // Parse command-line arguments. Return true if parsing succeeded, and the plugin
    // should proceed; return false otherwise.
    bool ParseArgs(const CompilerInstance&, const std::vector<std::string>&)
    {
        // We don't care about command-line arguments right now.
        return true;
    }
};

} // end namespace

// Register the PluginASTAction in the registry. This makes it available to be run with
// the '-plugin' command-line option.
static FrontendPluginRegistry::Add<ParameterNameChecker> X(
    "check-parameter-names", "check for parameter names mismatch");
