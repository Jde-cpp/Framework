#pragma once
namespace Jde::DB::Types
{
	struct Table;
	struct Schema
	{
	public:
		std::map<string,Table> _tables;
	};
}
