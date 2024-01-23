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
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "GeneratorActor.h"
#include "GeneratorScript.h"
#include <Kismet/KismetMathLibrary.h>
#include "Widgets/Layout/SBorder.h"

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
	FEditorDelegates::MapChange.AddRaw(this, &FEditorWindowModule::EditorMapChange);
	FEditorDelegates::OnAddLevelToWorld.AddRaw(this, &FEditorWindowModule::ManualAddLevel);
	FEditorDelegates::RefreshLevelBrowser.AddRaw(this, &FEditorWindowModule::RefreshLevels);

	ComboItems.Add(MakeShareable(new ExecutionMethod(Python)));
	ComboItems.Add(MakeShareable(new ExecutionMethod(Blueprint)));
	ComboItems.Add(MakeShareable(new ExecutionMethod(CPP)));

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SScrollBox)
			+SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SBorder)
				.Padding(0, 0, 0, -100)
				.Content()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.Padding(0, 5)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("LAYOUT GENERATION")))
						.ColorAndOpacity(FColor(255, 255, 255, 40))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.Margin(FMargin(0, -8))
					]
					+ SVerticalBox::Slot()
					.Padding(10, 5)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(5)
						[
							SNew(STextBlock)
							.Margin(FMargin(0,5,0,0))
							.Text(FText::FromString(TEXT("Levels DataTable:")))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(7)
						[
							SNew(SObjectPropertyEntryBox)
							.AllowedClass(UDataTable::StaticClass())
							.AllowCreate(false)
							.DisplayBrowse(true)
							.DisplayThumbnail(true)
							.DisplayUseSelected(true)
							.EnableContentPicker(true)
							.AllowClear(true)
							.OnShouldFilterAsset_Lambda([this](const FAssetData& Data) {
								if (!Data.IsValid()) return true;

								//filter only datatables from the currect struct
								UDataTable* NewDataTable = Cast<UDataTable>(Data.GetAsset());
								if (NewDataTable->RowStruct == FLevelsStruct::StaticStruct()) return false;
					
								return true;
							})
							.OnObjectChanged_Lambda([this](const FAssetData& Data) {
								LevelsDataTablePath = Data.GetObjectPathString();
								LevelsDataTable = Cast<UDataTable>(Data.GetAsset());
							})
							.ObjectPath_Lambda([this]() { return LevelsDataTablePath; })
						]
					]
					+ SVerticalBox::Slot()
					.Padding(10, 20)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(2)
						[
							SNew(SComboBox<TSharedPtr<ExecutionMethod>>)
							.OptionsSource(&ComboItems)
							.OnGenerateWidget_Lambda([](TSharedPtr<ExecutionMethod> Item)
							{
								return SNew(STextBlock).Text(FText::FromString(ExecutionMethodToString(*(Item.Get()))));
							})
							.OnSelectionChanged_Lambda([this](TSharedPtr<ExecutionMethod> InSelection, ESelectInfo::Type InSelectInfo)
							{
								Selection = *(InSelection.Get());
								FString DisplayName = ExecutionMethodToString(*(InSelection.Get()));
								ComboBoxTitleBlock->SetText(FText::FromString(DisplayName));
							})
							.InitiallySelectedItem(ComboItems[0])
							[
								SAssignNew(ComboBoxTitleBlock, STextBlock).Text(LOCTEXT("ComboLabel", "Python"))
							]
						]
						+ SHorizontalBox::Slot()
						.FillWidth(3)
						[
							SNew(SButton)
							.Text(FText::FromString("Execute Script"))
							.HAlign(HAlign_Center)
							.OnClicked_Raw(this, &FEditorWindowModule::ExecuteScriptButtonClicked)
						]
					]
					//python
					+SVerticalBox::Slot()
					.Padding(10, -10, 10, 50)
					[
						SNew(SVerticalBox)
						.RenderTransform_Lambda([this]() {
							if (Selection == ExecutionMethod::Python)
								return FSlateRenderTransform(FScale2D(1.0f), FVector2D(0.f, 0.f));
							else
								return FSlateRenderTransform(FScale2D(0.0f), FVector2D(0.f, 0.f));
						})
						+ SVerticalBox::Slot()
						.Padding(5, -5, 5, 5)
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return FText::FromString(FilePath); })
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(3)
							[
								SNew(SButton)
								.Text(FText::FromString("Open File"))
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								.OnClicked_Raw(this, &FEditorWindowModule::OpenFileButtonClicked)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1)
							[
								SNew(SButton)
								.Text(FText::FromString("Clear"))
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								.OnClicked_Lambda([this]() {
									FilePath = "";
									return FReply::Handled();
								})
							]
						]
					]
					//bp
					+ SVerticalBox::Slot()
					.Padding(10, -20, 10, 50)
					[
						SNew(SHorizontalBox)
						.RenderTransform_Lambda([this]() {
							if (Selection == ExecutionMethod::Blueprint)
								return FSlateRenderTransform(FScale2D(1.0f), FVector2D(0.f, -50.f));
							else
								return FSlateRenderTransform(FScale2D(0.0f), FVector2D(0.f, 0.f));
						})
						+ SHorizontalBox::Slot()
						[
							
							SNew(SObjectPropertyEntryBox)
							.AllowedClass(UBlueprint::StaticClass())
							.AllowCreate(false)
							.DisplayBrowse(true)
							.DisplayThumbnail(true)
							.DisplayUseSelected(true)
							.EnableContentPicker(true)
							.AllowClear(true)
							.OnShouldFilterAsset_Lambda([this](const FAssetData& Data) {
								if (!Data.IsValid()) return true;

								// filter only classes implementing the generatorscript interface
								UObject* Object = Data.GetAsset();
								UBlueprint* BP = Cast<UBlueprint>(Object);
								UClass* BPClass = BP->ParentClass;
								if (BPClass == AGeneratorActor::StaticClass()) {
									return false;
								}

								return true;
							})
							.OnObjectChanged_Lambda([this](const FAssetData& Data) {
								GeneratorBPPath = Data.GetObjectPathString();
								GeneratorBP = Cast<AGeneratorActor>(Cast<UBlueprint>(Data.GetAsset())->ParentClass->GetDefaultObject());
							})
							.ObjectPath_Lambda([this]() { return GeneratorBPPath; })
						]
					]
					//cpp
					+ SVerticalBox::Slot()
					.Padding(10, -40, 10, 50)
					[
						SNew(SHorizontalBox)
						.RenderTransform_Lambda([this]() {
							if (Selection == ExecutionMethod::CPP)
								return FSlateRenderTransform(FScale2D(1.0f), FVector2D(0.f, -80.f));
							else
								return FSlateRenderTransform(FScale2D(0.0f), FVector2D(0.f, 0.f));
						})
						+ SHorizontalBox::Slot()
						[
							SNew(SObjectPropertyEntryBox)
							.AllowedClass(UClass::StaticClass())
							.AllowCreate(false)
							.DisplayBrowse(true)
							.DisplayThumbnail(true)
							.DisplayUseSelected(true)
							.EnableContentPicker(true)
							.AllowClear(true)
							.OnShouldFilterAsset_Lambda([this](const FAssetData& Data) {
								if (!Data.IsValid()) return true;

								// filter only classes implementing the generatorscript interface
								UClass* Class = Cast<UClass>(Data.GetAsset());
								if (Class->ImplementsInterface(UGeneratorScript::StaticClass())) {
									return false;
								}

								return true;
							})
							.OnObjectChanged_Lambda([this](const FAssetData& Data) {
								GeneratorClassPath = Data.GetObjectPathString();
								GeneratorClass = Cast<UClass>(Data.GetAsset());
							})
							.ObjectPath_Lambda([this]() { return GeneratorClassPath; })
						]
					]
				]
			]
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SBorder)
				.Content()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.Padding(0, 5)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("LEVEL GENERATION")))
						.ColorAndOpacity(FColor(255, 255, 255, 40))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.Margin(FMargin(-2, -10))
					]
					+ SVerticalBox::Slot()
					.Padding(10, 5)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(5)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Levels DataTable:")))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(7)
						[
							SNew(SObjectPropertyEntryBox)
							.AllowedClass(UDataTable::StaticClass())
							.AllowCreate(false)
							.DisplayBrowse(true)
							.DisplayThumbnail(true)
							.DisplayUseSelected(true)
							.EnableContentPicker(true)
							.AllowClear(true)
							.OnShouldFilterAsset_Lambda([this](const FAssetData& Data) {
								if (!Data.IsValid()) return true;

								//filter only datatables from the currect struct
								UDataTable* NewDataTable = Cast<UDataTable>(Data.GetAsset());
								if (NewDataTable->RowStruct == FLevelsStruct::StaticStruct()) return false;
					
								return true;
							})
							.OnObjectChanged_Lambda([this](const FAssetData& Data) {
								LevelsDataTablePath = Data.GetObjectPathString();
								LevelsDataTable = Cast<UDataTable>(Data.GetAsset());
							})
							.ObjectPath_Lambda([this]() { return LevelsDataTablePath; })
						]
					]
					+ SVerticalBox::Slot()
					.Padding(10, 5)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Seed:")))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(3)
						.Padding(1,0,5,0)
						[
							SNew(SEditableTextBox)
							.OnTextChanged_Lambda([this](const FText& NewText) {
								int32 Num = FCString::Atoi(*(NewText.ToString()));
								if (Num == 0 && NewText.ToString() != "")
								{
									FText DialogText = FText::FromString("Input a positive integer!");
									FMessageDialog::Open(EAppMsgType::Ok, DialogText);
									LevelsSeedNum = 0;
								}
								else {
									LevelsSeedNum = Num;
								}
							})
							.Text_Lambda([this]() {
								if (LevelsSeedNum == 0) {
									return (RandomizeLevelsSeed ? FText() : FText::FromString("0"));
								}
								else {
									return FText(FText::FromString(FString::FromInt(LevelsSeedNum)));
								}
							})
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1)
						.Padding(25, 0, 0, 0)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
								RandomizeLevelsSeed = (State == ECheckBoxState::Checked);
							})
						]
						+ SHorizontalBox::Slot()
						.FillWidth(3)
						.HAlign(EHorizontalAlignment::HAlign_Left)
						.Padding(-5,0 ,0, 0)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Randomize?")))
						]
					]
					+ SVerticalBox::Slot()
					.Padding(10, 10)
					[
						SNew(SButton)
						.ContentPadding(FMargin(0))
						.Text(FText::FromString("Replace Levels"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Raw(this, &FEditorWindowModule::GenerateLevelsButtonClicked)
					]
				]
			]
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SBorder)
				.Content()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.Padding(0, 5)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("ACTOR FILTERING")))
						.ColorAndOpacity(FColor(255, 255, 255, 40))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.Margin(FMargin(-1, -10))
					]
					+ SVerticalBox::Slot()
					.Padding(10, 5)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(5)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Tags DataTable:")))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(7)
						[
							SNew(SObjectPropertyEntryBox)
							.AllowCreate(false)
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
							if (NewDataTable->RowStruct == FTagsStruct::StaticStruct()) return false;

								return true;
							})
							.OnObjectChanged_Lambda([this](const FAssetData& Data) {
								TagsDataTablePath = Data.GetObjectPathString();
								TagsDataTable = Cast<UDataTable>(Data.GetAsset());
							})
							.ObjectPath_Lambda([this]() { return TagsDataTablePath; })
						]
					]
					+ SVerticalBox::Slot()
					.Padding(10, 5)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Seed:")))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(3)
						.Padding(1,0,5,0)
						[
							SNew(SEditableTextBox)
							.OnTextChanged_Lambda([this](const FText& NewText) {
								int32 Num = FCString::Atoi(*(NewText.ToString()));
								if (Num == 0 && NewText.ToString() != "")
								{
									FText DialogText = FText::FromString("Input a positive integer!");
									FMessageDialog::Open(EAppMsgType::Ok, DialogText);
									TagsSeedNum = 0;
								}
								else {
									TagsSeedNum = Num;
								}
							})
							.Text_Lambda([this]() {
								if (TagsSeedNum == 0) {
									return (RandomizeTagsSeed ? FText() : FText::FromString("0"));
								}
								else {
									return FText(FText::FromString(FString::FromInt(TagsSeedNum)));
								}
							})
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1)
						.Padding(25, 0, 0, 0)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
								RandomizeTagsSeed = (State == ECheckBoxState::Checked);
							})
						]
						+ SHorizontalBox::Slot()
						.FillWidth(3)
						.HAlign(EHorizontalAlignment::HAlign_Left)
						.Padding(-5,0 ,0, 0)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Randomize?")))
						]
					]
					+ SVerticalBox::Slot()
					.Padding(10, 10)
					[
						SNew(SButton)
						.ContentPadding(FMargin(0))
						.Text(FText::FromString("Filter Actors"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Raw(this, &FEditorWindowModule::GenerateTagsButtonClicked)
					]
				]
			]
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SBorder)
				.Content()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.Padding(0, 5)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("ACTOR GENERATION")))
						.ColorAndOpacity(FColor(255, 255, 255, 40))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.Margin(FMargin(-1, -10))
					]
					+ SVerticalBox::Slot()
					.Padding(10, 5)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(5)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Actors DataTable:")))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(7)
						[
							SNew(SObjectPropertyEntryBox)
							.AllowCreate(false)
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
							if (NewDataTable->RowStruct == FActorsStruct::StaticStruct()) return false;

								return true;
							})
							.OnObjectChanged_Lambda([this](const FAssetData& Data) {
								ActorsDataTablePath = Data.GetObjectPathString();
								ActorsDataTable = Cast<UDataTable>(Data.GetAsset());
							})
							.ObjectPath_Lambda([this]() { return ActorsDataTablePath; })
						]
					]
					+ SVerticalBox::Slot()
					.Padding(10, 5)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Seed:")))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(3)
						.Padding(1,0,5,0)
						[
							SNew(SEditableTextBox)
							.OnTextChanged_Lambda([this](const FText& NewText) {
								int32 Num = FCString::Atoi(*(NewText.ToString()));
								if (Num == 0 && NewText.ToString() != "")
								{
									FText DialogText = FText::FromString("Input a positive integer!");
									FMessageDialog::Open(EAppMsgType::Ok, DialogText);
									ActorsSeedNum = 0;
								}
								else {
									ActorsSeedNum = Num;
								}
							})
							.Text_Lambda([this]() {
								if (ActorsSeedNum == 0) {
									return (RandomizeActorsSeed ? FText() : FText::FromString("0"));
								}
								else {
									return FText(FText::FromString(FString::FromInt(ActorsSeedNum)));
								}
							})
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1)
						.Padding(25, 0, 0, 0)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
							RandomizeActorsSeed = (State == ECheckBoxState::Checked);
							})
						]
						+ SHorizontalBox::Slot()
						.FillWidth(3)
						.HAlign(EHorizontalAlignment::HAlign_Left)
						.Padding(-5,0 ,0, 0)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Randomize?")))
						]
					]
					+ SVerticalBox::Slot()
					.Padding(10, 10)
					[
						SNew(SButton)
						.ContentPadding(FMargin(0))
						.Text(FText::FromString("Generate Actors"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Raw(this, &FEditorWindowModule::GenerateActorsButtonClicked)
					]
				]
			]
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SBorder)
				.Content()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.Padding(0, 5)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("MERGING")))
						.ColorAndOpacity(FColor(255, 255, 255, 40))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.Margin(FMargin(-2, -10))
					]
					+ SVerticalBox::Slot()
					.Padding(10, 10)
					[
						SNew(SButton)
						.DesiredSizeScale(FVector2D(1.0f, 1.5f))
						.ContentPadding(FMargin(0))
						.Text(FText::FromString("Merge all actors in Persistent Level"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Raw(this, &FEditorWindowModule::MergeButtonClicked)
					]
				]
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
bool FEditorWindowModule::CheckAndLog(bool Value, FString MessageToLog)
{
	bool Result = Value;
	if (Result)
	{
		FText DialogText = FText::FromString(MessageToLog);
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);
	}
	return Result;
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

		int32 BeginIndex = PathFormat.Find("UEDPIE");
		int32 EndIndex = PathFormat.Find("_", ESearchCase::IgnoreCase, ESearchDir::FromStart, BeginIndex + 7); //UEDPIE_ has 7 characters
		FString Crop = PathFormat.Mid(BeginIndex, EndIndex - BeginIndex + 1);

		PathFormat.ReplaceInline(*(Crop), TEXT(""));
	}

	return PathFormat;
}

