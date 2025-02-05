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

}

#endif // !__MATERIAL__H__
