#include "Geometry.h"
#include <array>
#include <cmath>
#include <algorithm>

using namespace DirectX;

namespace DSM {
	namespace Geometry {
		GeometryMesh GeometryGenerator::CreateBox(
			float width,
			float height,
			float depth,
			std::uint32_t subdivision) noexcept
		{
			GeometryMesh mesh{};
			// 这里细分到第六次会网格出现问题
			subdivision = std::min<std::uint32_t>(subdivision, 5u);

			std::array<Vertex, 24> v;

			float w2 = 0.5f * width;
			float h2 = 0.5f * height;
			float d2 = 0.5f * depth;

			// Fill in the front face vertex data.
			XMFLOAT3 front = { 0.0f, 0.0f, -1.0f };
			XMFLOAT3 back = { 0.0f, 0.0f, 1.0f };
			XMFLOAT3 top = { 0.0f, 1.0f, 0.0f };
			XMFLOAT3 button = { 0.0f, -1.0f, 0.0f };
			XMFLOAT3 left = { -1.0f, 0.0f, 0.0f };
			XMFLOAT3 right = { 1.0f, 0.0f, 0.0f };
			v[0] = { {-w2, -h2, -d2}, front, {1.0f, 0.0f, 0.0f, 0.0f}, {},{0.0f, 1.0f} };
			v[1] = { { -w2, +h2, -d2 }, front, {1.0f, 0.0f, 0.0f, 1.0f}, {},{0.0f, 0.0f} };
			v[2] = { { +w2, +h2, -d2 }, front, {1.0f, 0.0f, 0.0f, 1.0f}, {},{1.0f, 0.0f} };
			v[3] = { { +w2, -h2, -d2 }, front, {1.0f, 0.0f, 0.0f, 1.0f}, {},{1.0f, 1.0f} };

			// Fill in the back face vertex data.
			v[4] = { { -w2, -h2, +d2 }, back, { 0.0f, 1.0f, 0.0f ,1.0f}, {},{1.0f, 1.0f} };
			v[5] = { { +w2, -h2, +d2 }, back, { 0.0f, 1.0f, 0.0f ,1.0f}, {},{0.0f, 1.0f} };
			v[6] = { { +w2, +h2, +d2 }, back, { 0.0f, 1.0f, 0.0f ,1.0f}, {},{0.0f, 0.0f} };
			v[7] = { { -w2, +h2, +d2 }, back, { 0.0f, 1.0f, 0.0f ,1.0f}, {},{1.0f, 0.0f} };

			// Fill in the top face vertex data.
			v[8] = { { -w2, +h2, -d2 }, top, {1.0f, 0.0f, 0.0f, 1.0f}, {},{0.0f, 1.0f} };
			v[9] = { { -w2, +h2, +d2 }, top, {1.0f, 0.0f, 0.0f, 1.0f}, {},{0.0f, 0.0f} };
			v[10] = { { +w2, +h2, +d2 }, top, {1.0f, 0.0f, 0.0f, 1.0f}, {},{1.0f, 0.0f} };
			v[11] = { { +w2, +h2, -d2 }, top, {1.0f, 0.0f, 0.0f, 1.0f}, {},{1.0f, 1.0f} };

			// Fill in the bottom face vertex data.
			v[12] = { { -w2, -h2, -d2 }, button, { 0.0f, 1.0f, 0.0f ,1.0f}, {},{1.0f, 1.0f} };
			v[13] = { { +w2, -h2, -d2 }, button, { 0.0f, 1.0f, 0.0f ,1.0f}, {},{0.0f, 1.0f} };
			v[14] = { { +w2, -h2, +d2 }, button, { 0.0f, 1.0f, 0.0f ,1.0f}, {},{0.0f, 0.0f} };
			v[15] = { { -w2, -h2, +d2 }, button, { 0.0f, 1.0f, 0.0f ,1.0f}, {},{1.0f, 0.0f} };

			// Fill in the left face vertex data.
			v[16] = { { -w2, -h2, +d2 }, left, { 0.0f, 0.0f, -1.0f ,1.0f}, {},{0.0f, 1.0f} };
			v[17] = { { -w2, +h2, +d2 }, left, { 0.0f, 0.0f, -1.0f ,1.0f}, {},{0.0f, 0.0f} };
			v[18] = { { -w2, +h2, -d2 }, left, { 0.0f, 0.0f, -1.0f ,1.0f}, {},{1.0f, 0.0f} };
			v[19] = { { -w2, -h2, -d2 }, left, { 0.0f, 0.0f, -1.0f ,1.0f}, {},{1.0f, 1.0f} };

			// Fill in the right face vertex data.
			v[20] = { { +w2, -h2, -d2 }, right, { 0.0f, 0.0f, 1.0f ,1.0f}, {},{0.0f, 1.0f} };
			v[21] = { { +w2, +h2, -d2 }, right, { 0.0f, 0.0f, 1.0f ,1.0f}, {},{0.0f, 0.0f} };
			v[22] = { { +w2, +h2, +d2 }, right, { 0.0f, 0.0f, 1.0f ,1.0f}, {},{1.0f, 0.0f} };
			v[23] = { { +w2, -h2, +d2 }, right, { 0.0f, 0.0f, 1.0f ,1.0f}, {},{1.0f, 1.0f} };

			mesh.m_Vertices.assign(v.begin(), v.end());

			//
			// Create the indices.
			//

			std::uint32_t i[36];

			// Fill in the front face index data
			i[0] = 0; i[1] = 1; i[2] = 2;
			i[3] = 0; i[4] = 2; i[5] = 3;

			// Fill in the back face index data
			i[6] = 4; i[7] = 5; i[8] = 6;
			i[9] = 4; i[10] = 6; i[11] = 7;

			// Fill in the top face index data
			i[12] = 8; i[13] = 9; i[14] = 10;
			i[15] = 8; i[16] = 10; i[17] = 11;

			// Fill in the bottom face index data
			i[18] = 12; i[19] = 13; i[20] = 14;
			i[21] = 12; i[22] = 14; i[23] = 15;

			// Fill in the left face index data
			i[24] = 16; i[25] = 17; i[26] = 18;
			i[27] = 16; i[28] = 18; i[29] = 19;

			// Fill in the right face index data
			i[30] = 20; i[31] = 21; i[32] = 22;
			i[33] = 20; i[34] = 22; i[35] = 23;

			mesh.m_Indices32.assign(&i[0], &i[36]);

			// Put a cap on the number of subdivisions.
			for (std::uint32_t i = 0; i < subdivision; ++i)
				Subdivide(mesh);

			return mesh;
		}

