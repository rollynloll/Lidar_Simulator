// Fill out your copyright notice in the Description page of Project Settings.

#include "Velodyne/VelodyneBaseComponent.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Physics/PhysicsInterfaceCore.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"


#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/PositionVertexBuffer.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "StaticMeshOperations.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#include "Async/ParallelFor.h"
#include "Misc/ScopeLock.h"


// Sets default values for this component's properties
UVelodyneBaseComponent::UVelodyneBaseComponent()
{
  // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
  // off to improve performance if you don't need them.
  PrimaryComponentTick.bCanEverTick = false;

  // Check Platform supports multithreading
  SupportMultithread = FPlatformProcess::SupportsMultithreading();

  // Set-up initial values
  SensorModel          = EModelName::VLP16;
  SamplingRate         = EFrequency::SR10;
  ReturnMode           = ELaserReturnMode::Strongest;
  SensorIP             = FString(TEXT("127.0.0.1"));
  DestinationIP        = FString(TEXT("0.0.0.0"));
  ScanPort             = 2368;
  PositionPort         = 8308;
}


// Called when the game starts
void UVelodyneBaseComponent::BeginPlay()
{
  Super::BeginPlay();

  ConfigureVelodyneSensor();

  
}


// Called every frame
void UVelodyneBaseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVelodyneBaseComponent::EndPlay(EEndPlayReason::Type Reason)
{
  Super::EndPlay(Reason);
}

void UVelodyneBaseComponent::ConfigureVelodyneSensor()


