#include "MathHelper.h"
#include <random>

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

	float MathHelper::RandomFloat(float min, float max)
	{
		std::random_device seed{};
		std::mt19937_64 gen{seed()};
		std::uniform_real_distribution<float> dist{min, max};
		return dist(gen);
	}

	DirectX::XMFLOAT3 MathHelper::RandomVector(float min, float max)
	{
		return DirectX::XMFLOAT3{RandomFloat(min, max), RandomFloat(min, max), RandomFloat(min, max)};
	}
}