		GeometryMesh GeometryGenerator::CreateCylinder(
			float topRadius,
			float buttonRadius,
			float height,
			std::uint32_t sliceCount,
			std::uint32_t stackCount) noexcept
		{
			GeometryMesh mesh;

			// 每一层的高度
			float stackHeight = height / stackCount;
			// 每一层的半径差
			float dr = topRadius - buttonRadius;
			float radiusStep = dr / stackCount;
			float ringCount = (float)stackCount + 1;
			std::uint32_t ringVertexCount = sliceCount + 1;

			// 创建顶点
			for (std::uint32_t i = 0; i < ringCount; ++i) {
				// 当前环的数据
				float y = .5f * -height + i * stackHeight;
				float r = buttonRadius + i * radiusStep;
				float thetaStep = XM_2PI / sliceCount;

				for (std::uint32_t j = 0; j <= sliceCount; ++j) {
					Vertex vertex{};
					float theta = j * thetaStep;
					float c = std::cosf(theta);
					float s = std::sinf(theta);

					XMFLOAT3 tangent = { -s,0,c };
					XMVECTOR T = XMLoadFloat3(&tangent);
					XMVECTOR B{ -dr * c,-height,-dr * s };
					XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
					vertex.m_Position = { c * r,y,s * r };
					vertex.m_TexCoord = { (float)j / sliceCount,(float)i / sliceCount };
					vertex.m_Tangent = { tangent.x,tangent.y,tangent.z,1 };
					XMStoreFloat3(&vertex.m_Normal, N);

					mesh.m_Vertices.push_back(std::move(vertex));

					//	i+1	*-----------*
					//		|           |
					//		|           |
					//		*-----------*
					//	i	 j           j+1

					// 一层中的一个面片
					if (i != stackCount && j != sliceCount) {
						mesh.m_Indices32.push_back(i * ringVertexCount + j);
						mesh.m_Indices32.push_back((i + 1) * ringVertexCount + j);
						mesh.m_Indices32.push_back((i + 1) * ringVertexCount + j + 1);

						mesh.m_Indices32.push_back(i * ringVertexCount + j);
						mesh.m_Indices32.push_back((i + 1) * ringVertexCount + j + 1);
						mesh.m_Indices32.push_back(i * ringVertexCount + j + 1);
					}
				}
			}

			auto vertexFunc = [](Vertex& vertex, float height) {
				auto& position = vertex.m_Position;
				position.y += height * .5f;
				vertex.m_TexCoord = { position.x / height + 0.5f ,position.z / height + 0.5f }; };
			auto topVertexFunc = [=](Vertex& vertex) {vertexFunc(vertex, height); };
			auto buttonVertexFunc = [=](Vertex& vertex) {vertexFunc(vertex, -height); };

			auto indexFunc = [&mesh](std::uint32_t index) {
				mesh.m_Indices32.push_back(index + (std::uint32_t)mesh.m_Vertices.size()); };

			auto topMesh = CreatePolygon(topRadius, sliceCount);
			auto buttonMesh = CreatePolygon(buttonRadius, sliceCount);

			mesh = MergeMesh(mesh, topMesh, topVertexFunc, [](auto& i) {});

			std::reverse(buttonMesh.m_Indices32.begin(), buttonMesh.m_Indices32.end());
			mesh = MergeMesh(mesh, buttonMesh, buttonVertexFunc, [](auto& i) {});

			return mesh;
		}

