#ifndef DCE_H
#define DCE_H
#include "../../include/ir.h"
#include "../pass.h"

#include "../analysis/dominator_tree.h"
class DCEPass : public IRPass {
private:
    PostDomAnalysis *postdomtrees;
    void ADCE(CFG *C);
    void DCE(CFG *C);

public:
    DCEPass(LLVMIR *IR,PostDomAnalysis *postdom) : IRPass(IR) { postdomtrees=postdom;};
    void Execute();
};

#endif