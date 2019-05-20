#include "MJServer.h"
#include "Application.h"
#include "MJPlayerCard.h"
#include "DDMJPlayerCard.h"

#include "IMJPoker.h"
void tempTest()
{
	DDMJPlayerCard peer;
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 6));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 7));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 7));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 7));
	peer.addDistributeCard(make_Card_Num(eCT_Tong, 8));

	peer.addDistributeCard(make_Card_Num(eCT_Wan, 4));
	peer.addDistributeCard(make_Card_Num(eCT_Wan, 5));
	peer.addDistributeCard(make_Card_Num(eCT_Wan, 6));

	peer.addDistributeCard(make_Card_Num(eCT_Tiao, 4));
	peer.addDistributeCard(make_Card_Num(eCT_Tiao, 4));
	peer.addDistributeCard(make_Card_Num(eCT_Tiao, 7));
	peer.addDistributeCard(make_Card_Num(eCT_Tiao, 8));
	peer.addDistributeCard(make_Card_Num(eCT_Tiao, 9));
	uint8_t nJiang = 0;
	auto nRet = peer.canHuWitCard(make_Card_Num(eCT_Tong, 7));
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