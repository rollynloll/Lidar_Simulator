// Fill out your copyright notice in the Description page of Project Settings.

#include "Velodyne/VelodyneLidarActor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Misc/Paths.h"
#include "HAL/FileManagerGeneric.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/GameplayStatics.h"
#include <cmath>
#include "GameFramework/Actor.h"


//Replace Skeletal Mesh To Static Mesh
#include "Engine/SkeletalMesh.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Velodyne/VelodyneBaseComponent.h"
#include "EngineUtils.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"

// Save Static Mesh in Directory 
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "PackageTools.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "StaticMeshDescription.h"

// Load Static Meshes
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

// Delete Static Mesh Assets
#include "EditorAssetLibrary.h"
#include "FileHelpers.h"

// HDF5 format Files
#include "H5Cpp.h"
#include "HDFParserComponent.h"
//#include <opencv2/opencv.hpp>

// Sets default values
AVelodyneLidarActor::AVelodyneLidarActor()
{
  // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = false;

  LidarComponent = CreateDefaultSubobject<UVelodyneBaseComponent>(TEXT("VelodyneComponent"));
  this->AddOwnedComponent(LidarComponent);

  // Niagara Component 생성
  NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
  NiagaraComponent->SetupAttachment(RootComponent);

  HDFParser = CreateDefaultSubobject<UHDFParserComponent>(TEXT("HDFParser"));
  this->AddOwnedComponent(HDFParser);
}

// Called when the game starts or when spawned



void AVelodyneLidarActor::BeginPlay()
{
  Super::BeginPlay();

  PCLDIR = FString::Printf(TEXT("%sPointCloud"), FPlatformProcess::UserDir());
  //UE_LOG(LogTemp, Warning, TEXT("ThirdPartyPath = %s"), TEXT(THIRDPARTY_PATH));


  if(UdpScanComponent)
  {
    ConfigureUDPScan();
    UdpScanComponent->OpenSendSocket(UdpScanComponent->Settings.SendIP, UdpScanComponent->Settings.SendPort);
    UdpScanComponent->OpenReceiveSocket(UdpScanComponent->Settings.ReceiveIP, UdpScanComponent->Settings.SendPort);
  }

  FTimespan ThreadSleepTime =
  FTimespan::FromMilliseconds(LidarComponent->Sensor.NumberDataBlock * (LidarComponent->Sensor.NumberDataChannel / LidarComponent->Sensor.NumberLaserEmitter) * (0.000001f * FIRING_CYCLE));
  FString UniqueThreadName = "LidarThread";

  // Niagara Component 활성화
  NiagaraComponent->SetAsset(NiagaraSystem);
	NiagaraComponent->ActivateSystem();

  if(LidarMode != ELidarMode::VisMode){
    ClearDirectory(PCLDIR);
    UE_LOG(LogTemp, Warning, TEXT("Point Cloud Directory cleared"));
  }
  if(LidarMode == ELidarMode::AnimMode){
    FString SMPath = TEXT("/Game/StaticMesh");
    FPackageName::LongPackageNameToFilename(SMPath, SMPath);
    //DeleteExistingStaticMeshes();
  }
  if(LidarMode == ELidarMode::ScanMode || LidarMode == ELidarMode::LoadMode){
    std::string filename(TCHAR_TO_UTF8(*HDFDIR));
    H5::H5File file(filename.c_str(), H5F_ACC_TRUNC);
    file.close();
  }


  FrameSec = 1 / FrameRate;
  NiagaraComponent->SetIntParameter(TEXT("Lifetimes"), FrameSec);
  LastTime = FPlatformTime::Seconds();

  

  
  FTransform Transform;
  Transform.SetLocation(FVector(0.0f, 0.0f, 0.0f)); // Fixed Location
  Transform.SetRotation(FQuat(FRotator(0.0f, 0.0f, 0.0f))); // Fixed Rotation
  Transform.SetScale3D(FVector(1.0f, 1.0f, 1.0f)); // Fixed Scale

  for(int i = 0; i < MaxMeshPerFrame; i++){
    AStaticMeshActor* MeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass());
    MeshActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    MeshActor->SetActorTransform(Transform);
    MeshActorArray.Add(MeshActor);
  }
  
  
  
  if(LidarMode == ELidarMode::LoadMode){

    AsyncTask(ENamedThreads::GameThread, [this]()
    {
      double CurrentTime = FPlatformTime::Seconds();

      UE_LOG(LogTemp, Log, TEXT("check2"));
      
      while(CSVIndex < MaxMeshCount){
        FString SMPath = FString::Printf(TEXT("/Game/StaticMesh/MetaHuman_%d/"), CSVIndex);
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

        FString FileName = FPackageName::LongPackageNameToFilename(SMPath, FileName);
        if (!PlatformFile.DirectoryExists(*FileName)){
          UE_LOG(LogTemp, Log, TEXT("MetaHuman Directory does not exist: %s"), *FileName);
          break;
        }
        SpawnMeshes(SMPath);
      }

      UE_LOG(LogTemp, Log, TEXT("check3"));
      for(int i = 0; i < 5; i++){
        MeshActorArray[i] -> Destroy();
      }

      UE_LOG(LogTemp, Log, TEXT("check4"));
      UE_LOG(LogTemp, Warning, TEXT("Total Scanning %d Static Mesh Time: %fs"), CSVIndex, FPlatformTime::Seconds() - CurrentTime);
      LidarMode = ELidarMode::VisMode;
      CSVIndex = 0;
    });
  }
  
  ActorTag = TEXT("SM_MetaHuman");
  UE_LOG(LogTemp, Log, TEXT("ActorTag: %s"), *ActorTag);
  UE_LOG(LogTemp, Log, TEXT("GameMode: %s"), *UEnum::GetValueAsString(LidarMode));
  LidarThread = new LidarThreadProcess(ThreadSleepTime,*UniqueThreadName, this);
  if(LidarThread)
  {
    LidarThread->Init();
    LidarThread->LidarThreadInit();
    UE_LOG(LogTemp, Warning, TEXT("Lidar thread initialized! Frame Rate: %f, Frame Sec: %f"), FrameRate, FrameSec);
  }

}