		GeometryMesh GeometryGenerator::CreatePolygon(float radius, std::uint32_t sliceCount) noexcept
		{
			GeometryMesh mesh{};

			mesh.m_Vertices.push_back(Vertex{ { 0,0,0 }, { 0,1,0 }, { 1,0,0,1 }, {},{ .5f,.5f } });
			float thetaStep = XM_2PI / sliceCount;
			for (std::uint32_t i = 0; i <= sliceCount; ++i) {
				Vertex vertex{};
				float theta = i * thetaStep;
				float c = std::cosf(theta);
				float s = std::sinf(theta);
				vertex.m_Position = { radius * c,0,radius * s };
				vertex.m_Normal = { 0,1,0 };
				vertex.m_Tangent = { -s,0,c,1 };
				vertex.m_TexCoord = { c,s };
				mesh.m_Vertices.push_back(std::move(vertex));

				if (i != sliceCount) {
					mesh.m_Indices32.push_back(0);
					mesh.m_Indices32.push_back(i + 2);
					mesh.m_Indices32.push_back(i + 1);
				}
			}
			return mesh;
		}

		GeometryMesh GeometryGenerator::CreateSphere(
			float radius,
			std::uint32_t sliceCount,
			std::uint32_t stackCount) noexcept
		{
			GeometryMesh mesh{};
			sliceCount = std::max(2u, sliceCount);
			stackCount = std::max(2u, stackCount);

			Vertex topVertex{ {0,radius,0},{0,1,0},{1,0,0,1},{},{0,0} };
			Vertex buttonVertex{ {0,-radius,0},{0,-1,0},{1,0,0,1},{},{0,1} };

			mesh.m_Vertices.push_back(std::move(topVertex));

			float phiStep = XM_PI / stackCount;
			float thetaStep = XM_2PI / sliceCount;
			// 生成頂點
			for (std::uint32_t i = 1; i <= stackCount - 1; ++i) {
				float phi = i * phiStep;
				float cphi = std::cosf(phi);
				float sphi = std::sinf(phi);
				for (std::uint32_t j = 0; j <= sliceCount; ++j) {
					float theta = j * thetaStep;
					float ctheta = std::cosf(theta);
					float stheta = std::sinf(theta);
					Vertex vertex{};
					vertex.m_Position = { radius * sphi * ctheta,radius * cphi,radius * sphi * stheta };
					vertex.m_TexCoord = { theta / XM_2PI,phi / XM_PI };
					XMVECTOR T = { radius * sphi * -stheta,radius * cphi,radius * sphi * ctheta,1 };
					XMVECTOR N = XMLoadFloat3(&vertex.m_Position);
					XMVector3Normalize(N);
					XMStoreFloat4(&vertex.m_Tangent, XMVector3Normalize(T));
					XMStoreFloat3(&vertex.m_Normal, XMVector3Normalize(N));
					mesh.m_Vertices.push_back(std::move(vertex));
				}
			}
			mesh.m_Vertices.push_back(std::move(buttonVertex));

			// 生成頂部索引
			for (std::uint32_t i = 1; i <= sliceCount; ++i) {
				mesh.m_Indices32.push_back(0);
				mesh.m_Indices32.push_back(i + 1);
				mesh.m_Indices32.push_back(i);
			}

			//	i	*-----------*
			//		|           |
			//		|           |
			//		*-----------*
			//	i+1	 j           j+1

			// 生成中間索引
			std::uint32_t ringVertexCount = sliceCount + 1;
			for (std::uint32_t i = 0; i < stackCount - 2; ++i) {
				for (std::uint32_t j = 1; j <= sliceCount; ++j) {	// 去掉頂點
					mesh.m_Indices32.push_back(i * ringVertexCount + j);
					mesh.m_Indices32.push_back((i + 1) * ringVertexCount + j + 1);
					mesh.m_Indices32.push_back((i + 1) * ringVertexCount + j);

					mesh.m_Indices32.push_back(i * ringVertexCount + j);
					mesh.m_Indices32.push_back(i * ringVertexCount + j + 1);
					mesh.m_Indices32.push_back((i + 1) * ringVertexCount + j + 1);
				}
			}

			// 生成底部索引
			std::uint32_t lastIndex = (std::uint32_t)mesh.m_Vertices.size() - 1;
			std::uint32_t baseIndex = lastIndex - ringVertexCount;
			for (std::uint32_t i = 0; i < sliceCount; ++i) {
				mesh.m_Indices32.push_back(lastIndex);
				mesh.m_Indices32.push_back(baseIndex + i);
				mesh.m_Indices32.push_back(baseIndex + i + 1);
			}


			return mesh;
		}

