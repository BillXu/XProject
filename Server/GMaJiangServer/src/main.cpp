#include "MJServer.h"
#include "Application.h"
#include "MJPlayerCard.h"

#include "IMJPoker.h"
void tempTest()
{
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
	if (nRet)
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
	//tempTest();
	CApplication theAplication(CMJServerApp::getInstance());
	theAplication.startApp();
	return 0;
}