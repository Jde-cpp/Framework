#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <list>

#include <optional>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <string_view>
#include <vector>
#include <boost/container/flat_map.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/ostr.h>



namespace Jde
{
	template<typename T>
	constexpr auto ms = std::make_shared<T>;

	template<typename T>
	constexpr auto mu = std::make_unique<T>;

	typedef uint_fast8_t uint8;//todo make const
	typedef int_fast8_t int8;

	typedef uint_fast16_t uint16;
	typedef int_fast16_t int16;
	//typedef uint_fast16_t uint16;
//	typedef int_fast16_t int16;

	typedef uint_fast32_t uint32;
	typedef int_fast32_t int32;
//	typedef uint_fast32_t uint32;
// typedef int_fast32_t int32;

	typedef uint_fast64_t uint; //todo make const
	typedef int_fast64_t _int; //todo make const
	typedef std::chrono::steady_clock SClock;
	typedef SClock::duration SDuration;
	typedef SClock::time_point STimePoint;

	using std::array;
	using std::lock_guard;
	using std::make_unique;
	using std::make_shared;
	using std::mutex;
	using std::shared_ptr;
	using std::string;
	using std::string_view;
	using std::tuple;
	using std::unique_ptr;
	using std::set; //https://stackoverflow.com/questions/19480918/is-it-possible-to-know-if-a-header-is-included	//https://stackoverflow.com/questions/36117884/compile-time-typeid-for-every-type -> https://stackoverflow.com/questions/35941045/can-i-obtain-c-type-names-in-a-constexpr-way/35943472#35943472 ->
	using std::unique_lock;
	using std::shared_lock;
	using std::shared_mutex;
	//using std::cout;
	using std::move;
	using std::endl;
	using std::optional;
	using std::ostringstream;
	using std::chrono::duration_cast;
	using std::make_tuple;
	using std::list;
	using boost::container::flat_multimap;

	template<class T> using sp = std::shared_ptr<T>;
	template<class T> using up = std::unique_ptr<T>;

	using std::vector;
	template<class T> using VectorPtr = std::shared_ptr<std::vector<T>>;

	using std::map;
	template<class K, class V> using MapPtr = std::shared_ptr<std::map<K,V>>;
	template<class K, class V> using UMapPtr = std::unique_ptr<std::map<K,V>>;
	template<class K, class V> using UnorderedPtr = std::shared_ptr<std::unordered_map<K,V>>;
	using std::multimap;
	template<class T, class Y> using MultiMapPtr = std::shared_ptr<std::multimap<T,Y>>;

	typedef unsigned short PortType;
/*	using std::function;

	using std::vector;
*/
	namespace fs=std::filesystem;
	using fmt::format;
	using path = const fs::path&;
	using sv = std::string_view;
}
#ifdef _MSC_VER
	#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif