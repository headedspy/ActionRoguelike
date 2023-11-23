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
#include "Engine/LevelStreamingPersistent.h"

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

TSharedRef<SDockTab> FEditorWindowModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	PIELevelNameCounter = 0;
	PIEFolderNameCounter = 0;
	BuildNum = 0;
	FEditorDelegates::MapChange.AddRaw(this, &FEditorWindowModule::EditorMapChange);
	FEditorDelegates::OnAddLevelToWorld.AddRaw(this, &FEditorWindowModule::ManualAddLevel);

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
					.AllowClear(true)
					.OnShouldFilterAsset_Lambda([this](const FAssetData& Data) {
						if (!Data.IsValid()) return true;

						//filter only datatables from the currect struct
						UDataTable* NewDataTable = Cast<UDataTable>(Data.GetAsset());
						if (NewDataTable->RowStruct == FWorldStruct::StaticStruct()) return false;
					
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
			.Padding(10, 25)
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
					.Text(FText::FromString("Build (Debug Functionality)"))
					.HAlign(HAlign_Center)
					.OnClicked_Raw(this, &FEditorWindowModule::BuildButtonClicked)
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
						TrackLevels = (State == ECheckBoxState::Checked);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(9)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT(" Track added levels")))
				]
			]
			+SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SButton)
				.Text(FText::FromString("Generate"))
				.HAlign(HAlign_Center)
				.OnClicked_Raw(this, &FEditorWindowModule::GenerateButtonClicked)
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

FString FEditorWindowModule::ClearPathFormatting(FString InputString)
{
	FString PathFormat = "";
	//remove level instancing postfix or asset path formatting
	if (InputString.Contains("_LevelInstance_"))
	{
		TArray<FString> TempArray;
		InputString.ParseIntoArray(TempArray, TEXT("_LevelInstance_"));
		PathFormat = TempArray[0];
	}
	else if (InputString.Contains("."))
	{
		TArray<FString> TempArray;
		InputString.ParseIntoArray(TempArray, TEXT("."));
		PathFormat = TempArray[0];
	}

	//remove PIE renaming
	if (InputString.Contains("UEDPIE_"))
	{
		if(PathFormat == "") PathFormat = InputString;
		PathFormat.ReplaceInline(TEXT("UEDPIE"), TEXT(""));

		TArray<FString> TempArray;
		PathFormat.ParseIntoArray(TempArray, TEXT("_"));
		PathFormat = TempArray[0] + TempArray[2];
	}

	return PathFormat;
}

void FEditorWindowModule::GetAllLevels(UWorld* world, TSet<ULevelStreaming*>& OutLevels) {
	if (world == nullptr) return;
	
	FWorldContext& NewWorldContext = GEngine->CreateNewWorldContext(EWorldType::None);
	NewWorldContext.SetCurrentWorld(world);

	world->FlushLevelStreaming();

	TSet<ULevelStreaming*> TempLevels;
	const TArray<ULevelStreaming*> Levels = world->GetStreamingLevels();

	for (ULevelStreaming* Level : Levels)
	{
		OutLevels.Add(Level);
	}

	GEngine->DestroyWorldContext(world);
}

ULevelStreaming* FEditorWindowModule::LoadFullLevel(UWorld* World, FTransform Transform)
{
	TSet<ULevelStreaming*> AllLevels;


	bool success = false;

	GetAllLevels(World, AllLevels);
	
	ULevelStreaming* RootLevelStream = ULevelStreamingDynamic::LoadLevelInstance(GEditor->GetEditorWorldContext().World(),
																			World->GetPathName(),
																			Transform.GetLocation(),
																			Transform.Rotator(),
																			success);
	ensure(success);
	
	RootLevelStream->RenameForPIE(PIELevelNameCounter++);

	//dont create a folder if there are no sublevels
	if(AllLevels.Num() != 0) 
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

		ensure(success);

		LevelStream->RenameForPIE(PIELevelNameCounter++);
		LevelStream->SetFolderPath(FName("/" + World->GetName() + "_" + FString::FromInt(PIEFolderNameCounter)));
		LevelStream->LevelColor = RootLevelColor;
	}

	if (AllLevels.Num() != 0) 
		PIEFolderNameCounter++;

	FEditorDelegates::RefreshLevelBrowser.Broadcast();
	return RootLevelStream;
}

void FEditorWindowModule::ManualAddLevel(ULevel* Level)
{
	if (!TrackLevels) return;

	UWorld* LevelWorld = (UWorld*)Level->GetOuter();

	//find the level stream
	TArray<ULevelStreaming*> StreamingLevels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();
	for (ULevelStreaming* StreamingLevel : StreamingLevels)
	{
		if (StreamingLevel->GetLoadedLevel() == Level)
		{
			StreamingLevel->RenameForPIE(PIELevelNameCounter++);

			TSet<ULevelStreaming*> AllLevels;
			GetAllLevels(LevelWorld, AllLevels);

			//dont create a folder if there are no sublevels
			if(AllLevels.Num() != 0)
				StreamingLevel->SetFolderPath(FName("/" + LevelWorld->GetName() + "_" + FString::FromInt(PIEFolderNameCounter)));

			FLinearColor RootLevelColor = StreamingLevel->LevelColor;

			GameLevels.Add(StreamingLevel, AllLevels);

			for (ULevelStreaming* Level : AllLevels) {
				bool success = false;
				FTransform SubLevelTransform = Level->LevelTransform + StreamingLevel->LevelTransform;
				
				ULevelStreaming* LevelStream = ULevelStreamingDynamic::LoadLevelInstance(GEditor->GetEditorWorldContext().World(),
																						 Level->GetLoadedLevel()->GetOuter()->GetPathName(),
																						 SubLevelTransform.GetLocation(),
																						 SubLevelTransform.Rotator(),
																						 success);

				ensure(success);

				LevelStream->RenameForPIE(PIELevelNameCounter++);
				LevelStream->SetFolderPath(FName("/" + LevelWorld->GetName() + "_" + FString::FromInt(PIEFolderNameCounter)));
				LevelStream->LevelColor = RootLevelColor;
			}

			if (AllLevels.Num() != 0) 
				PIEFolderNameCounter++;
			break;
		}
	}
}

void FEditorWindowModule::EditorMapChange(uint32 flags)
{
	GameLevels.Empty();
	ReplacementLevels.Empty();


	//cleanup empty folders by rebooting worldbrowser module
	FWorldBrowserModule& WBModule = FModuleManager::LoadModuleChecked<FWorldBrowserModule>("WorldBrowser");
	TSharedPtr<FLevelCollectionModel> WorldModel = WBModule.SharedWorldModel(GEditor->GetEditorWorldContext().World());

	WorldModel.Reset();
	WBModule.ShutdownModule();
	WBModule.StartupModule();

	WBModule.OnBrowseWorld.Broadcast(GEditor->GetEditorWorldContext().World());
}

void FEditorWindowModule::RemoveSubLevelFromWorld(ULevelStreaming* LevelStream)
{
	TArray<ULevel*> Levels = GEditor->GetEditorWorldContext().World()->GetLevels();

	for (ULevel* LoadedLevel : Levels)
	{
		//compare levels by their BuildDataId
		if (LoadedLevel->LevelBuildDataId == LevelStream->GetLoadedLevel()->LevelBuildDataId)
		{
			ensure(UEditorLevelUtils::RemoveLevelFromWorld(LoadedLevel));
			break;
		}
	}
}

