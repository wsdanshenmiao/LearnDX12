#pragma once
#ifndef __MATERIAL__H__
#define __MATERIAL__H__

#include "Pubh.h"

namespace DSM {
	struct Material
	{
		using MaterialProperty =
			std::variant<int, float, DirectX::XMFLOAT2, DirectX::XMFLOAT3, DirectX::XMFLOAT4, std::string>;

		std::size_t PropertySize()const;

		template<typename T>
		void Set(const std::string& name, T value);

		template<typename T>
		T* Get(const std::string& name);
		template<typename T>
		const T* Get(const std::string& name) const;
		template<typename T>
		T* TryGet(const std::string& name);
		template<typename T>
		const T* TryGet(const std::string& name)const;

		template<typename T>
		bool Has(const std::string& name)const;

		static Material GetDefaultMaterial();

		std::unordered_map<std::string, MaterialProperty> m_Properties;
	};

	inline std::size_t Material::PropertySize() const
	{
		return m_Properties.size();
	}

	template<typename T>
	inline void Material::Set(const std::string& name, T value)
	{
		m_Properties[name] = value;
	}

	template<typename T>
	inline T* Material::Get(const std::string& name)
	{
		if (auto it = m_Properties.find(name); it != m_Properties.end()) {
			return &std::get<T>(it->second);
		}
		return nullptr;
	}

	template<typename T>
	inline const T* Material::Get(const std::string& name) const
	{
		if (auto it = m_Properties.find(name);
			it != m_Properties.end() && std::holds_alternative<T>(it->second)) {
			return &std::get<T>(it->second);
		}
		return nullptr;
	}

	template<typename T>
	inline T* Material::TryGet(const std::string& name)
	{
		return const_cast<T*>(static_cast<const Material*>(this)->TryGet<T>(name));
	}

	template<typename T>
	const T* Material::TryGet(const std::string& name) const
	{
		if (auto it = m_Properties.find(name); it != m_Properties.end() && std::holds_alternative<T>(it->second)) {
			return std::get_if<T>(&it->second);
		}
		else {
			return nullptr;
		}
	}

	template<typename T>
	bool Material::Has(const std::string& name) const
	{
		auto it = m_Properties.find(name);
		return it != m_Properties.end() && std::holds_alternative<T>(it->second);
	}

	inline Material Material::GetDefaultMaterial()
	{
		Material material;
		material.m_Properties["AmbientColor"] = DirectX::XMFLOAT3{0.1f, 0.1f, 0.1f};
		material.m_Properties["DiffuseColor"] = DirectX::XMFLOAT3{1, 1, 1};
		material.m_Properties["SpecularColor"] = DirectX::XMFLOAT3{1, 1, 1};
		material.m_Properties["SpecularFactor"] = 0.2f;
		return material;
	}


}

#endif // !__MATERIAL__H__
