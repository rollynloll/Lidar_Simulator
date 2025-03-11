
| ----------- |
| **플러그인 요약** |

**1.** **설치 유의사항**

Lidar 기반 Unreal Engine 플러그인을 활용하기 위해 다음과 같은 환경 설정이 필요하다.

- **환경 변수 설정:** HDF 서드파티 라이브러리의 bin 경로를 환경 변수에 추가해야 한다.

  - 예: C:\Users\President\Documents\UnrealProjects\ScanVerticles\Plugins\MetaLidar\Source\MetaLidar\ThirdParty\HDF5\1.14.6\bin

- **프로젝트 빌드 설정:** ScanVerticles.Build.cs 파일에 필요한 플러그인을 추가한다.

  ·        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara", "MetaLidar" });

PrivateDependencyModuleNames.AddRange(new string[] { "MetaLidar", "UDPWrapper" });

**2.** **전체적인 파이프라인**

본 플러그인은 VelodyneBaseComponent (VC), VelodyneLidarActor (VA), HDFParserComponent (HP)를 중심으로 구성된다.

**(1) VelodyneLidarActor (VA)**

  - **Begin Play** **단계:**
  
    - ClearDirectory: CSV 캐시를 비움
    - DeleteExistingStaticMeshes: Static Mesh를 Unbuild
    - SpawnMeshes: LoadMesh Mode에서 사용
  
  - **Thread** **단계:**
  
    - LidarThreadTick: Thread에서 Tick마다 호출
    - SpawnPointCloud: Point cloud 정보를 받아 Niagara 시스템에 전달
    - RecordSkinnedMesh: Skeletal Mesh의 데이터를 탐색
    - CacheSkeletalMeshFrame: Frame 단위로 Indices, Vertices 저장
    - DeltaTimeCall: FPS 계산 수행
  
  - **EndPlay** **단계:**
  
    - StaticMeshProcess: Anim Mode에서 Static Mesh 생성
    - CreateStaticMeshFromSkinnedFrameData: FrameData 기반 Static Mesh 생성
    - PackageStaticMesh: Static Mesh 저장 및 빌드
    - AnalyzeFPS: FPS 분석

**(2) VelodyneBaseComponent (VC)**

  - GetIntensity: Lidar 충돌 시 Intensity 계산
  - GetScanData: Lidar 센서 RayCasting 수행
  - GenerateCSVfromPCL: Point cloud 데이터를 CSV 파일로 변환
  - VisualizeData: CSV 데이터를 불러와 Point Cloud로 변환

**(3) HDFParserComponent (HP)**

  - LoadPCLFromHDF5: HDF 파일을 읽어 Point Cloud 정보 저장
  - SavePCLToHDF5: Point Cloud 정보를 HDF 형식으로 저장
