#include "Table.h"
#define var const auto

namespace Jde::DB::Types
{
	Index::Index( const shared_ptr<Table>& pTable, string_view indexName, bool clustered, bool unique, bool primaryKey ):
		TablePtr{pTable},
		Name{indexName},
		Clustered{clustered},
		Unique{unique},
		PrimaryKey{primaryKey}
	{}

	sp<Column> Table::FindColumn( string_view name )
	{
		auto pColumn = find_if( Columns.begin(), Columns.begin(), [&name](var& c){return c->Name==name;} );
		return pColumn==Columns.end() ? nullptr : *pColumn;
	}
}