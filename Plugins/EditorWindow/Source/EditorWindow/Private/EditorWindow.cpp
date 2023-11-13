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
#include "Engine/Engine.h"
#include <EditorSupportDelegates.h>
#include <LevelUtils.h>
#include <Subsystems/EditorActorSubsystem.h>
#include "WorldBrowser/Public/WorldBrowserModule.h"

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
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SEditableTextBox)
					.OnTextChanged_Lambda([this](const FText& NewText) {
							int32 Num = FCString::Atoi(*(NewText.ToString()));
							if (Num == 0)
							{
								FText DialogText = FText::FromString("Input a positive integer!");
								FMessageDialog::Open(EAppMsgType::Ok, DialogText);
							}
							else {
								BuildNum = Num;
							}
					})
					.Text_Lambda([this]() { 
						if (BuildNum == 0) {
							return FText();
						}
						else {
							return FText(FText::FromString(FString::FromInt(BuildNum)));
						}
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(5)
				[
					SNew(SButton)
					.Text(FText::FromString("Build"))
					.HAlign(HAlign_Center)
					.OnClicked_Raw(this, &FEditorWindowModule::BuildButtonClicked)
				]
			]
			+SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SButton)
				.Text(FText::FromString("Replace"))
				.HAlign(HAlign_Center)
				.OnClicked_Raw(this, &FEditorWindowModule::ReplaceButtonClicked)
			]
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(2)
				[
					SNew(SEditableTextBox)
					.OnTextChanged_Lambda([this](const FText& NewText) {
						int32 Num = FCString::Atoi(*(NewText.ToString()));
						if (Num == 0 && NewText.ToString() != "")
						{
							FText DialogText = FText::FromString("Input a positive integer!");
							FMessageDialog::Open(EAppMsgType::Ok, DialogText);
						}
						else {
							SeedNum = Num;
						}
					})
					.Text_Lambda([this]() {
						if (SeedNum == 0) {
							return FText();
						}
						else {
							return FText(FText::FromString(FString::FromInt(SeedNum)));
						}
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(4)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Seed Value (Leave empty for random)")))
				]
			]
			+SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SCheckBox)
					.OnCheckStateChanged_Raw(this, &FEditorWindowModule::CheckboxButtonStateChanged)
				]
				+SHorizontalBox::Slot()
				.FillWidth(5)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Delete on replace")))
				] 
			]
		];
		PIERenameCounter = 0;
		DeleteOnReplace = false;
}

