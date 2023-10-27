// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditorWindow.h"
#include "EditorWindowStyle.h"
#include "EditorWindowCommands.h"
#include "LevelEditor.h"
#include "Framework/SlateDelegates.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "ToolMenus.h"
#include "Containers/StringFwd.h"
#include "PropertyCustomizationHelpers.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Math/Rotator.h"
#include "Engine/World.h"
#include "Engine/UserDefinedStruct.h"
#include "EditorLevelUtils.h"
#include "Engine/LevelStreamingAlwaysLoaded.h"
#include "Engine/LevelStreaming.h"
#include "HAL/FileManagerGeneric.h"
#include "EditorAssetLibrary.h"
#include <UObject/ConstructorHelpers.h>
#include <Kismet/GameplayStatics.h>

static const FName EditorWindowTabName("EditorWindow");

#define LOCTEXT_NAMESPACE "FEditorWindowModule"

void FEditorWindowModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FEditorWindowStyle::Initialize();
	FEditorWindowStyle::ReloadTextures();

	FEditorWindowCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FEditorWindowCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FEditorWindowModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEditorWindowModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(EditorWindowTabName, FOnSpawnTab::CreateRaw(this, &FEditorWindowModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FEditorWindowTabTitle", "EditorWindow"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FEditorWindowModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FEditorWindowStyle::Shutdown();

	FEditorWindowCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(EditorWindowTabName);
}

TSharedPtr<STextBlock> ComboBoxTitleBlock;
TArray<TSharedPtr<FString>> ComboItems;

TSharedRef<SDockTab> FEditorWindowModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("DataTable:")))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(3)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UDataTable::StaticClass())
					.DisplayBrowse(true)
					.DisplayThumbnail(true)
					.DisplayUseSelected(true)
					.EnableContentPicker(true)
					.AllowClear(false)
					.OnObjectChanged_Lambda([this](const FAssetData& Data) {
						if (Data.IsValid())
						{
							DataTablePath = Data.GetObjectPathString();
							DataTable = Cast<UDataTable>(Data.GetAsset());

							//check if selected datatable is created from WorldStruct
							if (DataTable->GetRowStructName() != "WorldStruct")
							{
								FText DialogText = FText::FromString("Table must be created from WorldStruct!");
								FMessageDialog::Open(EAppMsgType::Ok, DialogText);

								DataTable = nullptr;
								DataTablePath = "";
							}
						}
					})
					.ObjectPath_Lambda([this]() { return DataTablePath; })
				]
			]
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SButton)
				.Text(FText::FromString("Build"))
				.HAlign(HAlign_Center)
				.OnClicked_Raw(this, &FEditorWindowModule::BuildButtonClicked)
			]
			+SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SButton)
				.Text(FText::FromString("Replace"))
				.HAlign(HAlign_Center)
				.OnClicked_Raw(this, &FEditorWindowModule::ReplaceButtonClicked)
			]
		];
		Counter = 0;
}

void FEditorWindowModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(EditorWindowTabName);
}

void FEditorWindowModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FEditorWindowCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FEditorWindowCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

AActor* FEditorWindowModule::AddActor(TSubclassOf<AActor> ActorClass, FTransform Transform)
{
	ULevel* Level = GEditor->GetEditorWorldContext().World()->GetCurrentLevel();
	return GEditor->AddActor(Level, ActorClass, Transform);
}

bool FEditorWindowModule::ErrorCheck()
{
	if (DataTable == nullptr)
	{
		FText DialogText = FText::FromString("DataTable can't be empty!");
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);

		return false;
	}

	return true;
}


void FEditorWindowModule::GetAllLevels(UWorld* world, TSet<ULevel*>& OutLevels) {

	world->FlushLevelStreaming();

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(world, AActor::StaticClass(), AllActors);


	for (AActor* actor : AllActors)
	{
		OutLevels.Add(actor->GetLevel());
	}
}

