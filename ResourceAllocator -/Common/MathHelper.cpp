#include "MathHelper.h"

using namespace DirectX;

namespace DSM {
	XMFLOAT4X4 MathHelper::Identity() noexcept
	{
		return XMFLOAT4X4(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
	}

	XMMATRIX MathHelper::InverseTranspose(FXMMATRIX matrix) noexcept
	{
		auto detWorld = XMMatrixDeterminant(matrix);
		XMMATRIX invWorld = XMMatrixInverse(&detWorld, matrix);
		return XMMatrixTranspose(invWorld);
	}

	DirectX::XMMATRIX MathHelper::InverseTransposeWithOutTranslate(DirectX::FXMMATRIX matrix) noexcept
	{
		XMMATRIX M = matrix;
		M.r[3] = g_XMIdentityR3;
		return InverseTranspose(M);
	}
}
