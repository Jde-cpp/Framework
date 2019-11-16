#pragma once

namespace Jde
{
	template <typename T>
	struct EmptyPtr
	{
		static std::shared_ptr<T> Value;
	};

	template <typename T>
	std::shared_ptr<T> EmptyPtr<T>::Value = std::shared_ptr<T>(nullptr);

	template <typename T>
	struct SPCompare
	{
		//using is_transparent = void;
		bool operator()( const sp<T>& a, const sp<T>& b )const{
			return *a < *b;
		}

		bool operator() (const sp<T>& a, const T& b) const
		{
			return *a < b;
		}

		bool operator()( const T& a, const sp<T>& b) const
		{
			return a < *b;
		}
	};
}