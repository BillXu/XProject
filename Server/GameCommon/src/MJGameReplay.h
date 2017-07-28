#pragma once
#include <list>
#include <memory>
#include "json\json.h"
#include "log4z.h"
#include "AsyncRequestQuene.h"
class MJReplayFrame
{
public:
	bool init( uint16_t nFrameType,Json::Value& jsFramArg )
	{
		m_nFrameType = nFrameType;
		return true;
	}

	void setFrameArg(Json::Value& jsArg)
	{
		m_jsFrameArg = jsArg;
	}

	void toJson( Json::Value& jsFrame)
	{
		jsFrame["T"] = m_nFrameType;
		jsFrame["A"] = m_jsFrameArg;
	}

protected:
	uint16_t m_nFrameType;
	Json::Value m_jsFrameArg;
};

class MJReplayGame
{
public:
	void init( uint32_t nRoomType ,Json::Value& jsRoomOpts )
	{
		setReplayRoomInfo(jsRoomOpts);
	}
	
	void startNewReplay( uint32_t nReplayID )
	{
		setReplayID(nReplayID);
		clearFrames();
	}

	void clearFrames() 
	{
		m_vAllFrames.clear();
	}

	uint32_t getReplayID() 
	{
		return m_nReplayID;
	}

	void toJson(Json::Value& jsReplay) 
	{
		if ( m_vAllFrames.empty())
		{
			LOGFMTE("no frames , so we needn't save");
			return;
		}
		jsReplay["info"] = m_jsGameInfo;
		jsReplay["id"] = m_nReplayID;
		Json::Value jsFrames;
		for (auto& ref : m_vAllFrames)
		{
			ref->toJson(jsFrames[jsFrames.size()]);
		}
		jsReplay["frames"] = jsFrames;
	}

	bool addFrame(uint16_t nFrameType, Json::Value& jsFrameArg ) 
	{
		auto ptr = std::make_shared<MJReplayFrame>();
		ptr->init(nFrameType, jsFrameArg);
		m_vAllFrames.push_back(ptr);
		return true;
	}
	bool doSaveReplayToDB( CAsyncRequestQuene* pSyncQuene )
	{
		Json::Value jsReplay;
		toJson(jsReplay);
		if ( jsReplay.isNull() )
		{
			LOGFMTD("replay frame is null so do not save db ");
			return false;
		}

		Json::StyledWriter jss;
		auto strJs = jss.write(jsReplay);
		LOGFMTI("replay len = %u", strJs.size());

		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, sizeof(pBuffer), "insert into gamereplay (replayID,roomType,time,detail ) values (%u,%u,now(),", getReplayID(), m_nRoomType );
		std::ostringstream ss;
		ss << pBuffer << strJs << " ) ;";
		jssql["sql"] = ss.str();
		pSyncQuene->pushAsyncRequest(ID_MSG_PORT_RECORDER_DB, getReplayID(), eAsync_DB_Add, jssql);
		return true;
	}
protected:
	void setReplayID(uint32_t nReplayID)
	{
		m_nReplayID = nReplayID;
	}
	void setReplayRoomInfo(Json::Value& jsInfo)
	{
		m_jsGameInfo = jsInfo;
	}
protected:
	Json::Value m_jsGameInfo;
	uint32_t m_nReplayID;
	uint32_t m_nRoomType;
	std::list<std::shared_ptr<MJReplayFrame>> m_vAllFrames;
};





