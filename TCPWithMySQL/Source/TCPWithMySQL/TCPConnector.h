// Fill out your copyright notice in the Description page of Project Settings.

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma once

#include "Windows/AllowWindowsPlatformTypes.h"

#include <WinSock2.h>
#include <iostream>

#include "Windows/HideWindowsPlatformTypes.h"

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "NetworkMessage.h"
#include "FTCPThread.h"
#include "GameFramework/Actor.h"
#include "TCPConnector.generated.h"

#pragma comment(lib, "ws2_32.lib")

DECLARE_STATS_GROUP(TEXT("TCPSocketActor"), STATGROUP_TCPSocketActor, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Send"), STAT_Send, STATGROUP_TCPSocketActor);
DECLARE_CYCLE_STAT(TEXT("Recv"), STAT_Recv, STATGROUP_TCPSocketActor);

USTRUCT(BlueprintType)
struct FJsonBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JsonData")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JsonData")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JsonData")
	FString Color;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JsonData")
	int32 Number;
};

struct TCPWITHMYSQL_API FMessageHeader
{
	int32 Type;
	int32 Size;

	FMessageHeader() : Type(0), Size(0)
	{

	}

	FMessageHeader(int32 Type, int32 PayloadSize) :
		Type(Type), Size(PayloadSize)
	{

	}

	friend FArchive& operator<<(FArchive& Archive, FMessageHeader& Header)
	{
		Archive << Header.Type;
		Archive << Header.Size;

		return Archive;
	}
};

UCLASS()
class TCPWITHMYSQL_API ATCPConnector : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATCPConnector();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	bool ConnectServer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCP")
	FString Text = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP")
	FJsonBase JsonData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP")
	FString DataText = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP")
	FString RecvText = "";

	TSharedPtr<FInternetAddr> ClientAddress;
	class FSocket* ClientSocket;

	//SOCKET Socket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP")
	FString ID = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP")
	FString CheckText = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP")
	bool bisLogin = false;

	class FTCPThread* TCPThreadInstance = new FTCPThread();

	class FRunnableThread* UnrealThread;

	int SendByte = 0;
	int32 RecvByte = 0;

	UFUNCTION(BlueprintCallable)
	void SendText();

	UFUNCTION(BlueprintCallable)
	void SendDataText();

	UFUNCTION(BlueprintCallable)
	void SendPacket();
	
	UFUNCTION(BlueprintCallable)
	void CreateDataText();

	static TSharedPtr<FBufferArchive> CreatePacket(int32 InType, const uint8* InPayload, int32 InPayloadSize);

	TSharedPtr<FBufferArchive> CreatePacket(int32 Type, const FString& SendText);

	UFUNCTION(BlueprintCallable)
	void StartThread();

	UFUNCTION(BlueprintCallable)
	void SendToThread();

	UFUNCTION(BlueprintCallable)
	void ReceiveToThread();

	UFUNCTION(BlueprintCallable)
	void StopThread();


public:
	UFUNCTION(BlueprintCallable)
	void Send();

	bool Receive(FSocket* Socket, uint8* Results, int32 Size);
};
