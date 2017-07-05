#pragma once 
#include "IPlayerComponent.h"
#include "NativeTypes.h"
#include <string>
#include "CommonDefine.h"
#include <list>
class CPlayer;
struct stMail;
class CPlayerMailComponent
	:public IPlayerComponent
{
public:

public:
	CPlayerMailComponent(CPlayer* pPlayer ):IPlayerComponent(pPlayer){}
	~CPlayerMailComponent(){ }
};