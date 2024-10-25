#include <jde/framework/io/json.h>
#include <jde/framework/io/file.h>
#include <libjsonnet++.h>
#include <jde/framework/str.h>

#define let const auto
namespace Jde{
	jarray _emptyArray;
	jobject _emptyObject;
	α Json::ReadJsonNet( fs::path path, SL sl )ε->jobject{
		jsonnet::Jsonnet vm;
		vm.init();
		string j;
		bool success = vm.evaluateFile( path.string(), &j );
		THROW_IFSL( !success, "Failed to evaluate '{}'.  {}", path.string(), vm.lastError() );
		return Parse( j, sl );
	}
	
	α Json::Parse( sv json, SL sl )ε->jobject{ 
		std::error_code ec;
		auto value = boost::json::parse( json, ec );
		if( ec )
			throw CodeException{ ec, ELogTags::Parsing, Ƒ("Failed to parse '{}'.", json), ELogLevel::Debug, sl };
		return AsObject( value ); 
	}

	α Json::AsArray( const jvalue& j, SL sl )ε->const jarray&{
		auto y = j.try_as_array();
		if( !y )
			throw CodeException{ y.error(), ELogTags::Parsing, Ƒ("'{}', is not an array but is a '{}'.", serialize(j), Kind(j.kind())), ELogLevel::Debug, sl };
		return *y;
	}

	α Json::AsArray( const jobject& j, sv key, SL sl )ε->const jarray&{
		let& y = j.try_at( key );
		if( !y )
			throw CodeException{ y.error(), ELogTags::Parsing, Ƒ("Key '{}' not found in {}.", key, serialize(j)), ELogLevel::Debug, sl };
		return AsArray( *y, sl );
	}

	α Json::AsObject( const jvalue& j, SL sl )ε->const jobject&{
		let y = j.try_as_object();
		if( !y )
			throw CodeException{ y.error(), ELogTags::Parsing, Ƒ("'{}', is not an object but is a '{}'.", serialize(j), Kind(j.kind())), ELogLevel::Debug, sl };
		return *y;
	}
	α Json::AsObject( const jvalue& j, sv path, SL sl )ε->const jobject&{
		auto p = FindValuePtr( j, path ); THROW_IFSL( !p, "Path '{}' not found.", path );
		return AsObject( *p, sl );
	}
	α AsSV( const jobject& j, sv key, SL sl )ε->sv{
		let& y = j.try_at( key );
		if( !y )
			throw CodeException{ y.error(), ELogTags::Parsing, Ƒ("Key '{}' not found in {}.", key, serialize(j)), ELogLevel::Debug, sl };
		if( !y->is_string() )
			throw CodeException{ y.error(), ELogTags::Parsing, Ƒ("Key '{}' not a string {}.", key, serialize(j)), ELogLevel::Debug, sl };
		return y->get_string();
	}

	α Json::AsValue( const jobject& o, sv path, SL sl )ε->const jvalue&{ 
		auto p = FindValue( o, path ); THROW_IFSL( !p, "Path '{}' not found in '{}'.", path, serialize(o) );
		return *p;
	}
	α Json::AsSV( const jobject& j, sv key, SL sl )ε->sv{
		auto p = FindSV( j, key );
		THROW_IFSL( !p, "Key '{}' not found in '{}'.", key, serialize(j) );
		return *p;
	}

	α Json::FindValue( const jobject& j, sv path )ι->const jvalue*{
		auto keys = Str::Split( path, '/' );
		const jobject* jobj = &j;
		for( uint i=0; jobj && i<keys.size(); ++i ){
			if( i==keys.size()-1 ){
				auto p = jobj->if_contains( keys[i] );
				return p;
			}
			else 
				jobj = FindObject( *jobj, keys[i] );
		}
		//throw Exception{ sl, ELogLevel::Debug, "Path '{}' not found in '{}'", path, serialize(j) };
		return nullptr;
	}
	α Json::AsObject( const jobject& o, sv key, SL sl )ε->const jobject&{
		let& y = o.try_at( key );
		if( !y )
			throw CodeException{ y.error(), ELogTags::Parsing, Ƒ("Key '{}' not found in {}.", key, serialize(o)), ELogLevel::Debug, sl };
		return AsObject( *y, sl );
	}


	α Json::FindArray( const jvalue& v, sv path )ι->const jarray*{ 
		auto value = v.try_at_pointer( path ); 
		return value ? value->if_array() : nullptr; 
	}	
	α Json::FindArray( const jobject& o, sv key )ι->const jarray*{
		let& value = o.try_at( key );
		return value ? value->if_array() : nullptr;
	}
	α Json::FindDefaultArray( const jvalue& v, sv path )ι->const jarray&{ 
		auto p = FindArray( v, path ); 
		return p ? *p : _emptyArray; 
	}
	α Json::FindDefaultArray( const jobject& o, sv key )ι->const jarray&{
		let y = FindArray( o, key );
		return y ? *y : _emptyArray;
	}
	α Json::FindBool( const jobject& o, sv key )ι->optional<bool>{
		let value = o.if_contains( key );
		return value && value->is_bool() ? value->get_bool() : optional<bool>{};
	}
	α Json::FindDefaultObject( const jvalue& v, sv path )ι->const jobject&{ 
		auto p = Json::FindObject(v, path); 
		return p ? *p : _emptyObject; 
	};

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
