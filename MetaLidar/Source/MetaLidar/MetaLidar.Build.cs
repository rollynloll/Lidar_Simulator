// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;

public class MetaLidar : ModuleRules
{
	public MetaLidar(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		//Type = ModuleType.External;

		
		// 1) ThirdParty 경로 등록
		string ThirdPartyPath = Path.Combine(ModuleDirectory, "ThirdParty/HDF5/1.14.6");
        string IncludePath = Path.Combine(ThirdPartyPath, "include");
        string LibPath = Path.Combine(ThirdPartyPath, "lib");
		string BinPath = Path.Combine(ThirdPartyPath, "bin");
		PublicDefinitions.Add("H5_SIZEOF_SSIZE_T=8");

		string SanitizedPath = ThirdPartyPath.Replace("\\", "/");
		PublicDefinitions.Add($"THIRDPARTY_PATH=\"{SanitizedPath}\"");
		// PublicDefinitions.Add("H5_SIZEOF_SSIZE_T=8");

        // 2) Include 경로 등록
        PublicIncludePaths.Add(IncludePath);

        // 3) 라이브러리 파일 등록 (정적 or 동적 .lib 파일)
        PublicAdditionalLibraries.Add(Path.Combine(LibPath, "hdf5.lib"));
		PublicAdditionalLibraries.Add(Path.Combine(LibPath, "hdf5_cpp.lib"));
		PublicAdditionalLibraries.Add(Path.Combine(LibPath, "hdf5_hl.lib"));

		// 4) DLL(동적 라이브러리) 파일 등록
		PublicDefinitions.Add("H5_BUILT_AS_DYNAMIC_LIB");
		//PublicDelayLoadDLLs.Add("hdf5.dll");
        //PublicDelayLoadDLLs.Add("hdf5_cpp.dll");
        RuntimeDependencies.Add(Path.Combine(BinPath, "hdf5.dll"));
        RuntimeDependencies.Add(Path.Combine(BinPath, "hdf5_cpp.dll"));
		
		PublicDefinitions.Add("_ITERATOR_DEBUG_LEVEL=0");  // 디버그 모드와 릴리즈 모드 충돌 방지
		PublicDefinitions.Add("USE_STD_STRING");          // std::string 관련 문제 해결
		bUseRTTI = true;                                  // HDF5는 RTTI 필요
		bEnableExceptions = true;                         // 예외 처리 활성화

		if (Target.Configuration == UnrealTargetConfiguration.Debug)
		{
			PublicDefinitions.Add("_DEBUG");
			PublicDefinitions.Add("_DLL");
		}
		else
		{
			PublicDefinitions.Add("NDEBUG");
			PublicDefinitions.Add("_DLL");
		}



		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"ProceduralMeshComponent",
				"Niagara",
				"RenderCore",
				"RHI",
    			"MeshDescription",
				"StaticMeshDescription",
				"RawMesh"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"PhysicsCore",				
				"UDPWrapper",
				"Projects",
				"RenderCore",
				"RHI",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"EditorScriptingUtilities" 	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

	}
}
