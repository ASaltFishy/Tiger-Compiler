#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
Result::~Result(){
    delete coloring_;
    delete il_;
}


void RegAllocator::RegAlloc(){

}
std::unique_ptr<ra::Result> RegAllocator::TransferResult(){
    return std::move(result_);
}
} // namespace ra