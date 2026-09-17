// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

/**
 * 
 */
class TCPWITHMYSQL_API FTCPThread : public FRunnable
{
public:
	FTCPThread();
	~FTCPThread() override;

	bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

	bool IsThreadRunning() const;

	// 액터가 이미 Connect()해 둔 소켓을 그대로 넘겨받는다.
	// (이 스레드가 별도로 127.0.0.1:17325에 다시 접속하지 않도록.)
	void SetSocket(class FSocket* InSocket) { ServerSocket = InSocket; }

	void StartThread();
	void StopThread();

	// 게임 스레드에서 안전하게 최신 수신 텍스트를 꺼내가기 위한 함수.
	// (RecvText를 양쪽 스레드가 락 없이 직접 주고받지 않도록.)
	FString GetAndClearRecvText();

	class FSocket* ServerSocket;

protected:

	FRunnableThread* Thread;
	bool bisRunThread;
	bool bisThreadRunning;

	FCriticalSection RecvTextLock;
	FString RecvText = "";
};