void FEditorWindowModule::CheckboxButtonStateChanged(ECheckBoxState State)
{
	DeleteOnReplace = State == ECheckBoxState::Checked ? true : false;
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

void FEditorWindowModule::GetAllLevels(UWorld* world, TSet<ULevelStreaming*>& OutLevels) {
	if (world == nullptr) return;
	
	FWorldContext& NewWorldContext = GEngine->CreateNewWorldContext(EWorldType::None);
	NewWorldContext.SetCurrentWorld(world);

	world->RefreshStreamingLevels();

	TSet<ULevelStreaming*> TempLevels;
	const TArray<ULevelStreaming*> Levels = world->GetStreamingLevels();
	const short LevelsSize = Levels.Num();

	bool alterTransform = !LevelsWithAlteredTransform.Contains(world);

	for (int i = 0; i < LevelsSize; i++)
	{
		//GetAllSubLevels(Levels[i], TempLevels, alterTransform);
		OutLevels.Add(Levels[i]);
	}
	/*
	if (alterTransform)
	{
		LevelsWithAlteredTransform.Add(world);
	}

	for (ULevelStreaming* Level : TempLevels)
	{
		OutLevels.Add(Level);
	}
	*/
	GEngine->DestroyWorldContext(world);
}

//to be removed
void FEditorWindowModule::GetAllSubLevels(ULevelStreaming* level, TSet<ULevelStreaming*>& OutLevels, bool AlterTransform) {
	TSet<ULevel*> TempLevels;

	UWorld* world = Cast<UWorld>(level->GetLoadedLevel()->GetOuter());
	FWorldContext& NewWorldContext = GEngine->CreateNewWorldContext(EWorldType::None);
	NewWorldContext.SetCurrentWorld(world);

	world->RefreshStreamingLevels();

	const TArray<ULevelStreaming*> Levels = world->GetStreamingLevels();
	const short LevelsSize = Levels.Num();

	if (AlterTransform)
	{
		for (ULevelStreaming* SubLevel : Levels)
		{
			SubLevel->LevelTransform += level->LevelTransform;
		}
	}

	for (int i = 0; i < LevelsSize; i++)
	{
		GetAllSubLevels(Levels[i], OutLevels, AlterTransform);
		OutLevels.Add(Levels[i]);
	}

	GEngine->DestroyWorldContext(world);
}

bool FEditorWindowModule::LoadFullLevel(UWorld* World, FTransform Transform, bool AddToGameLevels)
{
	TSet<ULevelStreaming*> AllLevels;

	GetAllLevels(World, AllLevels);

	bool success = false;
	ULevelStreaming* LevelStream = ULevelStreamingDynamic::LoadLevelInstance(GEditor->GetEditorWorldContext().World(),
																			World->GetPathName(),
																			Transform.GetLocation(),
																			Transform.Rotator(),
																			success);
	if (!success) return false;
	LevelStream->SetFolderPath(FName("/" + World->GetName() + "_" + FString::FromInt(PIERenameCounter)));
	FLinearColor RootLevelColor = LevelStream->LevelColor;

	FEditorDelegates::RefreshLevelBrowser.Broadcast();
	if(AddToGameLevels) GameLevels.Add(LevelStream, AllLevels);

	for (ULevelStreaming* Level : AllLevels) {
		FTransform SubLevelTransform = Level->LevelTransform + Transform;
		LevelStream = ULevelStreamingDynamic::LoadLevelInstance(GEditor->GetEditorWorldContext().World(),
																Level->GetLoadedLevel()->GetOuter()->GetPathName(),
																SubLevelTransform.GetLocation(),
																SubLevelTransform.Rotator(),
																success);
		if (!success) return false;
		LevelStream->SetFolderPath(FName("/" + World->GetName() + "_" + FString::FromInt(PIERenameCounter)));
		LevelStream->LevelColor = RootLevelColor;
		FEditorDelegates::RefreshLevelBrowser.Broadcast();
	}
	PIERenameCounter++;
	return true;
}

FReply FEditorWindowModule::BuildButtonClicked()
{
	if(!ErrorCheck()) return FReply::Handled();

	for (int i = 0; i < BuildNum; i++)
	{
		TArray<ULevelStreaming*> Levels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();

		FTransform Transform;
		Transform.SetLocation(FVector(GameLevels.Num() * 1000.0f, 0.0f, 0.0f));
		Transform.SetRotation(FQuat::Identity);

		const TMap<FName, uint8*> DataTableRows = DataTable->GetRowMap();
		TSet<FName> KeysSet;
		int32 NrOfKeys = DataTableRows.GetKeys(KeysSet);

		//FName RowName = KeysSet.Array()[FMath::RandRange(0, NrOfKeys - 1)];
		FName RowName = KeysSet.Array()[i%NrOfKeys];
		FWorldStruct* ChosenLevel = DataTable->FindRow<FWorldStruct>(RowName, "");

		UWorld* levelworld = ChosenLevel->World.LoadSynchronous();
		LoadFullLevel(levelworld, Transform);
	}
	
	return FReply::Handled();
}

FString FEditorWindowModule::ClearPathFormatting(FString InputString, FString RemoveFrom)
{
	TArray<FString> TempArray;
	InputString.ParseIntoArray(TempArray, *(RemoveFrom));
	return TempArray[0];
}

FReply FEditorWindowModule::ReplaceButtonClicked()
{
	if (!ErrorCheck()) return FReply::Handled();

	const TMap<FName, uint8*> DataTableRows = DataTable->GetRowMap();
	const TArray<ULevelStreaming*> StreamedLevels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();

	FRandomStream RandomStream;
	if (SeedNum == 0) {
		RandomStream.GenerateNewSeed();
		SeedNum = RandomStream.GetCurrentSeed();
	}
	else {
		RandomStream.Initialize(SeedNum);
	}

	int32 CreatedLevelNumber = 0;
	//for each row in user datatable
	for (TPair<FName, uint8*> Row : DataTableRows)
	{
		FWorldStruct* RowData = (FWorldStruct*)Row.Value;
		RowData->World.LoadSynchronous();
		UWorld* RowWorld = RowData->World.Get();
		TSet<TSoftObjectPtr<UWorld>> RowReplaceWorlds = RowData->ReplaceWorlds;

		TMap<ULevelStreaming*, TSet<ULevelStreaming*>> InstancedGameLevels(GameLevels);
		//for each created level
		for (TPair<ULevelStreaming*, TSet<ULevelStreaming*>> InstancedLevel : InstancedGameLevels)
		{
			//hotfix
			if (InstancedLevel.Key == nullptr || InstancedLevel.Key->GetLoadedLevel() == nullptr) continue;
			FString Path1 = ClearPathFormatting(InstancedLevel.Key->GetLoadedLevel()->GetOuter()->GetPathName(), "_LevelInstance_");
			FString Path2 = ClearPathFormatting(RowWorld->GetPathName(), ".");
			if (Path1 == Path2)
			{
				//if set for deletion
				if (DeleteOnReplace)
				{
					for (ULevelStreaming* InstancedSubLevel : InstancedLevel.Value)
					{
						for (ULevelStreaming* InstancedSublevelWorld : StreamedLevels)
						{
							if (InstancedSublevelWorld->GetIsRequestingUnloadAndRemoval()) continue;
							if (InstancedSublevelWorld->GetCurrentState() == ULevelStreaming::ECurrentState::Removed || InstancedSublevelWorld->GetLoadedLevel() == nullptr) continue;
							FString p1 = ClearPathFormatting(InstancedSubLevel->GetLoadedLevel()->GetOuter()->GetPathName(), ".");
							FString p2 = ClearPathFormatting(InstancedSublevelWorld->GetLoadedLevel()->GetOuter()->GetPathName(), "_LevelInstance_");
							if (p1 == p2)
							{

								EditorLevelUtils::RemoveLevelFromWorld(InstancedSublevelWorld->GetLoadedLevel());
								break;
							}
						}
					}
					GameLevels.Remove(InstancedLevel.Key);



					//TODO: remove the created folder somehow (rebooting the Worldbrowser module does the trick but an internam check fails :<)
					/*
					UObject* FolderPtr = InstancedLevel.Key->GetOutermostObject();
					FName FolderPath = InstancedLevel.Key->GetFolderPath();
					InstancedLevel.Key->GetFolderRootObject()

					FFolder Folder2 = FFolder::GetWorldRootFolder((UWorld*)InstancedLevel.Key->GetLoadedLevel()->GetOuter());
					UObject* FolderPtr2 = Folder2.GetRootObjectPtr();

					FolderPtr->ConditionalBeginDestroy();
					*/

					//FWorldBrowserModule* WBModule = (FWorldBrowserModule*)FModuleManager::Get().GetModule(TEXT("Worldbrowser"));
					//TSharedPtr<FLevelCollectionModel> WorldModel = WBModule->SharedWorldModel((UWorld*)GEditor->GetEditorWorldContext().World());
					//WBModule->ShutdownModule();
					//WBModule->StartupModule();

					EditorLevelUtils::RemoveLevelFromWorld(InstancedLevel.Key->GetLoadedLevel());

					FEditorDelegates::RefreshLevelBrowser.Broadcast();
				}
				
				int32 RandomNumer = RandomStream.RandRange(0, RowReplaceWorlds.Num() - 1);
				
				UWorld* ReplaceWorld = RowReplaceWorlds.Array()[RandomNumer].LoadSynchronous();
				LoadFullLevel(ReplaceWorld, InstancedLevel.Key->LevelTransform, DeleteOnReplace);
			}
		}
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEditorWindowModule, EditorWindow)