		GeometryMesh GeometryGenerator::CreateGeosphere(float radius, std::uint32_t subdivision) noexcept
		{
			GeometryMesh mesh{};
			subdivision = std::min(5u, subdivision);
			// 通过正十二边形近似圆
			const float X = 0.525731f;
			const float Z = 0.850651f;
			std::array<XMFLOAT3, 12> vertices = {
				XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
				XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
				XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
				XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
				XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
				XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
			};

			std::array<std::uint32_t, 60> indices =
			{
				1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
				1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
				3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
				10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
			};

			mesh.m_Vertices.resize(12);
			mesh.m_Indices32.assign(indices.begin(), indices.end());

			for (std::uint32_t i = 0; i < 12; ++i)
				mesh.m_Vertices[i].m_Position = vertices[i];

			for (std::uint32_t i = 0; i < subdivision; ++i) {
				Subdivide(mesh);
			}
			for (std::uint32_t i = 0; i < mesh.m_Vertices.size(); ++i)
			{
				// 将正六面体的位置投影到圆上
				auto& vertex = mesh.m_Vertices[i];
				XMVECTOR N = XMVector3Normalize(XMLoadFloat3(&vertex.m_Position));
				XMVECTOR P = radius * N;
				XMStoreFloat3(&vertex.m_Position, P);
				XMStoreFloat3(&vertex.m_Normal, N);

				// 还原出角度
				//	vertex.m_Position = { radius * ctheta,radius * cphi,radius * stheta };
				float theta = atan2f(vertex.m_Position.z, vertex.m_Position.x);
				//if (theta < 0.0f)
				//	theta += XM_2PI;
				float phi = acosf(vertex.m_Position.y / radius);

				vertex.m_TexCoord = { theta / XM_2PI, phi / XM_PI };

				// 重新计算切线
				vertex.m_Tangent.x = radius * sinf(phi) * -sinf(theta);
				vertex.m_Tangent.y = .0f;
				vertex.m_Tangent.z = radius * sinf(phi) * cosf(theta);
				vertex.m_Tangent.w = 1.f;

				XMVECTOR T = XMLoadFloat4(&vertex.m_Tangent);
				XMStoreFloat4(&vertex.m_Tangent, XMVector3Normalize(T));
			}

			return mesh;
		}