{
  switch (SensorModel.GetValue()) {
  case 0:{ // VLP-16
    float Elevation[] = {-15.f, 1.f, -13.f, 3.f, -11.f, 5.f, -9.f, 7.f,
                        -7.f, 9.f, -5.f, 11.f, -3.f, 13.f, -1.f, 15.f};
    Sensor.ElevationAngle.Append(Elevation, UE_ARRAY_COUNT(Elevation));
    Sensor.NumberLaserEmitter = 128;
    Sensor.NumberDataBlock = 12;
    Sensor.NumberDataChannel = 32;
    Sensor.ModelNumber = 34;
    Sensor.MinRange = 20.f;
    Sensor.MaxRange = 10000.f;
    break;
    }
  case 1:{ // PUCK-Lite
    float Elevation[] = {-15.f, 1.f, -13.f, 3.f, -11.f, 5.f, -9.f, 7.f,
                        -7.f, 9.f, -5.f, 11.f, -3.f, 13.f, -1.f, 15.f};
    Sensor.ElevationAngle.Append(Elevation, UE_ARRAY_COUNT(Elevation));
    Sensor.NumberLaserEmitter = 16;
    Sensor.NumberDataBlock = 12;
    Sensor.NumberDataChannel = 32;
    Sensor.ModelNumber = 34;
    Sensor.MinRange = 20.f;
    Sensor.MaxRange = 10000.f;
    break;
    }
  case 2:{ // PUCK-HiRes
    float Elevation[] = {-10.f, 0.67f, -8.67f, 2.f, -7.33f, 3.33f, -6.f, 4.67f,
                        -4.67f, 6.f, -3.33f, 7.33f, -2.f, 8.67f, -0.67f, 10.f};
    Sensor.ElevationAngle.Append(Elevation, UE_ARRAY_COUNT(Elevation));
    Sensor.NumberLaserEmitter = 16;
    Sensor.NumberDataBlock = 12;
    Sensor.NumberDataChannel = 32;
    Sensor.ModelNumber = 36;
    Sensor.MinRange = 20.f;
    Sensor.MaxRange = 10000.f;
    break;
    }
  case 3:{ // VLP-32C
    float Elevation[] = {-25.f, -1.f, -1.667f, -15.639f, -11.31f, 0.f, -0.667f, -8.843f,
                        -7.254f, 0.333f, -0.333f, -6.148f, -5.333f, 1.333f, 0.667f, -4.f,
                        -4.667f, 1.667f, 1.f, -3.667f, -3.333f, 3.333f, 2.333f, -2.667f,
                        -3.f, 7.f, 4.667f, -2.333f, -2.f, 15.f, 10.333f, -1.333f};
    float AzimuthOffset[] = {1.4f, -4.2f, 1.4f, -1.4f, 1.4f, -1.4f, 4.2f, -1.4f,
                            1.4f, -4.2f, 1.4f, -1.4f, 4.2f, -1.4f, 4.2f, -1.4f,
                            1.4f, -4.2f, 1.4f, -4.2f, 4.2f, -1.4f, 1.4f, -1.4f,
                            1.4f, -1.4f, 1.4f, -4.2f, 4.2f, -1.4f, 1.4f, -1.4f};
    Sensor.ElevationAngle.Append(Elevation, UE_ARRAY_COUNT(Elevation));
    Sensor.AzimuthOffset.Append(AzimuthOffset, UE_ARRAY_COUNT(AzimuthOffset));
    Sensor.NumberLaserEmitter = 32;
    Sensor.NumberDataBlock = 12;
    Sensor.NumberDataChannel = 32;
    Sensor.ModelNumber = 40;
    Sensor.MinRange = 50.f;
    Sensor.MaxRange = 10000.f;
    break;
    }
  case 4:{ // VELARRAY : Not implemented
    float Elevation[] = {15.f, -1.f, 13.f, -3.f, 11.f, -5.f, 9.f, -7.f,
                        7.f, -9.f, 5.f, -11.f, 3.f, -13.f, 1.f, -15.f};
    Sensor.ElevationAngle.Append(Elevation, UE_ARRAY_COUNT(Elevation));
    Sensor.NumberLaserEmitter = 32;
    Sensor.NumberDataBlock = 12;
    Sensor.NumberDataChannel = 32;
    Sensor.ModelNumber = 49;
    Sensor.MinRange = 20.f;
    Sensor.MaxRange = 8000.f;
    break;
    }
  case 5:{ // VLS_128 : Not implemented
    float Elevation[] = {15.f, -1.f, 13.f, -3.f, 11.f, -5.f, 9.f, -7.f,
                        7.f, -9.f, 5.f, -11.f, 3.f, -13.f, 1.f, -15.f};
    Sensor.ElevationAngle.Append(Elevation, UE_ARRAY_COUNT(Elevation));
    Sensor.NumberLaserEmitter = 128;
    Sensor.NumberDataBlock = 12;
    Sensor.NumberDataChannel = 32;
    Sensor.ModelNumber = 161;
    Sensor.MinRange = 20.f;
    Sensor.MaxRange = 10000.f;
    break;
    }
  case 6:{ // HDL-32 : Not implemented
    float Elevation[] = {15.f, -1.f, 13.f, -3.f, 11.f, -5.f, 9.f, -7.f,
                        7.f, -9.f, 5.f, -11.f, 3.f, -13.f, 1.f, -15.f};
    Sensor.ElevationAngle.Append(Elevation, UE_ARRAY_COUNT(Elevation));
    Sensor.NumberLaserEmitter = 32;
    Sensor.NumberDataBlock = 12;
    Sensor.NumberDataChannel = 32;
    Sensor.ModelNumber = 33;
    Sensor.MinRange = 20.f;
    Sensor.MaxRange = 10000.f;
    break;
    }
  }

  switch (SamplingRate.GetValue()) {
  case 0:
    Sensor.SamplingRate = 5;
    break;
  case 1:
    Sensor.SamplingRate = 10;
    break;
  case 2:
    Sensor.SamplingRate = 15;
    break;
  case 3:
    Sensor.SamplingRate = 20;
    break;
  default:
    Sensor.SamplingRate = 10;
    break;
  }

  switch (ReturnMode.GetValue()) {
  case 0:
    Sensor.ReturnMode = 55;
    break;
  case 1: // Last Return : Not implemented
    Sensor.ReturnMode = 56;
    break;
  case 2: // Dual Return : Not implemented
    Sensor.ReturnMode = 57;
    break;
  }

  Sensor.AzimuthResolution = 0.35;

  // Initialize raycast vector and azimuth vector
  Sensor.AzimuthAngle.Init(90.f, Sensor.NumberDataBlock * Sensor.NumberDataChannel);

  // Initialize packet vector
  Sensor.DataPacket.AddUninitialized(DATA_PACKET_PAYLOAD);
  Sensor.PositionPacket.AddUninitialized(POSITION_PACKET_PAYLOAD);
}

