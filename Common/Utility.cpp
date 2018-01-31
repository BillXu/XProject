//
//  Utility.cpp
//  God
//
//  Created by Xu BILL on 12-11-23.
//
//

#include "Utility.h"
#include "log4z.h"
void StringSplit( std::string& str, const char* delim, VEC_STRING& vOutString)
{
	const int len = strlen(delim);
	std::size_t index = 0;
	std::size_t pos = str.find(delim, index);
	while ( pos != std::string::npos )
	{
		auto ss = str.substr(index, pos - index);
		index = pos + len;
		pos = str.find(delim, index);
		vOutString.push_back(ss);
	}

	//cout << "is last?" << " index:" << index << " str.length():" << str.length() << endl;  
	if ((index + 1) <= str.length())
	{
		auto ss = str.substr(index, str.length() - index);
		vOutString.push_back(ss);
	}
}

void StringSplit( const char* pString, char pSplitChar, VEC_STRING& vOutString )
{
	std::string str = pString;
	std::string strDe;
	strDe.push_back(pSplitChar);
	StringSplit(str, strDe.c_str(), vOutString);
}

void parseESSpace(std::string& pString) {
	pString.erase(0, pString.find_first_not_of(" "));
	pString.erase(pString.find_last_not_of(" ") + 1);
}

std::string TimeToStringFormate( unsigned int nSec )
{
    char pBuffer[100] = { 0 } ;
#ifdef SERVER
    unsigned int nSecond = nSec % 60 ;
    unsigned int nMin = (nSec - nSecond ) / 60 ;
    unsigned int nMinite = nMin % 60 ;
    unsigned int nHou = ( nMin - nMinite ) / 60 ;
    unsigned int nHour = nHou % 24 ;
    unsigned int nDay = ( nHou - nHour ) / 24 ;
    if ( nDay > 0 )
    {
        sprintf_s(pBuffer, "%d天 : %d : %d : %d", nDay,nHour,nMinite,nSecond );
    }
    else
    {
        sprintf_s(pBuffer, "%d : %d : %d",nHour,nMinite,nSecond );
    }
#endif
    return pBuffer ;
}

void RET_ILSEQ() {
	//std::cout << "WRONG FROM OF THE SEQUENCE" << std::endl;
	//exit(1);
	LOGFMTE("WRONG FROM OF THE SEQUENCE");
}


void RET_TOOFEW() {
	//std::cout << "MISSING FROM THE SEQUENCE" << std::endl;
	//exit(1);
	LOGFMTE("MISSING FROM THE SEQUENCE");
}

std::vector<std::string> parse(std::string sin, bool& bValid) {
	int l = sin.length();
	std::vector<std::string> ret;
	ret.clear();
	bValid = true;
	for (int p = 0; p < l;) {
		int size = 0, n = l - p;
		unsigned char c = sin[p], cc = sin[p + 1];
		if (c < 0x80) {
			size = 1;
		}
		else if (c < 0xc2) {
			RET_ILSEQ();
			bValid = false;
			break;
		}
		else if (c < 0xe0) {
			if (n < 2) {
				RET_TOOFEW();
				bValid = false;
				break;
			}
			if (!((sin[p + 1] ^ 0x80) < 0x40)) {
				RET_ILSEQ();
				bValid = false;
				break;
			}
			size = 2;
		}
		else if (c < 0xf0) {
			if (n < 3) {
				RET_TOOFEW();
				bValid = false;
				break;
			}
			if (!((sin[p + 1] ^ 0x80) < 0x40 &&
				(sin[p + 2] ^ 0x80) < 0x40 &&
				(c >= 0xe1 || cc >= 0xa0))) {
				RET_ILSEQ();
				bValid = false;
				break;
			}
			size = 3;
		}
		else if (c < 0xf8) {
			if (n < 4) {
				RET_TOOFEW();
				bValid = false;
				break;
			}
			if (!((sin[p + 1] ^ 0x80) < 0x40 &&
				(sin[p + 2] ^ 0x80) < 0x40 &&
				(sin[p + 3] ^ 0x80) < 0x40 &&
				(c >= 0xf1 || cc >= 0x90))) {
				RET_ILSEQ();
				bValid = false;
				break;
			}
			size = 4;
		}
		else if (c < 0xfc) {
			if (n < 5) {
				RET_TOOFEW();
				bValid = false;
				break;
			}
			if (!((sin[p + 1] ^ 0x80) < 0x40 &&
				(sin[p + 2] ^ 0x80) < 0x40 &&
				(sin[p + 3] ^ 0x80) < 0x40 &&
				(sin[p + 4] ^ 0x80) < 0x40 &&
				(c >= 0xfd || cc >= 0x88))) {
				RET_ILSEQ();
				bValid = false;
				break;
			}
			size = 5;
		}
		else if (c < 0xfe) {
			if (n < 6) {
				RET_TOOFEW();
				bValid = false;
				break;
			}
			if (!((sin[p + 1] ^ 0x80) < 0x40 &&
				(sin[p + 2] ^ 0x80) < 0x40 &&
				(sin[p + 3] ^ 0x80) < 0x40 &&
				(sin[p + 4] ^ 0x80) < 0x40 &&
				(sin[p + 5] ^ 0x80) < 0x40 &&
				(c >= 0xfd || cc >= 0x84))) {
				RET_ILSEQ();
				bValid = false;
				break;
			}
			size = 6;
		}
		else {
			RET_ILSEQ();
			bValid = false;
			break;
		}
		std::string temp = "";
		temp = sin.substr(p, size);
		ret.push_back(temp);
		p += size;
	}
	return ret;
}

std::string checkStringForSql(const char* pstr)
{
	bool bValid = false;
	std::vector<std::string> strArray = parse(pstr, bValid);
	if (false == bValid)
	{
		LOGFMTE("error invlid name str %s", pstr);
		std::string strTemp = "";
		char pBuffer[200] = { 0 };
		sprintf_s(pBuffer, sizeof(pBuffer), "guest%u", rand() % 10000 + 1);
		strTemp = pBuffer;
		return strTemp;
	}

	std::string strout;
	for (auto& ref : strArray)
	{
		strout += ref;
		if (ref == "'")
		{
			strout += ref;
		}
	}

	return strout;
}