		GeometryMesh GeometryGenerator::CreateGrid(
			float width,
			float depth,
			std::uint32_t m,
			std::uint32_t n) noexcept
		{
			GeometryMesh mesh{};

			std::uint32_t vertexCout = m * n;
			std::uint32_t faceCount = (m - 1) * (n - 1) * 2;
			float halfWidth = width * 0.5;
			float halfDepth = depth * 0.5;

			// 每个小网格的分量
			float dx = width / (m - 1);
			float dz = depth / (n - 1);
			float du = 1.f / (m - 1);
			float dv = 1.f / (n - 1);

			// 生成顶点
			mesh.m_Vertices.resize(vertexCout);
			for (std::uint32_t i = 0; i < m; ++i) {
				float z = -halfDepth + i * dz;
				for (std::uint32_t j = 0; j < n; ++j) {
					float x = -halfWidth + j * dx;
					std::uint32_t index = i * n + j;
					mesh.m_Vertices[index].m_Position = { x, 0 ,z };
					mesh.m_Vertices[index].m_Normal = { 0,1,0 };
					mesh.m_Vertices[index].m_Tangent = { 1,0,0,1 };
					mesh.m_Vertices[index].m_TexCoord = { du * j, dv * i };
				}
			}

			/*	j			j+1
			 i+1-------------
				|			|
				|			|
				|			|
				|			|
			 i	-------------
			*/
			// 生成索引
			mesh.m_Indices32.resize(faceCount * 3);
			for (std::uint32_t i = 0, k = 0; i < m - 1; ++i) {
				for (std::uint32_t j = 0; j < n - 1; ++j) {
					std::uint32_t count = i * n + j;
					mesh.m_Indices32[k++] = count;
					mesh.m_Indices32[k++] = count + n + 1;
					mesh.m_Indices32[k++] = count + 1;
					mesh.m_Indices32[k++] = count;
					mesh.m_Indices32[k++] = count + n;
					mesh.m_Indices32[k++] = count + n + 1;
				}
			}

			return mesh;
		}

		template<typename VertFunc, typename IndexFunc>
		GeometryMesh GeometryGenerator::MergeMesh(
			const GeometryMesh& m0,
			const GeometryMesh& m1,
			VertFunc vertFunc,
			IndexFunc indexFunc) noexcept
		{
			GeometryMesh copyMesh = m0;
			GeometryMesh mergeMesh = m1;

			auto& mergeIndices = mergeMesh.m_Indices32;
			auto& mergeVertices = mergeMesh.m_Vertices;
			// 将m1的所有索引加上基础值,并使用闭包函数处理索引
			auto mergeIndexFunc = [&copyMesh, indexFunc](std::uint32_t& index) {
				index += (std::uint32_t)copyMesh.m_Vertices.size();
				indexFunc(index); };

			std::for_each(mergeIndices.begin(), mergeIndices.end(), mergeIndexFunc);
			copyMesh.m_Indices32.insert(copyMesh.m_Indices32.end(), mergeIndices.begin(), mergeIndices.end());

			std::for_each(mergeVertices.begin(), mergeVertices.end(), vertFunc);
			copyMesh.m_Vertices.insert(copyMesh.m_Vertices.end(), mergeVertices.begin(), mergeVertices.end());

			return copyMesh;
		}