void FEditorWindowModule::ManualAddLevel(ULevel* Level)
{
	if (LevelsDataTable == nullptr) return;

	bool LevelExistsInDataTable = false;

	UWorld* AddedLevelWorld = (UWorld*)Level->GetOuter();
	const TMap<FName, uint8*> DataTableRows = LevelsDataTable->GetRowMap();
	TSet<FName> KeysSet;
	int32 NrOfKeys = DataTableRows.GetKeys(KeysSet);

	//check if the level exists in the datatable
	for (int i = 0; i < NrOfKeys; i++)
	{
		FName RowName = KeysSet.Array()[i];
		FLevelsStruct* RowData = LevelsDataTable->FindRow<FLevelsStruct>(RowName, "");

		UWorld* DataTableWorld = RowData->World.LoadSynchronous();

		if (DataTableWorld == AddedLevelWorld)
		{
			LevelExistsInDataTable = true;
			break;
		}
	}

	if (!LevelExistsInDataTable) return;

	//find the level stream
	TArray<ULevelStreaming*> StreamingLevels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();
	for (ULevelStreaming* StreamingLevel : StreamingLevels)
	{
		if (StreamingLevel->GetLoadedLevel() == Level)
		{
			//rename the stream
			StreamingLevel->RenameForPIE(PIELevelNameCounter++);
			break;
		}
	}

	//refresh the level browser
	FEditorDelegates::RefreshLevelBrowser.Broadcast();
}

