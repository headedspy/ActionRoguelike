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
#include "PropertyCustomizationHelpers.h"
#include "WorldBrowser/Public/WorldBrowserModule.h"
#include "EditorLevelUtils.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/LevelStreamingAlwaysLoaded.h"

/*
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
#include "Engine/LevelStreamingPersistent.h"
#include "HAL/FileManagerGeneric.h"
#include "EditorAssetLibrary.h"
#include <UObject/ConstructorHelpers.h>
#include <Kismet/GameplayStatics.h>
#include "Engine/Engine.h"
#include <EditorSupportDelegates.h>
#include <LevelUtils.h>
#include <Subsystems/EditorActorSubsystem.h>
#include "WorldBrowser/Public/WorldBrowserModule.h"
#include "Folder.h"
#include "ActorFolder.h"
*/

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
					.OnShouldFilterAsset_Lambda([this](const FAssetData& Data) {
						DataTable = Cast<UDataTable>(Data.GetAsset());
						if (DataTable->RowStruct == FWorldStruct::StaticStruct()) return false;
						return true;
					})
					.OnObjectChanged_Lambda([this](const FAssetData& Data) {
						DataTablePath = Data.GetObjectPathString();
						DataTable = Cast<UDataTable>(Data.GetAsset());
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
							BuildNum = 1;
							return FText::FromString("1");
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
							SeedNum = 0;
						}
						else {
							SeedNum = Num;
						}
					})
					.Text_Lambda([this]() {
						if (SeedNum == 0) {
							return (RandomizeSeed ? FText() : FText::FromString("0"));
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
					.Text(FText::FromString(TEXT(" Seed Value")))
				]
			]
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SCheckBox)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
						RandomizeSeed = (State == ECheckBoxState::Checked);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(9)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT(" Randomize Seed")))
				]
			]
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SButton)
				.Text(FText::FromString("Load"))
				.HAlign(HAlign_Center)
				.OnClicked_Raw(this, &FEditorWindowModule::ChangeStreamingLevelsMode)
			]
		];
		PIEFolderNameCounter = 0;
		BuildNum = 0;
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


// HELPER FUNCTIONS
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

FString FEditorWindowModule::ClearPathFormatting(FString InputString, FString RemoveFrom)
{
	TArray<FString> TempArray;
	InputString.ParseIntoArray(TempArray, *(RemoveFrom));
	return TempArray[0];
}

void FEditorWindowModule::GetAllLevels(UWorld* world, TSet<ULevelStreaming*>& OutLevels) {
	if (world == nullptr) return;
	
	FWorldContext& NewWorldContext = GEngine->CreateNewWorldContext(EWorldType::None);
	NewWorldContext.SetCurrentWorld(world);

	world->FlushLevelStreaming();

	TSet<ULevelStreaming*> TempLevels;
	const TArray<ULevelStreaming*> Levels = world->GetStreamingLevels();
	const short LevelsSize = Levels.Num();

	for (int i = 0; i < LevelsSize; i++)
	{
		OutLevels.Add(Levels[i]);
	}

	GEngine->DestroyWorldContext(world);
}

ULevelStreaming* FEditorWindowModule::LoadFullLevel(UWorld* World, FTransform Transform)
{
	TSet<ULevelStreaming*> AllLevels;

	GetAllLevels(World, AllLevels);

	bool success = false;

	ULevelStreaming* RootLevelStream = ULevelStreamingDynamic::LoadLevelInstance(GEditor->GetEditorWorldContext().World(),
																			World->GetPathName(),
																			Transform.GetLocation(),
																			Transform.Rotator(),
																			success);

	if (!success) return nullptr;
	RootLevelStream->SetFolderPath(FName("/" + World->GetName() + "_" + FString::FromInt(PIEFolderNameCounter)));
	FLinearColor RootLevelColor = RootLevelStream->LevelColor;

	FEditorDelegates::RefreshLevelBrowser.Broadcast();
	GameLevels.Add(RootLevelStream, AllLevels);

	for (ULevelStreaming* Level : AllLevels) {
		FTransform SubLevelTransform = Level->LevelTransform + Transform;
		ULevelStreaming* LevelStream = ULevelStreamingDynamic::LoadLevelInstance(GEditor->GetEditorWorldContext().World(),
																Level->GetLoadedLevel()->GetOuter()->GetPathName(),
																SubLevelTransform.GetLocation(),
																SubLevelTransform.Rotator(),
																success);

		if (!success) return nullptr;
		LevelStream->SetFolderPath(FName("/" + World->GetName() + "_" + FString::FromInt(PIEFolderNameCounter)));
		LevelStream->LevelColor = RootLevelColor;
	}
	PIEFolderNameCounter++;

	FEditorDelegates::RefreshLevelBrowser.Broadcast();
	return RootLevelStream;
}

void FEditorWindowModule::RemoveSubLevelFromWorld(ULevelStreaming* LevelStream)
{
	TArray<ULevel*> Levels = GEditor->GetEditorWorldContext().World()->GetLevels();

	for (ULevel* LoadedLevel : Levels)
	{
		//compare levels by their BuildDataId
		if (LoadedLevel->LevelBuildDataId == LevelStream->GetLoadedLevel()->LevelBuildDataId)
		{
			UEditorLevelUtils::RemoveLevelFromWorld(LoadedLevel);
			break;
		}
	}
}

