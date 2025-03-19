#pragma once
#ifndef __SHADERPROPERTY__H__
#define __SHADERPROPERTY__H__

#include "Pubh.h"

namespace DSM {
    using ShaderProperty = std::variant<
        int, uint32_t, float, DirectX::XMFLOAT2, DirectX::XMFLOAT3, DirectX::XMFLOAT4, DirectX::XMFLOAT4X4, 
        std::vector<float>, std::vector<DirectX::XMFLOAT4>, std::vector<DirectX::XMFLOAT4X4>,
        std::string>;
}

#endif