void FEditorWindowModule::EditorMapChange(uint32 flags)
{
	ReplacementLevels.Empty();
	ReplacementActors.Empty();

	//cleanup empty folders by rebooting worldbrowser module
	FWorldBrowserModule& WBModule = FModuleManager::LoadModuleChecked<FWorldBrowserModule>("WorldBrowser");
	TSharedPtr<FLevelCollectionModel> WorldModel = WBModule.SharedWorldModel(GEditor->GetEditorWorldContext().World());

	WorldModel.Reset();
	WBModule.ShutdownModule();
	WBModule.StartupModule();

	WBModule.OnBrowseWorld.Broadcast(GEditor->GetEditorWorldContext().World());
}

void FEditorWindowModule::RefreshLevels()
{
	if (CurrentlyDeleting) return;

	for (TPair<ULevelStreaming*, TSet<ULevelStreaming*>> Level : ReplacementLevels)
	{
		// remove replacement levels if layout level has been removed manually
		if (Level.Key->GetCurrentState() == ULevelStreaming::ECurrentState::Removed)
		{
			CurrentlyDeleting = true;
			for (ULevelStreaming* ReplacementLevel : Level.Value)
			{
				UnloadFullLevel(ReplacementLevel);
			}

			ReplacementLevels.Remove(Level.Key);
			CurrentlyDeleting = false;
			break;
		}

		// remove replacement levels from the list if they have been removed manually
		TSet<ULevelStreaming*> ReplacementLevels_I(Level.Value);
		for (ULevelStreaming* ReplacementLevel : ReplacementLevels_I)
		{
			if (ReplacementLevel->GetCurrentState() == ULevelStreaming::ECurrentState::Removed)
			{
				ReplacementLevels[Level.Key].Remove(ReplacementLevel);
			}
		}
	}
}

