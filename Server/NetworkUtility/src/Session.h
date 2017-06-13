#pragma once
#include <memory>
#include "asio.hpp"
#include <deque> 
#include "InternalBuffer.h"
#include <functional>
#include <iostream>
#include <mutex>
using asio::ip::tcp;  
class CSession
	: public std::enable_shared_from_this<CSession>
{
public:
	typedef std::shared_ptr<CInternalBuffer> InternalBuffer_ptr;
	typedef std::deque<InternalBuffer_ptr> BufferQueue;
public:
	CSession( std::weak_ptr<asio::io_service::strand> ptrStrand );
	bool sendData(const char* pData , size_t nLen );
	uint32_t getConnectID();
	tcp::socket& socket(){return m_socket;}
	void start();
	void closeSession();  // only invoke by network ;
	// handle function ;
	void doReadHeader() ;
	void doReadBody() ;
	void doWriteBuffer() ;

	void handleError(const asio::error_code& error)
	{
		//closeSession();
		std::cout << error.message() << std::endl;
		if ( m_lpfErrorCallBack )
		{
			m_lpfErrorCallBack(m_nConnectID);
		}
	}

	void setErrorCallBack(std::function<void(uint32_t nConnectID)> lpf)
	{
		m_lpfErrorCallBack = lpf;
	}

	void setRecieveCallBack(std::function< void(uint32_t nConnectID, const char* pBuffer, size_t nBufferLen)> lpf )
	{
		m_plfReciveCallBack = lpf;
	}
protected:
	static uint32_t s_ConnectID ;
protected:
	tcp::socket m_socket; 
	uint32_t m_nConnectID ;
	 
	std::weak_ptr<asio::io_service::strand> m_ptrStrand;

	BufferQueue m_vWillSendBuffers ;

	InternalBuffer_ptr m_pReadIngBuffer ;

	std::function<void(uint32_t nConnectID)> m_lpfErrorCallBack;
	std::function< void(uint32_t nConnectID, const char* pBuffer, uint32_t nBufferLen)> m_plfReciveCallBack;
};