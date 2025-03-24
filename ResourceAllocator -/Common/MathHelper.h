#ifndef __MATRIX__H__
#define __MATRIX__H__

#include <DirectXMath.h>

namespace DSM {
	struct MathHelper
	{
		static DirectX::XMFLOAT4X4 Identity() noexcept;

		static DirectX::XMMATRIX InverseTranspose(DirectX::FXMMATRIX matrix) noexcept;

		static DirectX::XMMATRIX InverseTransposeWithOutTranslate(DirectX::FXMMATRIX matrix) noexcept;

		static float RandomFloat(float min = 0, float max = 1);
		static DirectX::XMFLOAT3 RandomVector(float min = 0, float max = 1);
		
		inline static float PI = 3.1415926535f;
	};

}

#endif // !__MATRIX__H__