void FEditorWindowModule::MoveAllActorsFromLevel(ULevelStreaming* LevelStream)
{
	TArray<AActor*> Actors = LevelStream->GetLoadedLevel()->Actors;
	// the first two actors of a world are always worldsettings and defualtbrush, which we ignore
	Actors.RemoveAt(0);
	Actors.RemoveAt(0);

	//rename all actors to unique names and move them to the persistent level
	for (AActor* Actor : Actors)
	{
		//if (Actor->IsActorBeingDestroyed()) continue;
		Actor->Rename(*(Actor->GetName() + "_" + LevelStream->GetName()), GEditor->GetEditorWorldContext().World()->PersistentLevel.Get());
		GEditor->GetEditorWorldContext().World()->PersistentLevel.Get()->Actors.Add(Actor);
	}
}

UClass* FEditorWindowModule::WeightedRandomActor(TSet<FWeightedActor> WeightedActors, FRandomStream& RandomStream, uint16 SumOfWeights)
{
	int32 RandomNumer = RandomStream.RandRange(0, SumOfWeights - 1);
	UClass* ReplaceActorClass = nullptr;

	for (FWeightedActor RowReplaceActor : WeightedActors)
	{
		if (RandomNumer < RowReplaceActor.Weight)
		{
			ReplaceActorClass = RowReplaceActor.Actor;
			break;
		}
		RandomNumer -= RowReplaceActor.Weight;
	}
	ensure(ReplaceActorClass);

	return ReplaceActorClass;
}



