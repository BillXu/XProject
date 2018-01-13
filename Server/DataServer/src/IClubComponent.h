#pragma once

enum eClubComponentType
{
	eClubComponent_None,
	eClubComponent_BaseData,
	eClubComponent_GameData,
	eClubComponent_Mail,            // last sit the last pos ,
	eClubComponent_Max,
};

class IClubComponent
{
public:
	IClubComponent();
	virtual ~IClubComponent();
	virtual void reset() {};
	virtual void init() {};
	virtual void timerSave() {};
};