FReply FEditorWindowModule::BuildButtonClicked()
{
	if(!ErrorCheck()) return FReply::Handled();

	//get only loaded and renamed levels
	/*
	TArray<ULevel*> Levels2 = GEditor->GetEditorWorldContext().World()->GetLevels().FilterByPredicate([](ULevel* Level) { 
		return Level->GetPathName().Contains("UEDPIE"); 
	})*/

	TArray<ULevelStreaming*> Levels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();

	FTransform Transform;
	Transform.SetLocation(FVector(Levels.Num()*1000.0f, 0.0f, 0.0f));
	Transform.SetRotation(FQuat::Identity);


	const TMap<FName, uint8*> DataTableRows = DataTable->GetRowMap();
	TSet<FName> KeysSet;
	int32 NrOfKeys = DataTableRows.GetKeys(KeysSet);

	FName RowName = KeysSet.Array()[FMath::RandRange(0, NrOfKeys-1)];


	FWorldStruct* ChosenLevel = DataTable->FindRow<FWorldStruct>(RowName, "");

	//FString level = ChosenLevel->World.ToSoftObjectPath().ToString(); /GAME PATH
	//FString level = ChosenLevel->World.GetAssetName(); ASSET NAME

	/*
	UWorld* levelworld = ChosenLevel->World.LoadSynchronous();
	levelworld->FlushLevelStreaming();

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(levelworld, AActor::StaticClass(), AllActors);

	TSet<ULevel*> AllLevels;
	
	for (AActor* actor : AllActors)
	{
		AllLevels.Add(actor->GetLevel());
	}
	*/


	UWorld* levelworld = ChosenLevel->World.LoadSynchronous();
	TSet<ULevel*> AllLevels;
	GetAllLevels(levelworld, AllLevels);

	for (ULevel* Level : AllLevels) {
		FText DialogText = FText::FromString(Level->GetPathName());
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);
	}




	ULevelStreaming* LevelStream = EditorLevelUtils::AddLevelToWorld(GEditor->GetEditorWorldContext().World(),
																	*ChosenLevel->World.ToSoftObjectPath().ToString(),
																	ULevelStreamingAlwaysLoaded::StaticClass(),
																	Transform);
	
	

	LevelStream->RenameForPIE(Counter++);

	return FReply::Handled();
}

struct DataTableData
{
	TSet<FString> ReplaceWith;
};

FReply FEditorWindowModule::ReplaceButtonClicked()
{
	if (!ErrorCheck()) return FReply::Handled();

	const TArray<ULevelStreaming*> StreamedLevels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();
	const TMap<FName, uint8*> RowMap = DataTable->GetRowMap();

	for (ULevelStreaming* StreamedLevel : StreamedLevels)
	{
		TArray<FString> Out;
		TArray<FString> Out2;
		StreamedLevel->GetLoadedLevel()->GetPathName().ParseIntoArray(Out, TEXT("/"), true);
		Out[4].ParseIntoArray(Out2, TEXT("."), true);

		FString CurrentLevelName = Out2[0];

		CurrentLevelName.ParseIntoArray(Out, TEXT("_"), true);

		FString OriginalLevelName = Out[2];

		/*
		//iterate through the datatable
		for (auto& Row : RowMap)
		{
			if (Row.Key.ToString() != OriginalLevelName) continue;

			TSet<FString> ReplaceWith = ((DataTableData*)Row.Value)->ReplaceWith;

			unsigned short RandomIndex = FMath::RandRange(0, ReplaceWith.Num() - 1);
			FString LevelNameRand = ReplaceWith.Array()[RandomIndex];

			FTransform StreamedLevelTransform = StreamedLevel->LevelTransform;

			ULevelStreaming* NewLevelStream = EditorLevelUtils::AddLevelToWorld(GEditor->GetEditorWorldContext().World(),
																				*(FolderPath + LevelNameRand),
																				ULevelStreamingAlwaysLoaded::StaticClass(),
																				StreamedLevelTransform);
			NewLevelStream->RenameForPIE(Counter++);
			
			EditorLevelUtils::RemoveLevelFromWorld(StreamedLevel->GetLoadedLevel());

			break;
		}
		*/
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEditorWindowModule, EditorWindow)