void FEditorWindowModule::UnloadFullLevel(ULevelStreaming* LevelStream)
{
	//if the level has already been manually removed
	if (LevelStream->GetCurrentState() == ULevelStreaming::ECurrentState::Removed ||
		LevelStream->GetCurrentState() == ULevelStreaming::ECurrentState::Unloaded)
	{
		//remove track and all sub-levels
		if (GameLevels.Contains(LevelStream))
		{
			TSet<ULevelStreaming*> SubLevels = GameLevels[LevelStream];
			for (ULevelStreaming* SubLevel : SubLevels)
			{
				RemoveSubLevelFromWorld(SubLevel);
			}
			GameLevels.Remove(LevelStream);
		}
		return;
	}

	//remove all sublevels
	TSet<ULevelStreaming*> SubLevels = GameLevels[LevelStream];

	FWorldBrowserModule& WBModule = FModuleManager::LoadModuleChecked<FWorldBrowserModule>("WorldBrowser");
	TSharedPtr<FLevelCollectionModel> WorldModel = WBModule.SharedWorldModel((UWorld*)LevelStream->GetLoadedLevel()->GetOuter());

	for (ULevelStreaming* SubLevel : SubLevels)
	{
		RemoveSubLevelFromWorld(SubLevel);
	}

	//remove root level stream
	ensure(UEditorLevelUtils::RemoveLevelFromWorld(LevelStream->GetLoadedLevel()));

	//cleanup empty folder by rebooting worldbrowser module
	WorldModel.Reset();
	WBModule.ShutdownModule();
	WBModule.StartupModule();

	WBModule.OnBrowseWorld.Broadcast(GEditor->GetEditorWorldContext().World());


	FEditorDelegates::RefreshLevelBrowser.Broadcast();
	GameLevels.Remove(LevelStream);
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

FReply FEditorWindowModule::GenerateButtonClicked()
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

	const TMap<ULevelStreaming*, TSet<ULevelStreaming*>> InstancedGameLevels = TMap<ULevelStreaming*, TSet<ULevelStreaming*>>(GameLevels);
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
		for (TPair<ULevelStreaming*, TSet<ULevelStreaming*>> InstancedLevel : InstancedGameLevels)
		{
			//if the level was removed manually before gerenation
			if (InstancedLevel.Key->GetCurrentState() == ULevelStreaming::ECurrentState::Removed ||
				InstancedLevel.Key->GetCurrentState() == ULevelStreaming::ECurrentState::Unloaded) {
				GameLevels.Remove(InstancedLevel.Key);

				//remove any replacement levels it might have
				for (TPair<ULevelStreaming*, ULevelStreaming*> ReplacementLevel : ReplacementLevels)
				{
					if (ReplacementLevel.Key == InstancedLevel.Key)
					{
						LevelsToRemove.Add(ReplacementLevel.Value);
						ReplacementLevels.Remove(ReplacementLevel);

						break;
					}
				}

				continue;
			}

			// Find the instanced level by name
			FString InstancedLevelPath = ClearPathFormatting(InstancedLevel.Key->GetLoadedLevel()->GetOuter()->GetPathName());
			FString DataTableLevelPath = ClearPathFormatting(RowWorld->GetPathName());
			if (InstancedLevelPath == DataTableLevelPath)
			{
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
				
				// Create new replacement level
				int32 RandomNumer = RandomStream.RandRange(0, RowReplaceWorlds.Num() - 1);

				UWorld* ReplaceWorld = RowReplaceWorlds.Array()[RandomNumer].LoadSynchronous();
				ULevelStreaming* RootLevelStream = LoadFullLevel(ReplaceWorld, InstancedLevel.Key->LevelTransform);

				TPair<ULevelStreaming*, ULevelStreaming*> Pair;
				Pair.Key = InstancedLevel.Key;
				Pair.Value = RootLevelStream;
				ReplacementLevels.Add(Pair);
			}
		}

		//remove all levels
		for (ULevelStreaming* LevelToRemove : LevelsToRemove)
		{
			UnloadFullLevel(LevelToRemove);
		}

		GEditor->ForceGarbageCollection(true);
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