// The VLP-16 measures reflectivity of an object independent of laser power and distances involved. Reflectivity values
// returned are based on laser calibration against NIST-calibrated reflectivity reference targets at the factory.
//
// For each laser measurement, a reflectivity byte is returned in addition to distance. Reflectivity byte values are segmented
// into two ranges, allowing software to distinguish diffuse reflectors (e.g. tree trunks, clothing) in the low range from
// retroreflectors (e.g. road signs, license plates) in the high range.
//
// A retroreflector reflects light back to its source with a minimum of scattering. The VLP-16 provides its own light, with negligible
// separation between transmitting laser and receiving detector, so retroreflecting surfaces pop with reflected IR light
// compared to diffuse reflectors that tend to scatter reflected energy.
//
//   - Diffuse reflectors report values from 0 to 100 for reflectivities from 0% to 100%.
//   - Retroreflectors report values from 101 to 255, where 255 represents an idealreflection.
//
// Note: When a laser pulse doesn't result in a measurement, such as when a laser is shot skyward, both distance and
//        reflectivity values will be 0. The key is a distance of 0, because 0 is a valid reflectivity value (i.e. one step above noise).
uint8 UVelodyneBaseComponent::GetIntensity(const FString Surface, const float Distance) const
{
  uint8 MaxReflectivity = 0;
  uint8 MinReflectivity = 0;

  if (Surface.Contains(TEXT("PM_Reflectivity_"), ESearchCase::CaseSensitive)) {
    // https://docs.unrealengine.com/5.0/en-US/API/Runtime/Core/Containers/FString/RightChop/1/
    MaxReflectivity = (uint8)FCString::Atoi(*Surface.RightChop(16));
    if(MaxReflectivity > 100)
    {
      MinReflectivity = 101;
    }
  }
  else { // Default PhysicalMaterial value, in case of the PhysicalMaterial is not applied
    MaxReflectivity = 20;
  }

  return (uint8)((MinReflectivity-MaxReflectivity) / (Sensor.MaxRange - Sensor.MinRange) * Distance + MaxReflectivity);
}
void UVelodyneBaseComponent::GetScanData(const FString& FilePath)
{ 
  // complex collisions: true
  FCollisionQueryParams TraceParams = FCollisionQueryParams(TEXT("LaserTrace"), true, GetOwner());
  TraceParams.bReturnPhysicalMaterial = true;
  TraceParams.bTraceComplex = true;

  // Get owner's location and rotation
  FVector LidarPosition = this->GetActorLocation();
  FRotator LidarRotation = this->GetActorRotation();

  // Initialize array for raycast result
  TArray<FHitResult> Records;
  int TotalPointNum = (int32)(2*HorizontalRange / Sensor.AzimuthResolution) * Sensor.NumberLaserEmitter;
  Records.Init(FHitResult(ForceInit), TotalPointNum);

  PointCloudData = TArray<FVector>();
  PointCloudColors = TArray<FLinearColor>();

  TArray<FVector> LocalPointCloudData;
  TArray<FLinearColor> LocalPointCloudColors;

  const int ThreadNum = FPlatformMisc::NumberOfWorkerThreadsToSpawn();
  const int DivideEnd = FMath::FloorToInt((float)(TotalPointNum / ThreadNum));
  FCriticalSection CriticalSection;
  
  ParallelFor(
    ThreadNum,
    [&](int32 PFIndex)
    {
      int StartAt = PFIndex * DivideEnd;
      if (StartAt >= TotalPointNum) {
          return;
      }

      int EndAt = StartAt + DivideEnd;
      if (PFIndex == (ThreadNum - 1)) {
          EndAt = TotalPointNum;
      }
      

      for (int32 Index = StartAt; Index < EndAt; Index++) {

        //Scan Lidar Data
        const float HAngle = (float)((int32)(Index / Sensor.NumberLaserEmitter) * Sensor.AzimuthResolution) - HorizontalRange;
        int VIndex = (int32)(Index % Sensor.NumberLaserEmitter);
        float VAngle = (float)(static_cast<float>(VIndex) / Sensor.NumberLaserEmitter);
        VAngle = (2*VAngle - 1) * VerticalRange;
        FHitResult RecordPoint;

        FRotator LaserRotation(0.f, 0.f, 0.f);
        LaserRotation.Add(VAngle, HAngle, 0.f);

        FRotator Rotation = UKismetMathLibrary::ComposeRotators(LaserRotation, LidarRotation);

        FVector BeginPoint = LidarPosition + Sensor.MinRange * UKismetMathLibrary::GetForwardVector(Rotation);
        FVector EndPoint = LidarPosition + Sensor.MaxRange * UKismetMathLibrary::GetForwardVector(Rotation);

        GetWorld()->LineTraceSingleByChannel(
            RecordPoint, BeginPoint, EndPoint, ECC_Visibility, TraceParams, FCollisionResponseParams::DefaultResponseParam);

        //PointCloud Geneateion
        uint16 Distance = 0;
        uint8 IntensityData;
        auto PhysMat = RecordPoint.PhysMaterial;

        if (RecordPoint.bBlockingHit) {
          Distance = ((RecordPoint.Distance + Sensor.MinRange) * 10) / 2; // 2mm resolution
          if(PhysMat!=nullptr){
            IntensityData = GetIntensity(*PhysMat->GetName(), (Distance * 2) / 10);
          }
          else{
            UE_LOG(LogTemp, Warning, TEXT("Impact Object does not have Name"));
            IntensityData = GetIntensity(TEXT("OBJECT"), (Distance * 2) / 10);
          }
          
          FVector HitLocation = RecordPoint.ImpactPoint;
          if(HitLocation.ContainsNaN()){
            UE_LOG(LogTemp, Error, TEXT("Hitpoint contains NaN"));
          }
          
          FScopeLock Lock(&CriticalSection);
          LocalPointCloudData.Add(HitLocation);
          LocalPointCloudColors.Add(FLinearColor(IntensityData, 0.0f, 0.0f, 1.0f));
        }
      }
    },
    !SupportMultithread
  );

  PointCloudData.Append(LocalPointCloudData);
  PointCloudColors.Append(LocalPointCloudColors);

  if(bCSVGenerate){
    GenerateCSVfromPCL(FilePath);
  }
  
}
bool UVelodyneBaseComponent::VisualizeData(const FString& FilePath){

  FString FileContent;
  if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
  {
    UE_LOG(LogTemp, Warning, TEXT("No more CSV File in Directory: %s"), *FilePath);
    return false;
  }

  TArray<FString> Lines;
  FileContent.ParseIntoArrayLines(Lines);

  PointCloudData = TArray<FVector>();
  PointCloudColors = TArray<FLinearColor>();

  // 첫 번째 줄은 헤더일 가능성이 있으므로 스킵
  if (Lines.Num() > 0 && Lines[0].Contains(TEXT("X,Y,Z,Intensity")))
  {
      Lines.RemoveAt(0);
  }

  for (const FString& Line : Lines)
  {
      // 쉼표로 분리
      TArray<FString> Columns;
      Line.ParseIntoArray(Columns, TEXT(","), true);

      if (Columns.Num() != 4)
      {
          UE_LOG(LogTemp, Warning, TEXT("잘못된 형식의 데이터가 발견되었습니다: %s"), *Line);
          continue;
      }

      // X, Y, Z, Intensity 값 파싱
      float X = FCString::Atof(*Columns[0]);
      float Y = FCString::Atof(*Columns[1]);
      float Z = FCString::Atof(*Columns[2]);
      float Intensity = FCString::Atof(*Columns[3]);

      // PointCloudData에 좌표 추가
      PointCloudData.Add(FVector(X, Y, Z));

      // PointCloudColors에 색상 추가 (R = Intensity, G = 0, B = 0, A = 1)
      PointCloudColors.Add(FLinearColor(Intensity, 0.0f, 0.0f, 1.0f));
  }
  return true;

}

