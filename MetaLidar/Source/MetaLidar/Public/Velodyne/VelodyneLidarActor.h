// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Velodyne/VelodyneBaseComponent.h"
#include "LidarBaseActor.h"
#include "ProceduralMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "VelodyneLidarActor.generated.h"

/**
 *
 */
UENUM(BlueprintType)
enum class ELidarMode : uint8
{
    ScanMode UMETA(DisplayName = "Scanning Mode"),
    VisMode  UMETA(DisplayName = "Visualize Mode"),
    AnimMode UMETA(DisplayName = "Animation Mode"),
    LoadMode UMETA(DisplayName = "LoadMesh Mode"),
    HDFMode UMETA(DisplayName = "HDF5 Visualize Mode")
};

struct FSkinnedMeshFrameData
{
    TArray<FVector> VertexPositions;
    TArray<uint32> Indices;
    FTransform Transform;
};

UCLASS()
class METALIDAR_API AVelodyneLidarActor : public ALidarBaseActor
{
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  AVelodyneLidarActor();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  class UVelodyneBaseComponent* LidarComponent;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  class UHDFParserComponent* HDFParser;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
	UNiagaraSystem* NiagaraSystem;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  double FrameRate = 60.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  int MaxMeshCount = 10;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  int MaxMeshPerFrame = 5;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  ELidarMode LidarMode = ELidarMode::ScanMode;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  FString PCLDIR = FString::Printf(TEXT("C:/Users/MOVIN_DataPC/Desktop/PointCloud")); 

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  FString SMDIR = FString::Printf(TEXT("C:/Users/MOVIN_DataPC/Documents/Unreal Projects/MyProject/Content/StaticMesh")); 

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  FString HDFDIR = FString::Printf(TEXT("C:/Users/MOVIN_DataPC/Desktop/PointCloud.h5")); 

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarActor")
  FString ActorTag = TEXT("SM_MetaHuman");

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

  // Called when the game end
  virtual void EndPlay(EEndPlayReason::Type Reason) override;

  

public:
  UNiagaraComponent* NiagaraComponent;
  FThreadSafeBool bKill;
  int CSVIndex = 0;
  double FrameCallStamp = 0.0f;
  double FrameSec = 0.05;
  double LastTime = 0.0f;
  TArray<float> FPSArray;
  TArray<TArray<FSkinnedMeshFrameData>> CachedFrames;
  TArray<TArray<UProceduralMeshComponent*>> CachedGrooms;
  bool bThreadTickPause = false;
  TArray<AStaticMeshActor*> MeshActorArray;
  /**
  * Set UDP communication parameters for scan data
  */
  virtual void ConfigureUDPScan() override;

  /**
  * Main routine
  * calculate raytracing and generate LiDAR packet data
  */
  virtual void LidarThreadTick() override;

  void ClearDirectory(const FString& FilePath);
  void SpawnPointCloud();
  double DeltaTimeCall();
  void AnalyzeFPS(const TArray<float>& Data);
  void DeleteExistingStaticMeshes();

  void StaticMeshProcess();
  void RecordSkinnedMesh(UWorld* World);
  bool PackageStaticMesh(UStaticMesh* StaticMesh, const FString& DirectoryName, const FString& AssetName);
  UStaticMesh* CreateStaticMeshFromSkinnedFrameData(const FSkinnedMeshFrameData& FrameData, const FString& MeshName);
  void CacheSkeletalMeshFrame(USkeletalMeshComponent* SkeletalMeshComp, FSkinnedMeshFrameData& FrameData);
  void SpawnMeshes(const FString& DirectoryPath);

  
};