// BUTTON FUNCTIONS
FReply FEditorWindowModule::GenerateLevelsButtonClicked()
{
	if (CheckAndLog(LevelsDataTable == nullptr, "Levels DataTable is empty!")) return FReply::Handled();

	// Initialize rng with seed
	FRandomStream RandomStream;
	if (RandomizeLevelsSeed) {
		RandomStream.GenerateNewSeed();
		LevelsSeedNum = RandomStream.GetCurrentSeed();
	}
	else {
		RandomStream.Initialize(LevelsSeedNum);
	}

	//clear all reaplacement actors
	for (TPair<AActor*, AActor*> ReplacementActor : ReplacementActors)
	{
		ReplacementActor.Value->Destroy();
		GEditor->ForceGarbageCollection(true);
	}
	ReplacementActors.Empty();

	const TMap<FName, uint8*> DataTableRows = LevelsDataTable->GetRowMap();

	// for each streamed level
	const TArray<ULevelStreaming*> StreamedLevels = GWorld->GetStreamingLevels();
	for(ULevelStreaming* StreamedLevel : StreamedLevels)
	{
		//if the level was already removed, skip
		if (StreamedLevel->GetCurrentState() == ULevelStreaming::ECurrentState::Removed ||
			StreamedLevel->GetCurrentState() == ULevelStreaming::ECurrentState::Unloaded ||
			!IsValid(StreamedLevel)) continue;

		// for each row
		for (TPair<FName, uint8*> Row : DataTableRows)
		{
			// parse row data
			FLevelsStruct* RowData = (FLevelsStruct*)Row.Value;
			RowData->World.LoadSynchronous();
			UWorld* RowWorld = RowData->World.Get();
			TSet<FWeightedWorld> RowReplaceWorlds = RowData->ReplaceWorlds;

			// find the correct row
			FString InstancedLevelPath = ClearPathFormatting(StreamedLevel->GetLoadedLevel()->GetOuter()->GetPathName());
			FString DataTableLevelPath = ClearPathFormatting(RowWorld->GetPathName());
			if (InstancedLevelPath == DataTableLevelPath)
			{
				// calculate the sum of all weights for the level
				uint16 SumOfWeights = 0;
				for (FWeightedWorld ReplaceWorld : RowReplaceWorlds)
				{
					SumOfWeights += ReplaceWorld.Weight;
				}

				//check if all row replacements' weights are zero
				if (CheckAndLog(SumOfWeights == 0,
					"All replacement worlds listed under the '" + DataTableLevelPath + "' row have a weight value of zero. No replacement world will be generated!")) continue;

				// If level already has replacement levels, delete them
				for (TPair<ULevelStreaming*, TSet<ULevelStreaming*>> ReplacementLevel : ReplacementLevels)
				{
					if (ReplacementLevel.Key == StreamedLevel)
					{
						//remove all reaplacement levels
						for (ULevelStreaming* CreatedReplacementLevel : ReplacementLevel.Value)
						{
							UnloadFullLevel(CreatedReplacementLevel);
						}
						ReplacementLevels.Remove(StreamedLevel);

						GEditor->ForceGarbageCollection(true);
						break;
					}
				}


				// Choose new replacement level
				int32 RandomNumer = RandomStream.RandRange(0, SumOfWeights-1);
				UWorld* ReplaceWorld = nullptr;
				for (FWeightedWorld RowReplaceWorld : RowReplaceWorlds)
				{
					if (RandomNumer < RowReplaceWorld.Weight)
					{
						ReplaceWorld = RowReplaceWorld.World.LoadSynchronous();
						break;
					}
					RandomNumer -= RowReplaceWorld.Weight;
				}
				ensure(ReplaceWorld);

				// name the replacement level folder with the same name as the layout level
				TArray<FString> Parse;
				StreamedLevel->GetWorldAssetPackageName().ParseIntoArray(Parse, TEXT("/"));
				FString FolderName = Parse[Parse.Num() - 1];

				TSet<ULevelStreaming*> ReplacementLevelsStreams = LoadFullLevel(ReplaceWorld,
																			    StreamedLevel->LevelTransform,
																			    FolderName,
																			    StreamedLevel->LevelColor);

				TPair<ULevelStreaming*, TSet<ULevelStreaming*>> Pair;
				Pair.Key = StreamedLevel;
				Pair.Value = ReplacementLevelsStreams;
				ReplacementLevels.Add(Pair);

				break;
			}
		}
	}

	return FReply::Handled();
}

