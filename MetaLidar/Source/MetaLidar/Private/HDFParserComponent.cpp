// Fill out your copyright notice in the Description page of Project Settings.


#include "HDFParserComponent.h"
#include "H5Cpp.h"

// Sets default values for this component's properties
UHDFParserComponent::UHDFParserComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UHDFParserComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UHDFParserComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UHDFParserComponent::LoadPCLFromHDF5(const FString& FilePath, const FString& HDFDir, TArray<FVector>& OutPointCloud)
{

    UE_LOG(LogTemp, Log, TEXT("log1"));
    std::string StdFilePath(TCHAR_TO_UTF8(*FilePath));

    // 1) HDF5 파일 오픈
    //    H5F_ACC_RDONLY : 읽기 전용 모드
    UE_LOG(LogTemp, Log, TEXT("log1.1"));
    H5::H5File File(StdFilePath.c_str(), H5F_ACC_RDONLY);

    // 2) 데이터셋(Dataset) 오픈
    //    여기서는 /Frame-000001 바로 아래에 point cloud 데이터가 있다고 가정
    //    만약 /Frame-000001/PointCloud 또는 /Frame-000001/Points 등의 하위 경로가 있다면
    //    해당 실제 경로를 반영해 주세요.

    UE_LOG(LogTemp, Log, TEXT("log2"));
    std::string StdHDFdir(TCHAR_TO_UTF8(*HDFDir));

    if( H5Lexists(File.getId(), StdHDFdir.c_str(), H5P_DEFAULT) < 1 ){
      UE_LOG(LogTemp, Warning, TEXT("HDF file does not exist"));
      return;
    }

    H5::DataSet Dataset = File.openDataSet(StdHDFdir);

    // 3) 데이터셋의 차원(Dataspace) 정보 확인
    H5::DataSpace Dataspace = Dataset.getSpace();
    const int Rank = Dataspace.getSimpleExtentNdims(); // 일반적으로 2차원 (N × 3)
    
    if (Rank != 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("데이터셋 차원이 2가 아닙니다. (Rank: %d)"), Rank);
        return;
    }

    // 4) 차원 크기 확인
    UE_LOG(LogTemp, Log, TEXT("log3"));
    hsize_t Dims[2] = {0, 0};
    Dataspace.getSimpleExtentDims(Dims, nullptr);

    //    Dims[0] = N(포인트 개수), Dims[1] = 3(각 점의 x, y, z)
    const hsize_t NumPoints = Dims[0];
    const hsize_t NumComponents = Dims[1]; // 보통 3

    if (NumComponents != 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("데이터셋의 열 개수가 3이 아닙니다. (NumComponents: %llu)"), NumComponents);
        return;
    }

    // 5) 실제 데이터 읽기
    //    N × 3개의 float 값을 한꺼번에 읽어올 배열을 준비
    UE_LOG(LogTemp, Log, TEXT("log4"));
    TArray<float> Buffer;
    Buffer.SetNumUninitialized(NumPoints * NumComponents);

    //    HDF5에서 float로 읽어올 것이므로 PredType::NATIVE_FLOAT 사용
    Dataset.read(Buffer.GetData(), H5::PredType::NATIVE_FLOAT);

    // 6) 읽어온 float 배열을 TArray<FVector>로 변환
    OutPointCloud.Reserve(NumPoints);
    for (int64 i = 0; i < static_cast<int64>(NumPoints); ++i)
    {
        const float X = Buffer[i * 3 + 0];
        const float Y = Buffer[i * 3 + 1];
        const float Z = Buffer[i * 3 + 2];

        // Unreal의 FVector 형태로 변환 후 OutPointCloud에 추가
        OutPointCloud.Add(FVector(X, Y, Z));
    }

    UE_LOG(LogTemp, Log, TEXT("HDF5 파일에서 포인트 클라우드 %lld개를 성공적으로 읽었습니다."), NumPoints);

}


bool UHDFParserComponent::SavePCLToHDF5(const FString& FilePath, const FString& HDFDir, const TArray<FVector>& PointCloud)
{
    try
    {
        std::string StdFilePath(TCHAR_TO_UTF8(*FilePath));
        // 2) HDF5 파일을 읽기/쓰기(RW) 모드로 오픈
        //    만약 파일이 없으면 에러가 날 텐데, 필요 시 try/catch로 없으면 만들기 등의 로직을 추가하셔도 됩니다.
        H5::H5File File(StdFilePath, H5F_ACC_RDWR);

        // 3) 프레임별 그룹 이름 생성: /frame-000123 같은 식
        std::string StdFrameGroupName(TCHAR_TO_UTF8(*HDFDir));

        // 4) 그룹 열기 or 없으면 생성
        H5::Group FrameGroup;
        try
        {
            FrameGroup = File.openGroup(StdFrameGroupName.c_str());
        }
        catch (...)
        {
            // 그룹이 없으면 새로 생성
            FrameGroup = File.createGroup(StdFrameGroupName.c_str());
        }

        // 5) TArray<FVector> → float 배열(N×3) 변환
        const int64 NumPoints = PointCloud.Num();
        if (NumPoints == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("PointCloud is empty, nothing to save."));
            return true;
        }

        TArray<float> Buffer;
        Buffer.Reserve(NumPoints * 3);
        for (int64 i = 0; i < NumPoints; ++i)
        {
            Buffer.Add(PointCloud[i].X);
            Buffer.Add(PointCloud[i].Y);
            Buffer.Add(PointCloud[i].Z);
        }

        // 6) 데이터셋을 그룹 내부에 /frame-xxxxx/pointcloud 경로로 생성
        hsize_t Dims[2] = {static_cast<hsize_t>(NumPoints), 3};
        H5::DataSpace Dataspace(2, Dims);

        // 최종 경로: "/frame-xxxxx/pointcloud"
        std::string DatasetPath = StdFrameGroupName + "/pointcloud";

        H5::DataSet Dataset = File.createDataSet(
            DatasetPath.c_str(),
            H5::PredType::NATIVE_FLOAT,
            Dataspace
        );

        // 7) 실제 데이터 쓰기
        Dataset.write(Buffer.GetData(), H5::PredType::NATIVE_FLOAT);

        UE_LOG(LogTemp, Log, TEXT("Saved %lld points to %s"), NumPoints, *HDFDir);

        // 리소스 정리
        Dataset.close();
        FrameGroup.close();
        File.close();

        return true;
    }
    catch (H5::FileIException &e)
    {
        e.printErrorStack();
        UE_LOG(LogTemp, Error, TEXT("HDF5 FileIException occurred."));
        return false;
    }
    catch (H5::DataSetIException &e)
    {
        e.printErrorStack();
        UE_LOG(LogTemp, Error, TEXT("HDF5 DataSetIException occurred."));
        return false;
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("Unknown exception occurred while saving point cloud to HDF5."));
        return false;
    }
}
