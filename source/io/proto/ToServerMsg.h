#pragma once

namespace Jde::ToServerMsg{
	using Jde::Logging::Proto::ToServer;
	Ξ AddLogin( str domain, str loginName, uint32 providerId )ι->ToServer{
			ToServer t;
			auto p = t.add_messages()->mutable_add_session();
			p->set_domain( domain );
			p->set_login_name( loginName );
			p->set_provider_id( providerId );
			return t;
	}
	Ξ GraphQL( str query, uint requestId )ι->ToServer{
			ToServer t;
			auto p = t.add_messages()->mutable_graph_ql();
			p->set_query( query );
			p->set_request_id( requestId );
			return t;
	}
}