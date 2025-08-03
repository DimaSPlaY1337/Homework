// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HomeworkProject : ModuleRules
{
    public HomeworkProject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { 
        "Core",
        "CoreUObject",
        "Engine",
        "InputCore",
        "Niagara",
        "GameplayTasks",
        "NavigationSystem",
        "OnlineSubsystem",
        "OnlineSubsystemUtils",//добавляем модули
        "SignificanceManager"
        });

        //добавляем динамически подключаем модуль, с помощью добавления в массив строки "OnlineSubsystemSteam"
        DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");//таким образом подключаем сетевой модуль для тестирования

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        PrivateIncludePaths.AddRange(new string[] { Name });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