void UVelodyneBaseComponent::GenerateCSVfromPCL(const FString& FilePath){
  FString CSVContent;
  CSVContent.Append(TEXT("X,Y,Z,Intensity\n"));

  for(int i = 0; i< PointCloudData.Num(); i++){
    FVector HitLocation = PointCloudData[i];
    float intensity = PointCloudColors[i].R;
    CSVContent.Append(FString::Printf(TEXT("%f,%f,%f,%d\n"), HitLocation.X, HitLocation.Y, HitLocation.Z, intensity));
  }

  if (FFileHelper::SaveStringToFile(CSVContent, *FilePath))
  {
      //UE_LOG(LogTemp, Log, TEXT("PointCloud data successfully saved to: %s"), *FilePath);
  }
  else
  {
      UE_LOG(LogTemp, Error, TEXT("Failed to save PointCloud data to: %s"), *FilePath);
  }

  //UE_LOG(LogTemp, Log, TEXT("Generate Point Cloud with %d Points."), PointCloudData.Num());

}


FVector UVelodyneBaseComponent::GetActorLocation() { return GetOwner()->GetActorLocation(); }
FRotator UVelodyneBaseComponent::GetActorRotation() { return GetOwner()->GetActorRotation(); }
uint32 UVelodyneBaseComponent::GetTimestampMicroseconds() { return (uint32)(fmod(GetWorld()->GetTimeSeconds(), 3600.f) * 1000000); }