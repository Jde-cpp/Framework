#include <jde/framework/io/json.h>

#define let const auto
namespace Jde{
	jarray _emptyArray;
	jobject _emptyObject;
	α Json::AsObject( const jvalue& j, SL sl )ε->const jobject&{
		auto y = j.try_as_object();
		if( !y )
			throw CodeException{ y.error(), ELogTags::Parsing, Ƒ("'{}', is not an object but is a '{}'.", serialize(j), Kind(j.kind())), ELogLevel::Debug, sl };
		return *y;
	}
	α Json::AsObject( const jvalue& j, sv path, SL sl )ε->const jobject&{
		auto p = FindValue( j, path ); THROW_IFSL( !p, "Path '{}' not found.", path );
		return AsObject( *p, sl );
	}

	α Json::FindDefaultArray( const jvalue& j, sv path )ι->const jarray&{ auto p = FindArray(j,path); return p ? *p : _emptyArray; };
	α Json::FindDefaultObject( const jvalue& j, sv path )ι->const jobject&{ auto p = Json::FindObject(j, path); return p ? *p : _emptyObject; };

	α Json::Kind( boost::json::kind kind )ι->string{
		string y;
		switch( kind ){
			using enum boost::json::kind;
			case null: y = "null";
			case string: y = "string";
			case bool_: y = "bool";
			case int64: y = "int";
			case uint64: y = "uint";
			case double_: y = "double";
			case object: y = "object";
			case array: y = "array";
			default: y = "unknown";
		}
		return y;
	}

	//used to find a database value in a ql filter.
	α Json::Find( const jvalue& container, const jvalue& item )ι->const jvalue*{
		auto y = container.is_primitive() && item.is_primitive() && container==item ? &item : nullptr;
		if( let array = y || !container.is_array() ? nullptr : &container.get_array(); array ){
			for( auto p = array->begin(); !y && p!=array->end(); ++p )
				y = p->is_primitive() && container==*p ? &*p : nullptr;
		}
		return y;
	}
}
α Jde::operator<( const jvalue& a, const jvalue& b )ι->bool{
	bool less{};
	if( a.is_primitive() && b.is_primitive() ){
		if( a.is_string() && b.is_string() )
			less = a.get_string() < b.get_string();
		else if( a.is_double() && b.is_double() )
			less = a.get_double() < b.get_double();
		else if( a.is_number() && b.is_number() )
			less = a.to_number<uint>() < b.to_number<uint>();
		else if( a.is_bool() && b.is_bool() )
			less = a.get_bool() < b.get_bool();
	}
	return less;
}
