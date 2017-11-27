#include "MJServer.h"
#include "Application.h"
#include "MJPlayerCard.h"

#include "IMJPoker.h"
void tempTest()
{
	char pBuffer[] = { "{\"msgID\":260697,\"W\":32.00448748192937,\"msgID\":2601}" };
	Json::Reader reader;
	Json::Value rootValue;
	auto bRet = reader.parse(pBuffer, pBuffer + strlen(pBuffer), rootValue, false);
	auto is = rootValue.isObject();
	if (bRet)
	{
		int a = 0;
	}


	MJPlayerCard peer;
	peer.addDistributeCard(make_Card_Num(eCT_Tong,3));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 3));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 5));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 5));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 4));

	peer.addDistributeCard(make_Card_Num(eCT_Tong, 4));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 6));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 6));
	uint8_t nJiang = 0;
	auto nRet = peer.isHoldCardCanHu(nJiang);
	if (nRet )
	{
		printf("ok hu");
	}
	else
	{
		printf("not hu ");
	}
}
int main()
{
	tempTest();
	CApplication theAplication(CMJServerApp::getInstance());
	theAplication.startApp();
	return 0;
}