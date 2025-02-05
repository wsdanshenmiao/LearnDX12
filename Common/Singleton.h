#pragma once
#ifndef __SINGLETON__H__
#define __SINGLETON__H__

#include <assert.h>

namespace DSM {

	// 需要显式初始化且可随时释放的单例模板
	template<typename T>
	class Singleton
	{
	public:
		Singleton(const T&) = delete;
		void operator= (const T&) = delete;

		template<typename... Args>
		static void Create(Args... args);
		static void ShutDown() noexcept;
		static T& GetInstance() noexcept;


	protected:
		Singleton() = default;
		virtual ~Singleton() = default;

	protected:
		static inline T* m_Instance = nullptr;
	};

	template<typename T>
	template<typename ...Args>
	inline void Singleton<T>::Create(Args ...args)
	{
		m_Instance = new T{ args... };
	}

	template<typename T>
	inline void Singleton<T>::ShutDown() noexcept
	{
		delete m_Instance;
		m_Instance = nullptr;
	}

	template<typename T>
	inline T& Singleton<T>::GetInstance() noexcept
	{
		assert(m_Instance);
		return *m_Instance;
	}

}


#endif // !__SINGLETON__H__