FReply FEditorWindowModule::GenerateTagsButtonClicked()
{
	if (CheckAndLog(TagsDataTable == nullptr, "Tags DataTable is empty!")) return FReply::Handled();

	// Initialize rng with seed
	FRandomStream RandomStream;
	if (RandomizeTagsSeed) {
		RandomStream.GenerateNewSeed();
		TagsSeedNum = RandomStream.GetCurrentSeed();
	}
	else {
		RandomStream.Initialize(TagsSeedNum);
	}

	const TMap<FName, uint8*> DataTableRows = TagsDataTable->GetRowMap();
	for (TPair<FName, uint8*> Row : DataTableRows)
	{
		// parse row data
		FTagsStruct* RowData = (FTagsStruct*)Row.Value;

		FName ActorTag = RowData->ActorTag;
		uint8 NumberOfElements = RowData->NumberOfElements;
		UClass* ReplaceActorClass = RowData->ReplaceActor;

		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithTag(GEditor->GetEditorWorldContext().World(), ActorTag, FoundActors);

		if (FoundActors.Num() == 0) continue;

		if (CheckAndLog(FoundActors.Num() < NumberOfElements,
			"Tag '" + ActorTag.ToString() + "' has less than " + FString::FromInt(NumberOfElements) + " actors in the level!")) continue;

		TMap<ULevel*, TArray<AActor*>> ActorsInLevels;

		for (AActor* FoundActor : FoundActors)
		{
			ULevel* Level = FoundActor->GetLevel();
			TArray<AActor*>& Array = ActorsInLevels.FindOrAdd(Level);
			Array.Add(FoundActor);
		}

		for (TPair<ULevel*, TArray<AActor*>> ActorsInLevel : ActorsInLevels)
		{
			TArray<AActor*> FoundActorsInLevel = ActorsInLevel.Value;

			//remove all replacement actors
			for (AActor* FoundActor : FoundActorsInLevel)
			{
				//check if there's already replacement actors
				TMap<AActor*, AActor*> ReplacementActors_I(ReplacementActors);
				for (TPair<AActor*, AActor*> ReplacementActor : ReplacementActors_I)
				{
					// TODO: check if working! check if actor was removed (via regeneration of level)
					if (!ReplacementActor.Key->IsValidLowLevel())
					{
						ReplacementActors.Remove(ReplacementActor.Key);
						continue;
					}

					if (ReplacementActor.Key == FoundActor)
					{
						//delete the replacement actor
						ReplacementActor.Value->Destroy();

						GEditor->ForceGarbageCollection(true);

						ReplacementActors.Remove(ReplacementActor.Key);
					}

					if (ReplacementActor.Key->GetClass() == ReplaceActorClass)
					{
						//delete its replacement actor
						ReplacementActor.Value->Destroy();

						GEditor->ForceGarbageCollection(true);

						ReplacementActors.Remove(ReplacementActor.Key);
					}
				}
			}

			//filter random number of actors
			uint8 NumberOfActorsToRemove = FoundActorsInLevel.Num() - NumberOfElements;
			for (int i = 0; i < NumberOfActorsToRemove; i++)
			{
				int32 RandomActorToRemoveIndex = RandomStream.RandRange(0, FoundActorsInLevel.Num() - 1);
				FoundActorsInLevel.RemoveAt(RandomActorToRemoveIndex);
			}

			//spawn replacement actor
			for (AActor* FoundActor : FoundActorsInLevel)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.OverrideLevel = FoundActor->GetLevel();
				AActor* ReplacementActor = GEditor->GetEditorWorldContext().World()->SpawnActor<AActor>(ReplaceActorClass, FoundActor->GetTransform(), SpawnParams);

				TPair<AActor*, AActor*> Pair;
				Pair.Key = FoundActor;
				Pair.Value = ReplacementActor;
				ReplacementActors.Add(Pair);
			}
		}
	}

	return FReply::Handled();
}