void AVelodyneLidarActor::EndPlay(EEndPlayReason::Type Reason)
{

  AnalyzeFPS(FPSArray);
  UE_LOG(LogTemp, Warning, TEXT("EndPlay"));
  UE_LOG(LogTemp, Log, TEXT("Sensor.AzimuthResolution : %f"), LidarComponent->Sensor.AzimuthResolution);

  FString SMPath = TEXT("/Game/StaticMesh");
  SMPath = FPackageName::LongPackageNameToFilename(SMPath, SMPath);

  if(LidarMode == ELidarMode::AnimMode){

    double CurrentTime = FPlatformTime::Seconds();
    if (!FPaths::DirectoryExists(SMPath)){
      LidarThread->SetPaused(true);
      StaticMeshProcess();
      LidarThread->SetPaused(false);
    }
    else
      UE_LOG(LogTemp, Error, TEXT("StaticMesh Does not deleted"));

    UE_LOG(LogTemp, Warning, TEXT("Total Bake %d Static Mesh Time: %fs"), MaxMeshCount, FPlatformTime::Seconds() - CurrentTime);
  }




  //MeshActor->Destroy();
  
  if(LidarThread)
  {
    LidarThread->LidarThreadShutdown();
    LidarThread->Stop();
  }

  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //! Wait here until WorkerThread is verified as having stopped!
  //!
  //! This is a safety feature, that will delay PIE EndPlay or
  //! closing of the game while complex calcs occurring on the
  //! thread have a chance to finish
  //!
  while(!LidarThread->ThreadHasStopped())
  {
    FPlatformProcess::Sleep(0.1);
  }
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  //Do this last
  delete LidarThread;

  Super::EndPlay(Reason);
}

