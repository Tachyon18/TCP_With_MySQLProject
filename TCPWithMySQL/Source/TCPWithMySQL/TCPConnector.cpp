// Fill out your copyright notice in the Description page of Project Settings.
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "TCPConnector.h"

#include "Windows/AllowWindowsPlatformTypes.h"

#include <WinSock2.h>
#include <iostream>

#include "Windows/HideWindowsPlatformTypes.h"

#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"

#include "Networking.h"
#include "SocketSubsystemModule.h"

#include "FTCPThread.h"

#pragma comment(lib, "ws2_32.lib")

// Sets default values
ATCPConnector::ATCPConnector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ClientSocket = nullptr;
	UnrealThread = nullptr;

	JsonData.Name = "";
	JsonData.Description = "";
	JsonData.Color = "";
	JsonData.Number = -1;
}

// Called when the game starts or when spawned
void ATCPConnector::BeginPlay()
{
	Super::BeginPlay();

	if (ConnectServer())
	{
		SendText();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to connect to server."));
	}
	
}

void ATCPConnector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopThread();
	delete TCPThreadInstance;
	TCPThreadInstance = nullptr;

	if (ClientSocket)
	{
		ClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ATCPConnector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool ATCPConnector::ConnectServer()
{
	ClientSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("DefaultSocket"), false);
	ClientAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	int32 Port = 17325;

	FString IP = TEXT("127.0.0.1");
	FIPv4Address TemporaryAddr;
	FIPv4Address::Parse(IP, TemporaryAddr);

	ClientAddress->SetPort(Port);
	ClientAddress->SetIp(TemporaryAddr.Value);

	return (ClientSocket->Connect(*ClientAddress));;
}

void ATCPConnector::SendText()
{
	if (Text != "")
	{
		FString Serial = "C|" + Text + "|#";
		TCHAR* SerializedText = Serial.GetCharArray().GetData();
		int32 Size = FCString::Strlen(SerializedText);
		int32 BytesSent = 0;
		ClientSocket->Send((uint8*)TCHAR_TO_UTF8(SerializedText), Size, BytesSent);

		TArray<uint8> ReceivedData;
		uint32 RecvSize = 0;

		if (ClientSocket->HasPendingData(RecvSize))
		{
			// 널 종료를 보장하려고 +1 만큼 잡고 마지막을 0으로 채운다.
			ReceivedData.Init(0, RecvSize + 1);
			int32 Read = 0;
			ClientSocket->Recv(ReceivedData.GetData(), RecvSize, Read);

			// 계산만 해두고 버리던 값을 실제로 저장한다.
			RecvText = FString(UTF8_TO_TCHAR(ReceivedData.GetData()));
		}
	}
}

void ATCPConnector::SendDataText()
{
	CreateDataText();

	if (DataText != "")
	{
		// 서버 InterpretMessage()는 "cmd|메시지|#" 형식을 기대한다.
		// 기존처럼 '#' 없이 보내면 서버 파싱 루프가 종료 문자를 못 찾고
		// 수신 버퍼 밖까지 읽어버리는 버그를 유발한다 (서버 쪽에서도 방어 처리했지만
		// 클라이언트도 애초에 프로토콜을 지켜서 보내야 한다).
		FString Serial = "Q|" + DataText + "|#";
		TCHAR* SerializedText = Serial.GetCharArray().GetData();
		int32 Size = FCString::Strlen(SerializedText);
		int32 BytesSent = 0;
		ClientSocket->Send((uint8*)TCHAR_TO_UTF8(SerializedText), Size, BytesSent);
	}
}

void ATCPConnector::SendPacket()
{
	TSharedPtr<FBufferArchive> Packet = CreatePacket(0, TEXT("start packet"));

	//AsyncTask(ENamedThreads::AnyThread, [this, Packet]()
	//	{
	//		if (ClientSocket == nullptr || this == nullptr)
	//		{
	//			return;
	//		}

			int32 NumSend;
			bool bSuccess = ClientSocket->Send(Packet->GetData(), Packet->Num(), NumSend);
//		});
}

void ATCPConnector::CreateDataText()
{
	if ((JsonData.Name == "") || (JsonData.Description == "") || (JsonData.Color == "") ||
		(JsonData.Number == -1))
	{
		return;
	}

	DataText = JsonData.Name + ',' + JsonData.Description + ',' + JsonData.Color + ',' + FString::FromInt(JsonData.Number);
}

TSharedPtr<FBufferArchive> ATCPConnector::CreatePacket(int32 InType, const uint8* InPayload, int32 InPayloadSize)
{
	FMessageHeader Header(InType, InPayloadSize);
	constexpr static int32 HeaderSize = sizeof(FMessageHeader);

	TSharedPtr<FBufferArchive> Packet = MakeShareable(new FBufferArchive());

	(*Packet) << Header;

	Packet->Append(InPayload, InPayloadSize);

	return Packet;
}

TSharedPtr<FBufferArchive> ATCPConnector::CreatePacket(int32 Type, const FString& SendText)
{
	SCOPE_CYCLE_COUNTER(STAT_Send);
	
	FTCHARToUTF8 Convert(*SendText);
	FArrayWriter WriterArray;

	WriterArray.Serialize((UTF8CHAR*)Convert.Get(), Convert.Length());

	TSharedPtr<FBufferArchive> Packet = CreatePacket(Type, WriterArray.GetData(), WriterArray.Num());

	return Packet;
}

void ATCPConnector::StartThread()
{
	if (!ClientSocket || !TCPThreadInstance) return;

	// 새로 접속하지 않고, 이 액터가 이미 연결해 둔 소켓을 그대로 넘겨서 쓴다.
	TCPThreadInstance->SetSocket(ClientSocket);
	TCPThreadInstance->StartThread();
}

void ATCPConnector::SendToThread()
{
	SendDataText();
}

void ATCPConnector::ReceiveToThread()
{
	if (!TCPThreadInstance) return;

	FString Received = TCPThreadInstance->GetAndClearRecvText();
	if (!Received.IsEmpty())
	{
		RecvText = Received;
	}
}

void ATCPConnector::StopThread()
{
	if (TCPThreadInstance)
	{
		TCPThreadInstance->StopThread();
	}
}

void ATCPConnector::Send()
{
}

bool ATCPConnector::Receive(FSocket* Socket, uint8* Results, int32 Size)
{
	int32 Offset = 0;
	while (Size > 0)
	{
		int32 NumRead = 0;
		Socket->Recv(Results + Offset, Size, NumRead);
		check(NumRead <= Size);

		if (NumRead <= 0)
		{
			return false;
		}

		Offset += NumRead;
		Size -= NumRead;
	}

	return true;
}

