
|             |
| ----------- |
| **플러그인 요약** |

**1.** **설치 유의사항**

  Lidar 기반 Unreal Engine 플러그인을 활용하기 위해 다음과 같은 환경 설정이 필요하다.
    
    - 환경 변수 설정: HDF 서드파티 라이브러리의 bin 경로를 환경 변수에 추가해야 한다.
    
      예: C:\Users\President\Documents\UnrealProjects\ScanVerticles\Plugins\MetaLidar\Source\MetaLidar\ThirdParty\HDF5\1.14.6\bin
    
    - 프로젝트 빌드 설정: ScanVerticles.Build.cs 파일에 필요한 플러그인을 추가한다.
    
      PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara", "MetaLidar" });
      PrivateDependencyModuleNames.AddRange(new string[] { "MetaLidar", "UDPWrapper" });

**2.** **전체적인 파이프라인**

본 플러그인은 MetaLidar 플러그인을 기반으로 설계되었으며, VelodyneBaseComponent (VC), VelodyneLidarActor (VA), HDFParserComponent (HP)를 중심으로 구성된다.
(MetaLidar Git: https://github.com/metabotics-ai/MetaLidar)
  **(1) VelodyneLidarActor (VA)**
  
    - Begin Play단계:
    
      - ClearDirectory: CSV 캐시를 비움
      - DeleteExistingStaticMeshes: Static Mesh를 Unbuild
      - SpawnMeshes: LoadMesh Mode에서 사용
    
    - Thread단계:
    
      - LidarThreadTick: Thread에서 Tick마다 호출
      - SpawnPointCloud: Point cloud 정보를 받아 Niagara 시스템에 전달
      - RecordSkinnedMesh: Skeletal Mesh의 데이터를 탐색
      - CacheSkeletalMeshFrame: Frame 단위로 Indices, Vertices 저장
      - DeltaTimeCall: FPS 계산 수행
    
    - EndPlay단계:
    
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

**3.** **Lidar 모드 설명 및 예시**

본 Lidar 시뮬레이터는 VA에서 정의한 enumerator ELidarMode에 따라 5개의 모드로 구분된다.
<code>
    enum class ELidarMode : uint8
  {
      ScanMode UMETA(DisplayName = "Scanning Mode"), // Lidar 스캔 및 실시간 시각화 모드
      VisMode  UMETA(DisplayName = "Visualize Mode"),// CSV 파일을 읽어 Lidar 시각화
      AnimMode UMETA(DisplayName = "Animation Mode"),// 캐릭터의 Animation을 녹화
      LoadMode UMETA(DisplayName = "LoadMesh Mode"), // Static Mesh를 불러 Lidar 스캔
      HDFMode UMETA(DisplayName = "HDF5 Visualize Mode") // HDF 파일을 읽어 Lidar 시각화
  };
</code>

  **(1)  Scanning Mode**  <br/>
  
  i. 모드 설명

    1.     Ray Casting으로 Lidar 충돌 지점을 계산
    2.     CSV 및 HDF 파일로 저장 및 Niagara System에 데이터 파싱
    3.     Niagara Particle System으로 충돌 위치 실시간 시각화
    
  ii. 예시
  
[![YouTube 썸네일](https://img.youtube.com/vi/-W8CY4Kvdxk/0.jpg)](https://youtu.be/-W8CY4Kvdxk)

  **(2)  Visualize Mode**
  
  i.	모드 설명
  
    1.	저장된 CSV 파일을 읽어 point cloud 정보로 변환
    2.	정보를 Niagara System으로 전달하여 실시간 시각화

  ii. 예시

  [![YouTube 썸네일](https://img.youtube.com/vi/4DoOWkCwtHM/0.jpg)](https://youtu.be/4DoOWkCwtHM)
  
  **(3)  Animation Mode**

  i. 모드 설명

    1.    Thread 단계에서 프레임마다 Skeletal Mesh의 Polygon정보 추출
    2.    EndPlay 단계에서 Polygon 정보를 바탕으로 메타휴먼의 Static Mesh
    3.    생성한 프레임별 Static Mesh를 프로젝트에 저장하고 빌드

  ii. 예시

  [![YouTube 썸네일](https://img.youtube.com/vi/9rulrT0kaNA/0.jpg)](https://youtu.be/9rulrT0kaNA)

  **(4)  LoadMesh Mode**

  i. 모드 설명

    1.	BeginPlay시 StaticMesh 폴더에 저장된 StaticMesh를 프레임별로 스폰하여 RayCasting 계산
    2.	계산한 RayCasting값을 CSV 및 HDF파일로 저장
    3.	모드를 Visualize Mode로 전환 후 Thread 단계 시행

  ii. 예시

  [![YouTube 썸네일](https://img.youtube.com/vi/MbRYjCY1FeA/0.jpg)](https://youtu.be/MbRYjCY1FeA)

  **(5)  HDF5 Visualize Mode**

  i.	모드 설명
  
    1.	HDF 파일을 읽어서 point cloud 시각화, visualize mode의 hdf 버전으로 보면 된다.
    2.	프레임마다 hdf에서 point cloud 정보를 읽어서 배열에 저장
    3.	Niagara System으로 전달하여 실시간 시각화

  ii. 예시

  없음, Visualize Mode 참고
  
