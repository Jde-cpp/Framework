#pragma once
#include "Row.h"
//#include "../../types/Schema.h"
//#include "../../DataSchemaProc.h"

namespace Jde::DB
{
	namespace Types{ struct IRow; }
	struct IDataSource
	{
		virtual ~IDataSource() = default;

		virtual uint Execute( string_view sql )noexcept(false)=0;
		virtual uint Execute( string_view sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual uint Execute( string_view sql, const std::vector<DataValue>& parameters, std::function<void(const IRow&)> f, bool log=true )noexcept(false)=0;
		virtual uint ExecuteProc( string_view sql, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual uint ExecuteProc( string_view sql, const std::vector<DataValue>& parameters, std::function<void(const IRow&)> f, bool log=true )noexcept(false)=0;

		virtual void Select( string_view sql, std::function<void(const IRow&)> f, const std::vector<DataValue>& parameters, bool log=true )noexcept(false)=0;
		virtual void Select( string_view sql, std::function<void(const IRow&)> f )noexcept(false)=0;
		void Log( string_view /*sql*/, const std::vector<DataValue>* /*pParameters*/ )noexcept{};
		string ConnectionString;
	};
}