// Fill out your copyright notice in the Description page of Project Settings.


#include "FTCPThread.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "IPAddress.h"

FTCPThread::FTCPThread()
{
	ServerSocket = nullptr;
	Thread = nullptr;
	bisRunThread = false;
	bisThreadRunning = false;
}

FTCPThread::~FTCPThread()
{
	StopThread();
}

bool FTCPThread::Init()
{
	bisRunThread = true;

	// 소켓 연결은 ATCPConnector::ConnectServer()가 이미 했고 SetSocket()으로 넘겨받는다.
	// 여기서 또 새로 Connect()하면 서버에 같은 액터가 두 개의 커넥션을 여는 꼴이 된다.
	return ServerSocket != nullptr;
}

uint32 FTCPThread::Run()
{
	bisThreadRunning = true;

	while (bisRunThread)
	{
		if (!ServerSocket || ServerSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected) break;

		// HasPendingData()는 uint32&, Recv()는 int32&를 받는다 — 서로 다른 타입이라
		// 변수를 하나로 겹쳐 쓰면 안 되고 각각 맞는 타입으로 따로 선언해야 한다.
		uint32 PendingSize = 0;
		int32 RecvByte = 0;
		uint8 RecvBuf[513]; // 마지막 1바이트는 널 종료용으로 남겨둔다

		bool bReceived = ServerSocket->HasPendingData(PendingSize) &&
			ServerSocket->Recv(RecvBuf, sizeof(RecvBuf) - 1, RecvByte);

		if (bReceived && RecvByte > 0)
		{
			// 실제로 받은 RecvByte 바이트 뒤에 널을 찍어서, 받지도 않은
			// 나머지 버퍼 내용이 문자열에 섞여 들어가지 않게 한다.
			RecvBuf[RecvByte] = '\0';

			FScopeLock Lock(&RecvTextLock);
			RecvText = FString(UTF8_TO_TCHAR(RecvBuf));
		}

		FPlatformProcess::Sleep(0.1f);
	}

	bisThreadRunning = false;
	return 0;
}

void FTCPThread::Stop()
{
	bisRunThread = false;
}

bool FTCPThread::IsThreadRunning() const
{
	return bisThreadRunning;
}

void FTCPThread::StartThread()
{
	if (Thread) return;
	Thread = FRunnableThread::Create(this, TEXT("FTCPThread"));
}

void FTCPThread::StopThread()
{
	Stop();
	if (Thread)
	{
		Thread->WaitForCompletion();
		Thread->Kill();
		delete Thread;
		Thread = nullptr;
	}
}

FString FTCPThread::GetAndClearRecvText()
{
	FScopeLock Lock(&RecvTextLock);
	FString Result = RecvText;
	RecvText.Empty();
	return Result;
}
