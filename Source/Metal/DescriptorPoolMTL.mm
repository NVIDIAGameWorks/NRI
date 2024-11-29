
#include "SharedMTL.h"
#include "DescriptorPoolMTL.h"

using namespace nri;


DescriptorPoolMTL::~DescriptorPoolMTL() {
    
}

Result DescriptorPoolMTL::Create(const DescriptorPoolDesc& descriptorPoolDesc) {

}

//================================================================================================================
// NRI
//================================================================================================================

void DescriptorPoolMTL::SetDebugName(const char* name) {
    
}
void DescriptorPoolMTL::Reset() {
    
}
Result DescriptorPoolMTL::AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return Result::SUCCESS;
}

