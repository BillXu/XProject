#include "PokerServer.h"
#include "Application.h"
#include "NiuNiu/NiuNiuPeerCard.h"

void Test()
{
	CNiuNiuPeerCard ttPeer;
	ttPeer.setOpts(true, true, true, true, true,true);

	CCard tCard;
	tCard.SetCard(CCard::eCard_Diamond, 7);
	ttPeer.addCompositCardNum(tCard.GetCardCompositeNum());
	tCard.SetCard(CCard::eCard_Club, 8);
	ttPeer.addCompositCardNum(tCard.GetCardCompositeNum());
	tCard.SetCard(CCard::eCard_Club, 9);
	ttPeer.addCompositCardNum(tCard.GetCardCompositeNum());
	tCard.SetCard(CCard::eCard_Club, 4);
	ttPeer.addCompositCardNum(tCard.GetCardCompositeNum());
	tCard.SetCard(CCard::eCard_Club, 13);
	ttPeer.addCompositCardNum(tCard.GetCardCompositeNum());
	auto nW = ttPeer.getWeight();
}

int main()
{
	//Test();
	CApplication theAplication(PokerServerApp::getInstance());
	theAplication.startApp();
	return 0;
}