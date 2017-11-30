#include "BJServer.h"
#include "Application.h"
//#include "./BiJi/BJCardTypeChecker.h"
//#include "BJPoker.h"
int main()
{
	//std::vector<uint8_t> vCompsitCard;
	//vCompsitCard.push_back(BJ_MAKE_CARD(ePoker_Club,2) );
	//vCompsitCard.push_back(BJ_MAKE_CARD(ePoker_Club, 1));
	//vCompsitCard.push_back(BJ_MAKE_CARD(ePoker_Joker, 15));
	//uint32_t nWeight = 0;
	//eBJCardType nType;
	//BJCardTypeChecker::getInstance()->checkCardType(vCompsitCard, nWeight, nType);

	CApplication theAplication(BJServerApp::getInstance());
	theAplication.startApp();
	return 0;
}