// Fill out your copyright notice in the Description page of Project Settings.


#include "TCPChatLogActor.h"

// Sets default values
ATCPChatLogActor::ATCPChatLogActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATCPChatLogActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("ATCPChatLogActor BeginPlay() called."));
	
}

// Called every frame
void ATCPChatLogActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