		GeometryMesh GeometryGenerator::MergeMesh(const GeometryMesh& m0, const GeometryMesh& m1) noexcept
		{
			auto vertFunc = [](Vertex& vertex) {};
			auto indexFunc = [](std::uint32_t index) {};
			return MergeMesh(m0, m1, vertFunc, indexFunc);
		}

		void GeometryGenerator::Subdivide(GeometryMesh& mesh) noexcept
		{
			//       v1
			//       *
			//      / \
			//     /   \
			//  m0*-----*m1
			//   / \   / \
			//  /   \ /   \
			// *-----*-----*
			// v0    m2     v2
			GeometryMesh copyMesh = mesh;
			mesh.m_Vertices.resize(0);
			mesh.m_Indices32.resize(0);

			std::uint32_t numTriangles = (std::uint32_t)copyMesh.m_Indices32.size() / 3;
			for (std::uint32_t i = 0; i < numTriangles; ++i) {
				std::array<Vertex, 6> vs{};
				for (auto j = 0; j < 3; ++j) {
					vs[j] = copyMesh.m_Vertices[copyMesh.m_Indices32[3 * i + j]];
				}
				for (auto j = 3; j < 6; ++j) {
					vs[j] = MidPoint(vs[j - 3], vs[(j - 2) % 3]);
				}
				mesh.m_Vertices.insert(mesh.m_Vertices.end(), vs.begin(), vs.end());

				mesh.m_Indices32.push_back(i * 6 + 0);
				mesh.m_Indices32.push_back(i * 6 + 3);
				mesh.m_Indices32.push_back(i * 6 + 5);

				mesh.m_Indices32.push_back(i * 6 + 3);
				mesh.m_Indices32.push_back(i * 6 + 4);
				mesh.m_Indices32.push_back(i * 6 + 5);

				mesh.m_Indices32.push_back(i * 6 + 5);
				mesh.m_Indices32.push_back(i * 6 + 4);
				mesh.m_Indices32.push_back(i * 6 + 2);

				mesh.m_Indices32.push_back(i * 6 + 3);
				mesh.m_Indices32.push_back(i * 6 + 1);
				mesh.m_Indices32.push_back(i * 6 + 4);
			}

		}

		Vertex GeometryGenerator::MidPoint(const Vertex& v0, const Vertex& v1) noexcept
		{
			XMVECTOR p0 = XMLoadFloat3(&v0.m_Position);
			XMVECTOR p1 = XMLoadFloat3(&v1.m_Position);
			XMVECTOR n0 = XMLoadFloat3(&v0.m_Normal);
			XMVECTOR n1 = XMLoadFloat3(&v1.m_Normal);
			XMVECTOR t0 = XMLoadFloat4(&v0.m_Tangent);
			XMVECTOR t1 = XMLoadFloat4(&v1.m_Tangent);
			XMVECTOR tex0 = XMLoadFloat2(&v0.m_TexCoord);
			XMVECTOR tex1 = XMLoadFloat2(&v1.m_TexCoord);

			Vertex ret{};
			XMStoreFloat3(&ret.m_Position, (p0 + p1) * .5f);
			XMStoreFloat3(&ret.m_Normal, XMVector3Normalize(n0 + n1));
			XMStoreFloat4(&ret.m_Tangent, XMVector4Normalize(t0 + t1));
			XMStoreFloat2(&ret.m_TexCoord, (tex0 + tex1) * .5f);
			return ret;
		}

	}
}