// ! On Thread (not game thread)
// Never stop until finished calculating!
// This would be a verrrrry large hitch if done on game thread!
void AVelodyneLidarActor::LidarThreadTick(){

  if(LidarThread && LidarThread->IsThreadPaused())
  {
    return;
  }

  if(bThreadTickPause) return;

  //NiagaraComponent->SetBoolParameter(TEXT("bKillParticles"), true);

  float deltaTime = DeltaTimeCall();
  float FPS = deltaTime > 0 ? 1 / deltaTime : 0.0f;
  if(FPS<=0) return;
  if(FPS < FrameRate){
    UE_LOG(LogTemp, Log, TEXT("Engine FPS(%f) is lower than FrameRate(%f)"),  FPS, FrameRate);
  }
  FrameCallStamp = FrameCallStamp + deltaTime;
  


  if(FrameCallStamp > FrameSec){
    if(LidarMode == ELidarMode::AnimMode){
      AsyncTask(ENamedThreads::GameThread, [this]()
      {
        RecordSkinnedMesh(GetWorld());
      });
    }
    else if(LidarMode == ELidarMode::LoadMode){

    }
    else{
      
      SpawnPointCloud();
    }

    FrameCallStamp = FrameCallStamp - FrameSec;
    if(FrameCallStamp > 1){
      UE_LOG(LogTemp, Warning, TEXT("FrameCallStamp is over 1sec"));
      FrameCallStamp = 0;
    }
    
    while(FrameCallStamp > FrameSec){
      UE_LOG(LogTemp, Warning, TEXT("FrameCallStamp(%f) is still over FrameSec(%f)"), FrameCallStamp, FrameSec);
      FrameCallStamp = FrameCallStamp - FrameSec;
    }
  }
  
  if(FPS > 500){
    //UE_LOG(LogTemp, Warning, TEXT("FPS is over 500"));
    FPlatformProcess::SleepNoStats(0.002);
  }
  //NiagaraComponent->SetBoolParameter(TEXT("bKillParticles"), false);
  
 
}




// Scan & Visualize Point Cloud

//    BeginPlay
void AVelodyneLidarActor::ConfigureUDPScan(){
  UdpScanComponent->Settings.SendIP    = LidarComponent->DestinationIP;
  UdpScanComponent->Settings.ReceiveIP = LidarComponent->SensorIP;
  UdpScanComponent->Settings.SendPort  = LidarComponent->ScanPort;
  UdpScanComponent->Settings.SendSocketName = FString(TEXT("ue5-scan-send"));
  UdpScanComponent->Settings.BufferSize = PACKET_HEADER_SIZE + DATA_PACKET_PAYLOAD;
}
void AVelodyneLidarActor::ClearDirectory(const FString& DirectoryPath) {
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    // 가비지 컬렉션 실행
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

    if (PlatformFile.DirectoryExists(*DirectoryPath)) {
        if (!PlatformFile.DeleteDirectoryRecursively(*DirectoryPath)) {
            UE_LOG(LogTemp, Warning, TEXT("Failed to delete directory: %s"), *DirectoryPath);
        } else {
            UE_LOG(LogTemp, Log, TEXT("Successfully deleted directory: %s"), *DirectoryPath);
        }
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Directory does not exist: %s"), *DirectoryPath);
    }
}
void AVelodyneLidarActor::DeleteExistingStaticMeshes(){

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> AssetList;
    bool bRecursive = true; // 하위 디렉토리 포함
    AssetRegistryModule.Get().GetAssetsByPath(FName("/Game/StaticMesh"), AssetList, bRecursive);

    for (const FAssetData& Asset : AssetList)
    {
        if (!UEditorAssetLibrary::DeleteAsset(Asset.ObjectPath.ToString()))
            UE_LOG(LogTemp, Error, TEXT("Failed to delete: %s"), *Asset.ObjectPath.ToString());
    }

    UE_LOG(LogTemp, Log, TEXT("Delete %d Static Mesh Assets"), AssetList.Num());
}

