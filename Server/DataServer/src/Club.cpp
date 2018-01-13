#include "ClubDefine.h"
#include "Club.h"
#include <string>

void CClub::setName(const char* cName) {
	sprintf_s(stBaseData.cName, "%s", cName);
}