
namespace Jde::DB
{
	struct WhereClause final
	{
		WhereClause( sv init )ι{ Add(init); }
		friend α operator<<( WhereClause &self, sv clause )ι->WhereClause&{ self.Add(clause); return self; }
		α Add(sv x)ι->void{ if( x.size() )_clauses.push_back(string{x}); }
		α Move()->string
		{
			string result = _clauses.size() ? "where" : "";
			for( int i=0; i<_clauses.size(); ++i )
				result += format( " {} {}", i==0 ? "" : "and", move(_clauses[i]) );
			return result;
		}

	private:
		 vector<string> _clauses;
	};
}