//    LidarTick 
void AVelodyneLidarActor::SpawnPointCloud(){

  // Generate veldyne data packet
  FString FilePath = FString::Printf(TEXT("%s/LidarPointCloud_%d.csv"), *PCLDIR, CSVIndex++);
  if(LidarMode == ELidarMode::ScanMode){
    LidarComponent->GetScanData(FilePath);
    FString FrameGroupName = FString::Printf(TEXT("/frame-%06d"), CSVIndex);
    HDFParser->SavePCLToHDF5(HDFDIR, FrameGroupName, LidarComponent->PointCloudData);
  }
    
  else if(LidarMode == ELidarMode::VisMode){
    if(!LidarComponent->VisualizeData(FilePath)){
      LidarThread -> SetPaused(true);
      return;
    }
  }
  else if(LidarMode == ELidarMode::HDFMode){
    FString HDFPath = TEXT("C:/Users/MOVIN_DataPC/Desktop/PointCloud.h5");
    FString HDFDir = FString::Printf(TEXT("/frame-%06d/pointcloud"), CSVIndex);
    TArray<FVector> PCLArray;
    HDFParser->LoadPCLFromHDF5(HDFPath, HDFDir, PCLArray);
    UE_LOG(LogTemp, Log, TEXT("Frame %d of HDF // %d points generated"), CSVIndex, PCLArray.Num());

    NiagaraComponent->SetIntParameter(TEXT("SpawnCount"), PCLArray.Num());
    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector( NiagaraComponent, TEXT("PointCloudData"), PCLArray);
    return;
  }


  // Check NiagaraSystem & NiagaraComponent
  //NiagaraComponent->SetBoolParameter(TEXT("bKillParticles"), true);
  if(!NiagaraSystem || !NiagaraComponent){
    UE_LOG(LogTemp, Error, TEXT("Niagara System missing!"));
    return;
  }

  // PointCloud 데이터 전송
  NiagaraComponent->SetIntParameter(TEXT("SpawnCount"), LidarComponent->PointCloudData.Num());
  UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector( NiagaraComponent, TEXT("PointCloudData"), LidarComponent->PointCloudData);
  UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(NiagaraComponent, TEXT("PointCloudColors"), LidarComponent->PointCloudColors);
  //NiagaraComponent->SetBoolParameter(TEXT("bKillParticles"), false);

}
double AVelodyneLidarActor::DeltaTimeCall(){
  double CurrentTime = FPlatformTime::Seconds();
  double deltaTime = CurrentTime - LastTime;
  float CurrentFPS = (deltaTime > 0) ? (1.0 / deltaTime) : 0.0;
  //UE_LOG(LogTemp,Log,TEXT("Dynamic FPS LidarThreadTick: %f"), CurrentFPS);
  if(CurrentFPS > 1 && CurrentFPS < 1000) FPSArray.Add(CurrentFPS);
  LastTime = CurrentTime;

  return deltaTime;

}
//    EndPlay
void AVelodyneLidarActor::AnalyzeFPS(const TArray<float>& Data){
  if(Data.Num() == 0){
    UE_LOG(LogTemp, Warning, TEXT("Data is Empty!"));
    return;
  }

  float mean = 0;
  float std = 0;
  float min = Data[0];
  float max = Data[0];
  for(float value : Data){
    if(value > max) max = value;
    if(value < min) min = value;
    mean = mean + value;
    std = std + value*value;
  }

  mean = mean / Data.Num();
  std = FMath::Sqrt(std / Data.Num() - mean*mean);

  UE_LOG(LogTemp, Log, TEXT("mean: %f / std: %f / minValue: %f / maxValue: %f"), mean, std, min, max);
}



// Static Mesh Process

//   Scan Verticle

