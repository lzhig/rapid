#pragma once

namespace rapidlib
{
	template<class T>
	class r_singleton_instance
	{
	public:
		static void create_instance()
		{
			if (mp_instance == nullptr)
			{
				mp_instance = new T();
			}
		}

		static void destroy_instance()
		{
			if (mp_instance)
			{
				delete mp_instance;
				mp_instance = nullptr;
			}
		}

		inline static T* get_instance()
		{
			return mp_instance;
		}

	protected:
		r_singleton_instance() {}
		~r_singleton_instance() {}

		static T* mp_instance;
	};

	template<class T>
	T* r_singleton_instance<T>::mp_instance = nullptr;
}