FReply FEditorWindowModule::GenerateActorsButtonClicked()
{
	if (CheckAndLog(ActorsDataTable == nullptr, "Actors DataTable is empty!")) return FReply::Handled();

	// Initialize rng with seed
	FRandomStream RandomStream;
	if (RandomizeActorsSeed) {
		RandomStream.GenerateNewSeed();
		ActorsSeedNum = RandomStream.GetCurrentSeed();
	}
	else {
		RandomStream.Initialize(ActorsSeedNum);
	}

	TArray<AActor*> FoundActors;

	const TMap<FName, uint8*> DataTableRows = ActorsDataTable->GetRowMap();
	for (TPair<FName, uint8*> Row : DataTableRows)
	{
		// parse row data
		FActorsStruct* RowData = (FActorsStruct*) Row.Value;
		UClass* RowClass = RowData->Actor;
		TSet<FWeightedActor> RowReplaceActors = RowData->ReplaceActors;

		// calculate the sum of weights
		uint16 SumOfWeights = 0;
		for (FWeightedActor ReplaceActor : RowReplaceActors)
		{
			SumOfWeights += ReplaceActor.Weight;
		}

		//check if all row replacements' weights are zero
		if (CheckAndLog(SumOfWeights == 0,
			"All replacement actors listed under the '" + RowData->Actor.GetDefaultObject()->GetName() + "' row have a weight value of zero. No replacement actor will be generated!")) continue;

		UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), RowClass, FoundActors);

		//remove replacement actors
		for (AActor* Actor : FoundActors)
		{
			//check if there's already replacement actors
			TMap<AActor*, AActor*> ReplacementActors_I(ReplacementActors);
			for (TPair<AActor*, AActor*> ReplacementActor : ReplacementActors_I)
			{
				// TODO: check if working! check if actor was removed (via regeneration of level)
				if (!ReplacementActor.Key->IsValidLowLevel())
				{
					ReplacementActors.Remove(ReplacementActor.Key);
					continue;
				}

				if (ReplacementActor.Key == Actor)
				{
					//delete the replacement actor
					ReplacementActor.Value->Destroy();

					GEditor->ForceGarbageCollection(true);

					ReplacementActors.Remove(ReplacementActor.Key);
				}
			}

			// select a replacement actor
			UClass* ReplaceActorClass = WeightedRandomActor(RowReplaceActors, RandomStream, SumOfWeights);

			//spawn the replacement actor
			FActorSpawnParameters SpawnParams;
			SpawnParams.OverrideLevel = Actor->GetLevel();
			AActor* ReplacementActor = GEditor->GetEditorWorldContext().World()->SpawnActor<AActor>(ReplaceActorClass, Actor->GetTransform(), SpawnParams);

			TPair<AActor*, AActor*> Pair;
			Pair.Key = Actor;
			Pair.Value = ReplacementActor;
			ReplacementActors.Add(Pair);
		}

		/*
		if (RowData->bVariableSpawnPoint)
		{
			// for each labelled tag in the datatable
			for (TPair<FName, uint8> ElementTag : RowData->NumberOfElementsInTag)
			{
				TArray<AActor*> TaggedActors;
				//for each actor of same class
				for (AActor* Actor : FoundActors)
				{
					//if the actor has the tag
					if (Actor->ActorHasTag(ElementTag.Key))
					{
						TaggedActors.Add(Actor);
					}
				}

				if (TaggedActors.Num() == 0) continue;
				
				if(CheckAndLog(TaggedActors.Num() < ElementTag.Value,
					"Tag '" + ElementTag.Key.ToString() + "' has less than " + FString::FromInt(ElementTag.Value) + " actors in the level!")) continue;

				//filter random number of actors
				uint8 NumberOfActorsToRemove = TaggedActors.Num() - ElementTag.Value;
				for (int i = 0; i < NumberOfActorsToRemove; i++)
				{
					int32 RandomActorToRemoveIndex = RandomStream.RandRange(0, TaggedActors.Num() - 1);
					TaggedActors.RemoveAt(RandomActorToRemoveIndex);
				}

				//spawn replacement actor
				for (AActor* TaggedActor : TaggedActors)
				{
					// select a replacement actor
					UClass* ReplaceActorClass = WeightedRandomActor(RowReplaceActors, RandomStream, SumOfWeights);

					//spawn the replacement actor
					FActorSpawnParameters SpawnParams;
					SpawnParams.OverrideLevel = TaggedActor->GetLevel();
					AActor* ReplacementActor = GEditor->GetEditorWorldContext().World()->SpawnActor<AActor>(ReplaceActorClass, TaggedActor->GetTransform(), SpawnParams);

					TPair<AActor*, AActor*> Pair;
					Pair.Key = TaggedActor;
					Pair.Value = ReplacementActor;
					ReplacementActors.Add(Pair);
				}
			}
		}
		*/

	}

	return FReply::Handled();
}