void AVelodyneLidarActor::RecordSkinnedMesh(UWorld* World){
  if(!World){
    UE_LOG(LogTemp, Error, TEXT("Invalid World!"));
    return;
  }

  AActor* TargetActor = nullptr;
  for (TActorIterator<AActor> It(World); It; ++It)
  {
      AActor* Actor = *It;
      if (Actor && Actor->Tags.Contains(ActorTag)) // 태그 비교
      {
          TargetActor = Actor;
          break;
      }
  }

  if(!TargetActor){
    UE_LOG(LogTemp, Error, TEXT("Actor with Tag[%s] does not exist!"), *ActorTag);
    return;
  }

  TArray<USkeletalMeshComponent* > SkeletalMeshes;
  TargetActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshes);

  if (SkeletalMeshes.Num() == 0)
  {
      UE_LOG(LogTemp, Warning, TEXT("No SkeletalMeshComponent found on actor: %s"), *TargetActor->GetName());
      return;
  }

  TArray<FSkinnedMeshFrameData> FrameDataArray;
  for(USkeletalMeshComponent* SkeletalMeshComp : SkeletalMeshes){
    FSkinnedMeshFrameData FrameData;
    CacheSkeletalMeshFrame(SkeletalMeshComp, FrameData);
    FrameDataArray.Add(FrameData);
  }
  
  CachedFrames.Add(FrameDataArray);

  /*
  TArray<UGroomComponent* > GroomComps;
  TargetActor->GetComponents<UGroomComponent>(GroomComps);

  if (GroomComps.Num() == 0)
  {
      UE_LOG(LogTemp, Warning, TEXT("No GroomComponents found on actor: %s"), *TargetActor->GetName());
      return;
  }

  TArray<UProceduralMeshComponent* > ProcMeshes;
  for(UGroomComponent* GroomComp : GroomComps){
    UProceduralMeshComponent* ProceduralMesh = ConvertToProceduralMesh(GroomComp);
    ProcMeshes.Add(ProceduralMesh);
  }

  CachedGrooms.Add(ProcMeshes);
  */
  
}
void AVelodyneLidarActor::CacheSkeletalMeshFrame(USkeletalMeshComponent* SkeletalMeshComp, FSkinnedMeshFrameData& FrameData){

    // Get the SkeletalMesh and RenderData
    USkeletalMesh* SkeletalMesh = SkeletalMeshComp->GetSkeletalMeshAsset();
    if (!SkeletalMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid SkeletalMesh."));
        return;
    }

    FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetResourceForRendering();
    if (!RenderData || RenderData->LODRenderData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No LOD data found for SkeletalMesh."));
        return;
    }

    const FSkeletalMeshLODRenderData& LODRenderData = RenderData->LODRenderData[0];
    const FPositionVertexBuffer& VertexBuffer = LODRenderData.StaticVertexBuffers.PositionVertexBuffer;
    const FRawStaticIndexBuffer16or32Interface* IndexBuffer = LODRenderData.MultiSizeIndexContainer.GetIndexBuffer();
    FSkinWeightVertexBuffer& SkinWeightBuffer = const_cast<FSkinWeightVertexBuffer&>(LODRenderData.SkinWeightVertexBuffer);
    TArray<FMatrix44f> CachedMatrices;
    SkeletalMeshComp->CacheRefToLocalMatrices(CachedMatrices);

    if (!IndexBuffer)
    {
        UE_LOG(LogTemp, Error, TEXT("IndexBuffer is null."));
        return;
    }

    // Cache Vertex Positions
    for (uint32 VertexIndex = 0; VertexIndex < LODRenderData.GetNumVertices(); ++VertexIndex)
    {
        FVector3f SkinnedPosition = SkeletalMeshComp->GetSkinnedVertexPosition(SkeletalMeshComp, VertexIndex, LODRenderData, SkinWeightBuffer, CachedMatrices);
        FrameData.VertexPositions.Add((FVector)SkinnedPosition);
    }

    // Cache Indices
    for (int32 Index = 0; Index < IndexBuffer->Num(); ++Index)
    {
        FrameData.Indices.Add(IndexBuffer->Get(Index));
    }

    FrameData.Transform = SkeletalMeshComp->GetComponentTransform();
}

