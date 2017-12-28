#include "PokerServer.h"
#include "Application.h"
#include "Golden/GoldenPeerCard.h"

void Test()
{
	/*CNiuNiuPeerCard ttPeer;
	ttPeer.setOpts(true, true, true, true, true);

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
	auto nW = ttPeer.getWeight();*/

	CGoldenPeerCard gCard;
	gCard.addCompositCardNum(2);
	gCard.addCompositCardNum(33);
	gCard.addCompositCardNum(28);
	auto gType = gCard.getType();

	CGoldenPeerCard gCard_1;
	gCard_1.addCompositCardNum(5);
	gCard_1.addCompositCardNum(31);
	gCard_1.addCompositCardNum(3);
	auto gType_1 = gCard_1.getType();

	if (gCard > gCard_1) {
		
	}

}

int main()
{
	//Test();
	CApplication theAplication(PokerServerApp::getInstance());
	theAplication.startApp();
	return 0;
}