FReply FEditorWindowModule::MergeButtonClicked()
{
	// for each streamed level
	const TArray<ULevelStreaming*> StreamedLevels = GWorld->GetStreamingLevels();

	for (TPair<AActor*, AActor*> ReplacementActor : ReplacementActors)
	{
		//ReplacementActor.Key->Destroy();
	}

	for (ULevelStreaming* StreamedLevel : StreamedLevels)
	{
		//move all actors from the level
		MoveAllActorsFromLevel(StreamedLevel);
		UnloadFullLevel(StreamedLevel);
	}

	ReplacementLevels.Empty();
	ReplacementActors.Empty();

	FEditorDelegates::RefreshLevelBrowser.Broadcast();

	return FReply::Handled();
}


FReply FEditorWindowModule::OpenFileButtonClicked()
{
	uint32 flags = 0;
	TArray<FString> OutFilenames;

	// open file selector
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	const void* Handle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	DesktopPlatform->OpenFileDialog(Handle, "Select Python Script", "", "", "Python Script (*.py)|*.py", flags, OutFilenames);

	//if the cancel button was pressed
	if (OutFilenames.Num() == 0)
	{
		return FReply::Handled();
	}

	FilePath = OutFilenames[0];

	return FReply::Handled();
}

FReply FEditorWindowModule::ExecuteScriptButtonClicked()
{
	switch (Selection)
	{
		case(ExecutionMethod::Python): {

			if (CheckAndLog(FilePath.IsEmpty(), "Script file not selected!")) return FReply::Handled();
			if (CheckAndLog(!FilePath.EndsWith(".py"), "Script is not a python file!")) return FReply::Handled();

			FString Command = "py " + FilePath;

			//execute using the console command executor
			TArray<IConsoleCommandExecutor*> CommandExecutors = IModularFeatures::Get().GetModularFeatureImplementations<IConsoleCommandExecutor>(IConsoleCommandExecutor::ModularFeatureName());
			IConsoleCommandExecutor* ActiveCommandExecutor = nullptr;

			ensure(CommandExecutors.IsValidIndex(0));

			ActiveCommandExecutor = CommandExecutors[0];
			ActiveCommandExecutor->Exec(*Command);
			break;
		}
		case(ExecutionMethod::Blueprint): {
			if (CheckAndLog(GeneratorBP == nullptr, "Generator not selected!")) break;

			UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *(GeneratorBPPath));
			UClass* GeneratedClass = BP->GeneratedClass;
			UFunction* Function = GeneratedClass->FindFunctionByName(FName(TEXT("OnGenerateButtonPressed")));
			ensure(Function);
			GeneratedClass->GetDefaultObject(true)->ProcessEvent(Function, nullptr);

			break;
		}
		case(ExecutionMethod::CPP): {
			if (CheckAndLog(GeneratorClass == nullptr, "No script selected")) break;

			UFunction* Function = GeneratorClass->FindFunctionByName(FName(TEXT("OnGenerateButtonPressed")));
			ensure(Function);
			GeneratorClass->GetDefaultObject(true)->ProcessEvent(Function, nullptr);
			break;
		}
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEditorWindowModule, EditorWindow)
