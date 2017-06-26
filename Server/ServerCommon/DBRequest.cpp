#include "DBRequest.h"
#include <assert.h>
stDBResult::~stDBResult()
{
	VEC_MYSQLROW::iterator iter = vResultRows.begin();
	for ( ; iter != vResultRows.end() ; ++iter )
	{
		delete *iter ;
		*iter = NULL ;
	}
	vResultRows.clear() ;
}

void stDBResult::reset()
{
	nRequestUID = 0;
	VEC_MYSQLROW::iterator iter = vResultRows.begin();
	for (; iter != vResultRows.end(); ++iter)
	{
		delete *iter;
		*iter = NULL;
	}
	vResultRows.clear();
	nAffectRow = 0;
	pUserData = nullptr;
}