bool FEditorWindowModule::UnloadFullLevel(ULevelStreaming* LevelStream)
{
	FWorldBrowserModule* WBModule = (FWorldBrowserModule*)FModuleManager::Get().GetModule(TEXT("Worldbrowser"));
	TSharedPtr<FLevelCollectionModel> WorldModel = WBModule->SharedWorldModel((UWorld*)(LevelStream->GetLoadedLevel()->GetOuter()));

	//remove all sublevels
	TSet<ULevelStreaming*> SubLevels = GameLevels[LevelStream];
	for (ULevelStreaming* SubLevel : SubLevels)
	{
		RemoveSubLevelFromWorld(SubLevel);
	}

	//remove root level stream
	UEditorLevelUtils::RemoveLevelFromWorld(LevelStream->GetLoadedLevel());

	//delete empty folder by restarting the worldbrowser module
	WorldModel = nullptr;

	WBModule->ShutdownModule();
	WBModule->StartupModule();

	FEditorDelegates::RefreshLevelBrowser.Broadcast();
	GameLevels.Remove(LevelStream);

	return true;
}


// BUTTON FUNCTIONS
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

		FName RowName = KeysSet.Array()[i%NrOfKeys];
		FWorldStruct* ChosenLevel = DataTable->FindRow<FWorldStruct>(RowName, "");

		UWorld* levelworld = ChosenLevel->World.LoadSynchronous();
		LoadFullLevel(levelworld, Transform);
	}
	
	return FReply::Handled();
}

FReply FEditorWindowModule::ReplaceButtonClicked()
{
	if (!ErrorCheck()) return FReply::Handled();

	// Initialize rng with seed
	FRandomStream RandomStream;
	if (RandomizeSeed) {
		RandomStream.GenerateNewSeed();
		SeedNum = RandomStream.GetCurrentSeed();
	}
	else {
		RandomStream.Initialize(SeedNum);
	}

	TArray<ULevelStreaming*> LevelsToRemove;

	//for each row in user datatable
	const TMap<FName, uint8*> DataTableRows = DataTable->GetRowMap();
	for (TPair<FName, uint8*> Row : DataTableRows)
	{
		FWorldStruct* RowData = (FWorldStruct*)Row.Value;
		RowData->World.LoadSynchronous();
		UWorld* RowWorld = RowData->World.Get();
		TSet<TSoftObjectPtr<UWorld>> RowReplaceWorlds = RowData->ReplaceWorlds;

		//for each created level
		const TMap<ULevelStreaming*, TSet<ULevelStreaming*>> InstancedGameLevels = TMap<ULevelStreaming*, TSet<ULevelStreaming*>>(GameLevels);
		for (TPair<ULevelStreaming*, TSet<ULevelStreaming*>> InstancedLevel : InstancedGameLevels)
		{
			if (InstancedLevel.Key->GetLoadedLevel() == nullptr) continue;

			//return checks

			// Find the instanced level by name
			FString InstancedLevelPath = ClearPathFormatting(InstancedLevel.Key->GetLoadedLevel()->GetOuter()->GetPathName(), "_LevelInstance_");
			FString DataTableLevelPath = ClearPathFormatting(RowWorld->GetPathName(), ".");
			if (InstancedLevelPath == DataTableLevelPath)
			{
				FTransform InstancedLevelTransform = InstancedLevel.Key->LevelTransform;

				// If level already has a replacement, delete the replacement
				for (TPair<ULevelStreaming*, ULevelStreaming*> ReplacementLevel : ReplacementLevels)
				{
					if (ReplacementLevel.Key == InstancedLevel.Key)
					{
						//add level for removal
						LevelsToRemove.Add(ReplacementLevel.Value);
						ReplacementLevels.Remove(ReplacementLevel);

						break;
					}
				}
				
				// Create new level
				int32 RandomNumer = RandomStream.RandRange(0, RowReplaceWorlds.Num() - 1);

				UWorld* ReplaceWorld = RowReplaceWorlds.Array()[RandomNumer].LoadSynchronous();
				ULevelStreaming* RootLevelStream = LoadFullLevel(ReplaceWorld, InstancedLevelTransform);

				TPair<ULevelStreaming*, ULevelStreaming*> Pair;
				Pair.Key = InstancedLevel.Key;
				Pair.Value = RootLevelStream;
				ReplacementLevels.Add(Pair);
			}
		}
		
	}

	//remove all levels
	for (ULevelStreaming* LevelToRemove : LevelsToRemove)
	{
		UnloadFullLevel(LevelToRemove);
	}

	return FReply::Handled();
}

FReply FEditorWindowModule::ChangeStreamingLevelsMode()
{
	TArray<ULevelStreaming*> Levels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();
	for (ULevelStreaming* Level : Levels)
	{
		UEditorLevelUtils::SetStreamingClassForLevel(Level, ULevelStreamingAlwaysLoaded::StaticClass());
	}

	GEditor->GetEditorWorldContext().World()->RefreshStreamingLevels();

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEditorWindowModule, EditorWindow)
