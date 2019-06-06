#include "IGameOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
#include "CFMJOpts.h"
#include "DDMJOpts.h"
#include "FXMJOpts.h"
#include "LuoMJOpts.h"
#include "TestMJOpts.h"
#include "MQMJOpts.h"
#include "NCMJOpts.h"
#include "SCMJOpts.h"
#include "BJOpts.h"
#include "DDZOpts.h"
#include "AHMJOpts.h"
#include "GoldenOpts.h"
#include "NNOpts.h"
#include "SZMJOpts.h"
void IGameOpts::initRoomOpts(Json::Value& jsOpts) {
	m_jsOpts = jsOpts;
	setRoomID(0);

	if (jsOpts["gameType"].isUInt()) {
		m_nGameType = jsOpts["gameType"].asUInt();
	}
	else {
		LOGFMTE("room opts error, gameType type is incorrect");
	}

	if (jsOpts["seatCnt"].isUInt()) {
		m_nSeatCnt = jsOpts["seatCnt"].asUInt();
	}
	else {
		LOGFMTE("room opts error, seatCnt type is incorrect");
	}

	if (jsOpts["payType"].isUInt()) {
		m_nPayType = jsOpts["payType"].asUInt();
	}
	else {
		LOGFMTE("room opts error, payType type is incorrect");
	}

	if (jsOpts["level"].isUInt()) {
		m_nRoundLevel = jsOpts["level"].asUInt();
	}
	else {
		LOGFMTE("room opts error, level type is incorrect");
	}

	m_bDaiKai = jsOpts["isDK"].asBool();
	m_bEnableWhiteList = jsOpts["enableWhiteList"].asBool();
	m_bEnablePointRestrict = jsOpts["pointRct"].asBool();
	m_bAA = jsOpts["isAA"].asBool();

	m_nVipLevel = 0;
	if (jsOpts["vipLevel"].isUInt()) {
		m_nVipLevel = jsOpts["vipLevel"].asUInt();
	}

	m_nAutoStartCnt = 0;
	if (jsOpts["starGame"].isUInt()) {
		m_nAutoStartCnt = jsOpts["starGame"].asUInt();
	}

	m_nOwnerUID = 0;
	if (jsOpts["uid"].isUInt()) {
		m_nOwnerUID = jsOpts["uid"].asUInt();
	}

	m_nClubID = 0;
	if (jsOpts["clubID"].isUInt()) {
		m_nClubID = jsOpts["clubID"].asUInt();
	}
}

uint16_t IGameOpts::getDiamondNeed() {
	if (!m_bCalculateDiamondNeed) {
		m_nDiamondNeed = calculateDiamondNeed();
		m_bCalculateDiamondNeed = true;
	}
	return m_nDiamondNeed;
}

uint8_t IGameOpts::getInitRound() {
	if (!m_bCalculateInitRound) {
		m_nInitRound = calculateInitRound();
		m_bCalculateInitRound = true;
	}
	return m_nInitRound;
}

std::shared_ptr<IGameOpts> IGameOpts::parseOpts(Json::Value& jsOpts) {
	if (jsOpts["gameType"].isUInt() == false) {
		return nullptr;
	}
	//TODO
	std::shared_ptr<IGameOpts> pIGameOpts = nullptr;
	auto nGameType = jsOpts["gameType"].asUInt();
	switch (nGameType)
	{
	case eGame_CFMJ:
	{
		pIGameOpts = std::make_shared<CFMJOpts>();
	}
	break;
	case eGame_DDMJ:
	{
		pIGameOpts = std::make_shared<DDMJOpts>();
	}
	break;
	case eGame_FXMJ:
	{
		pIGameOpts = std::make_shared<FXMJOpts>();
	}
	break;
	case eGame_LuoMJ:
	{
		pIGameOpts = std::make_shared<LuoMJOpts>();
	}
	break;
	case eGame_TestMJ:
	{
		pIGameOpts = std::make_shared<TestMJOpts>();
	}
	break;
	case eGame_MQMJ:
	{
		pIGameOpts = std::make_shared<MQMJOpts>();
	}
	break;
	case eGame_NCMJ:
	{
		pIGameOpts = std::make_shared<NCMJOpts>();
	}
	break;
	case eGame_SCMJ:
	{
		pIGameOpts = std::make_shared<SCMJOpts>();
	}
	break;
	case eGame_AHMJ:
	{
		pIGameOpts = std::make_shared<AHMJOpts>();
	}
	break;
	case eGame_SZMJ:
	{
		pIGameOpts = std::make_shared<SZMJOpts>();
	}
	break;
	case eGame_BiJi:
	{
		pIGameOpts = std::make_shared<BJOpts>();
	}
	break;
	case eGame_CYDouDiZhu:
	case eGame_JJDouDiZhu:
	{
		pIGameOpts = std::make_shared<DDZOpts>();
	}
	break;
	case eGame_Golden:
	{
		pIGameOpts = std::make_shared<GoldenOpts>();
	}
	break;
	case eGame_NiuNiu:
	{
		pIGameOpts = std::make_shared<NNOpts>();
	}
	break;
	//TODO...
	default:
	{
		LOGFMTE(" can not create game opts with game type : %u", nGameType);
	}
	break;
	}
	if (pIGameOpts) {
		pIGameOpts->initRoomOpts(jsOpts);
	}
	return pIGameOpts;
}