//   Bake StaticMesh
void AVelodyneLidarActor::StaticMeshProcess(){
  //if(SkinnedMeshRecords.Num() == 0) return;

  int32 NumOfMeshes = CachedFrames.Num() > MaxMeshCount ? MaxMeshCount : CachedFrames.Num();
  UE_LOG(LogTemp, Log, TEXT("Number of Meshes: %d"), NumOfMeshes);

  for(int i = 0; i < NumOfMeshes; i++){
    
    TArray<FSkinnedMeshFrameData> FrameDataArray = CachedFrames[i];
    FString FilePath = FString::Printf(TEXT("/Game/StaticMesh/MetaHuman_%d"), CSVIndex++);

    for(int j = 0; j < FrameDataArray.Num(); j++){
      FString MeshName = FString::Printf(TEXT("StaticMesh%d"), j);
      UStaticMesh* StaticMesh = CreateStaticMeshFromSkinnedFrameData(FrameDataArray[j], MeshName);
      PackageStaticMesh(StaticMesh, FilePath, MeshName);
    }
    

  }
}
UStaticMesh* AVelodyneLidarActor::CreateStaticMeshFromSkinnedFrameData(const FSkinnedMeshFrameData& FrameData, const FString& MeshName){
    if (FrameData.VertexPositions.Num() == 0 || FrameData.Indices.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("FrameData is empty. Cannot create Static Mesh."));
        return nullptr;
    }

    // Create a new StaticMesh
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(GetTransientPackage(), FName(*MeshName));
    if (!StaticMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create StaticMesh."));
        return nullptr;
    }

    // Create a new MeshDescription
    FMeshDescription MeshDescription;
    FStaticMeshAttributes Attributes(MeshDescription);
    Attributes.Register();

    // Create a polygon group
    FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();

    // Create vertices
    TMap<int32, FVertexInstanceID> VertexInstanceMap;
    for (int32 VertexIndex = 0; VertexIndex < FrameData.VertexPositions.Num(); ++VertexIndex)
    {
        // Transform the vertex position using FrameData.Transform
        FVector TransformedPosition = FrameData.Transform.TransformPosition(FrameData.VertexPositions[VertexIndex]);

        FVertexID VertexID = MeshDescription.CreateVertex();
        Attributes.GetVertexPositions()[VertexID] = FVector3f(TransformedPosition);

        FVertexInstanceID VertexInstanceID = MeshDescription.CreateVertexInstance(VertexID);
        VertexInstanceMap.Add(VertexIndex, VertexInstanceID);
    }

    // Create polygons from indices
    for (int32 Index = 0; Index < FrameData.Indices.Num(); Index += 3)
    {
        FVertexInstanceID Vertex0 = VertexInstanceMap[FrameData.Indices[Index]];
        FVertexInstanceID Vertex1 = VertexInstanceMap[FrameData.Indices[Index + 1]];
        FVertexInstanceID Vertex2 = VertexInstanceMap[FrameData.Indices[Index + 2]];

        MeshDescription.CreatePolygon(PolygonGroupID, { Vertex0, Vertex1, Vertex2 });
    }

    // Build the StaticMesh from MeshDescription
    StaticMesh->BuildFromMeshDescriptions({ &MeshDescription });
    StaticMesh->InitResources();

    UE_LOG(LogTemp, Log, TEXT("StaticMesh created successfully: %s"), *MeshName);

    return StaticMesh;
}
bool AVelodyneLidarActor::PackageStaticMesh(UStaticMesh* StaticMesh, const FString& PackagePath, const FString& MeshName) {

    UE_LOG(LogTemp, Log, TEXT("Start to SAVE_SM:%s"), *MeshName);
    if (!StaticMesh) {
        UE_LOG(LogTemp, Error, TEXT("Invalid StaticMesh provided."));
        return false;
    }

    // 패키지 생성
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package) {
        UE_LOG(LogTemp, Error, TEXT("Failed to create package at path: %s"), *PackagePath);
        return false;
    }

    // Static Mesh 이름 변경 및 패키지에 추가
    StaticMesh->Rename(*MeshName, Package);
    StaticMesh->SetFlags(RF_Public | RF_Standalone);
    Package->FullyLoad();

    // 패키지 저장 경로 설정
    FString FileName = FPackageName::LongPackageNameToFilename(PackagePath, FileName);
    FileName = FString::Printf(TEXT("%s/%s.uasset"), *FileName, *MeshName);
    UE_LOG(LogTemp, Log, TEXT("절대경로: %s"), *FileName);
    

    // 패키지 저장
    bool bSaved = UPackage::SavePackage(
        Package,
        StaticMesh,
        EObjectFlags::RF_Public | EObjectFlags::RF_Standalone,
        *FileName,
        GError,
        nullptr,
        true,
        true,
        SAVE_NoError
    );

    if (!bSaved) {
        UE_LOG(LogTemp, Error, TEXT("Failed to save package: %s"), *FileName);
        return false;
    }

    // 에셋 레지스트리 업데이트
    FAssetRegistryModule::AssetCreated(StaticMesh);

    UE_LOG(LogTemp, Log, TEXT("StaticMesh saved successfully at: %s"), *FileName);
    return true;
}

//   PCL Generate
void AVelodyneLidarActor::SpawnMeshes(const FString& DirectoryPath){
    // Get Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDatas;

    for(int Index = 0; Index < MaxMeshPerFrame; Index++){
      FString MeshPath = FString::Printf(TEXT("%sStaticMesh%d"), *DirectoryPath, Index);
      FName MeshFName(*MeshPath);
      AssetRegistryModule.Get().GetAssetsByPackageName(MeshFName, AssetDatas);
      if(AssetDatas.Num() == 0){
        break;
      }

      
      UStaticMesh* StaticMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *MeshPath));
      if(!StaticMesh){
        UE_LOG(LogTemp, Error, TEXT("Static mesh does not exist!"));
        return;
      }

      if(!MeshActorArray[Index]){
        UE_LOG(LogTemp, Error, TEXT("MeshActor does not exist"));
        return;
      }
      MeshActorArray[Index]->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
    }

    
    FString FilePath = FString::Printf(TEXT("%s/LidarPointCloud_%d.csv"), *PCLDIR, CSVIndex++);
    LidarComponent->GetScanData(FilePath);
    FString framedir = FString::Printf(TEXT("/frame-%06d"), CSVIndex);
    HDFParser->SavePCLToHDF5(HDFDIR, framedir, LidarComponent->PointCloudData);


}

