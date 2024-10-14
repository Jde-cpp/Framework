#include <jde/framework/io/json/JObject.h>

#define let const auto
#define $ template<> α

namespace Jde{
	constexpr ELogTags _tags{ ELogTags::Parsing };
	JObject::JObject( jvalue v )ι:
		JObject{ v.is_object() ? v.as_object() : jobject{} }{
		if( !v.is_object() )
			Debug{ _tags, "Expected object, but found: {}.", underlying(v.kind()) };
	}

	$ JObject::Find<sv>( sv path )Ι->optional<sv>{
		auto p = find( path );
		return p!=end() && p->value().is_string() ? p->value().as_string() : optional<sv>{};
	}

	$ JObject::Find<bool>( sv path )Ι->optional<bool>{
		auto p = find( path );
		return p!=end() && p->value().is_bool() ? p->value().as_bool() : optional<bool>{};
	}
	$ JObject::Find<fs::path>( sv path )Ι->optional<fs::path>{
		let p = Find<string>( path );
		return p ? fs::path{*p} : optional<fs::path>{};
	}

	$ JObject::Get<sv>( sv path, SL sl )Ε->sv{
		auto p = Find<sv>( path ); THROW_IFSL( !p, "Path: '{}' not found in: '{}'.", path, serialize(*this) );
		return *p;
	}

	$ JObject::FindPtr<jvalue>( sv path )Ι->const jvalue*{
		auto p = jobject::find( path );
		return p!=end() ? &p->value() : nullptr;
	}

	$ JObject::FindPtr<jobject>( sv path )Ι->const jobject*{
		let p = FindPtr<jvalue>( path );
		return p && p->is_object() ? &p->as_object() : nullptr;
	}
}