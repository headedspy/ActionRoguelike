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
#include "Components/SplineComponent.h"
#include "ProceduralRoom.h"
#include "ToolMenus.h"
#include "Containers/StringFwd.h"
#include "PropertyCustomizationHelpers.h"

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
TArray< TSharedPtr< FString > > ComboItems;

TSharedRef<SDockTab> FEditorWindowModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::FromString(TEXT("One"));

	ComboItems.Add(MakeShareable(new FString(TEXT("One"))));
	ComboItems.Add(MakeShareable(new FString(TEXT("Two"))));
	ComboItems.Add(MakeShareable(new FString(TEXT("Three"))));
	ComboItems.Add(MakeShareable(new FString(TEXT("Four"))));
	ComboItems.Add(MakeShareable(new FString(TEXT("Five"))));
	ComboItems.Add(MakeShareable(new FString(TEXT("Six"))));
	ComboItems.Add(MakeShareable(new FString(TEXT("Seven"))));
	ComboItems.Add(MakeShareable(new FString(TEXT("Eight"))));
	ComboItems.Add(MakeShareable(new FString(TEXT("Nine"))));
	ComboItems.Add(MakeShareable(new FString(TEXT("Ten"))));

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			/*
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
			*/
			SNew(SScrollBox)
			+SScrollBox::Slot()
			.Padding(10,5)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Text(WidgetText)
				]

				+SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(WidgetText)
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(WidgetText)
				]
			]
			+SScrollBox::Slot()
			.Padding(10,5)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(2)
				[
					SNew(STextBlock)
					.Text(WidgetText)
				]
				
				+SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(STextBlock)
					.Text(WidgetText)
				]

				+SHorizontalBox::Slot()
				.FillWidth(3)
				[
					SNew(STextBlock)
					.Text(WidgetText)
				]
			]
			+ SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(2)
				[
					SNew(SCheckBox)
					.OnCheckStateChanged_Raw(this, &FEditorWindowModule::ChangedState)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SComboBox<TSharedPtr<FString> >)
					.OptionsSource(&ComboItems)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
					{
						return SNew(STextBlock).Text(FText::FromString(*Item));
					})
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InSelection, ESelectInfo::Type InSelectInfo)
					{
						if (InSelection.IsValid() && ComboBoxTitleBlock.IsValid())
						{
							ComboBoxTitleBlock->SetText(FText::FromString(*InSelection));
						}
					})
					[
						SAssignNew(ComboBoxTitleBlock, STextBlock).Text(LOCTEXT("ComboLabel", "Label"))
					]
				]
			]
			+SScrollBox::Slot()
			.Padding(10, 5)
			[
				SNew(SButton)
				.Text(FText::FromString("Create room"))
				.HAlign(HAlign_Center)
				.OnClicked_Raw(this, &FEditorWindowModule::ButtonClicked)
			]
			+SScrollBox::Slot()
			.Padding(10,5)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UStaticMesh::StaticClass())
				.DisplayBrowse(true)
				.DisplayThumbnail(true)
				.DisplayUseSelected(true)
				.EnableContentPicker(true)
				.AllowClear(false)
				.OnObjectChanged_Lambda([this](const FAssetData& Data) {
					if (Data.IsValid())
					{
						FloorObjectPath = Data.ObjectPath.ToString();
					}
				})
				.ObjectPath_Lambda([this]() { return FloorObjectPath; })
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

void FEditorWindowModule::ChangedState(ECheckBoxState newState)
{
	FText DialogText = FText::FromString("Changed state");
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);
}

AActor* FEditorWindowModule::AddActor(TSubclassOf<AActor> ActorClass, FTransform Transform)
{
	ULevel* Level = GEditor->GetEditorWorldContext().World()->GetCurrentLevel();
	return GEditor->AddActor(Level, ActorClass, Transform);
}


FReply FEditorWindowModule::ButtonClicked()
{
	FText DialogText = FText::FromString("Button Clicked");
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);

	FTransform Transform;

	AProceduralRoom* Room = Cast<AProceduralRoom>(AddActor(AProceduralRoom::StaticClass(), Transform));

	//TODO: error handing if mesh not found
	Room->FloorMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *FloorObjectPath));

	Room->InitializeGraph();

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEditorWindowModule, EditorWindow)