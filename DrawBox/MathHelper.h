#ifndef __MATRIX__H__
#define __MATRIX__H__

#include <DirectXMath.h>

namespace DSM {
	struct MathHelper
	{
		static DirectX::XMFLOAT4X4 Identity() noexcept;

		inline static float PI = 3.1415926535f;
	};

}

#endif // !__MATRIX__H__
