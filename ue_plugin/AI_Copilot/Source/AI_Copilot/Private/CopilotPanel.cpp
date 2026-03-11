#include "CopilotPanel.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SVerticalBox.h"
#include "Widgets/Layout/SHorizontalBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SSeparator.h"

void SCopilotPanel::Construct(const FArguments& InArgs)
{
    const TArray<FString> Templates = {TEXT("Sprint Ability"), TEXT("Burst Damage"), TEXT("AI Patrol"), TEXT("Dialogue Event")};
    const TArray<FString> Resources = {TEXT("Content/VFX/explosion_fx"), TEXT("Blueprints/BP_Player"), TEXT("Materials/M_Spirit"), TEXT("Sound/Ability/impact")};

    ChildSlot
    [
        SNew(SBorder)
        .Padding(16)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("AI Copilot · Templates & Resources")))
                    .Font(FCoreStyle::Get().GetFontStyle("HeadingExtraSmall"))
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                [
                    SNew(SWrapBox)
                    + SWrapBox::Slot().Padding(4)
                    [
                        SNew(STextBlock).Text(FText::FromString(TEXT("Select a template to prefill the prompt")))
                    ]
                    + SWrapBox::Slot().Padding(4)
                    [
                        SNew(STextBlock).Text(FText::FromString(TEXT("(Templates interactively expand AND attach context)")))
                    ]
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SNew(SWrapBox)
                    .UseAllottedSize(true)
                    + SWrapBox::Slot().Padding(4)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Prompt Templates")))
                        .Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
                    ]
                    + SWrapBox::Slot().Padding(4)
                    [
                        SNew(SSeparator)
                    ]
                    + SWrapBox::Slot().Padding(4)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(FString::Join(Templates, TEXT(" | "))))
                        .AutoWrapText(true)
                    ]
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                [
                    SNew(SEditableTextBox)
                    .HintText(FText::FromString(TEXT("Describe the behavior you want (e.g. Player sprint ability).")))
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Send to Copilot")))
                        .OnClicked(this, &SCopilotPanel::OnSendPromptClicked)
                    ]
                    + SHorizontalBox::Slot().AutoWidth()
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Chunk follow-up: select a chunk in the list and it auto-appends to the prompt.")))
                        .AutoWrapText(true)
                    ]
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Resource Cards")))
                    .Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                [
                    SNew(SUniformGridPanel)
                    .SlotPadding(FMargin(6.0f))
                    + SUniformGridPanel::Slot(0, 0)
                    [
                        SNew(SBorder)
                        .Padding(8)
                        [
                            SNew(STextBlock).Text(FText::FromString(Resources[0]))
                        ]
                    ]
                    + SUniformGridPanel::Slot(1, 0)
                    [
                        SNew(SBorder)
                        .Padding(8)
                        [
                            SNew(STextBlock).Text(FText::FromString(Resources[1]))
                        ]
                    ]
                    + SUniformGridPanel::Slot(0, 1)
                    [
                        SNew(SBorder)
                        .Padding(8)
                        [
                            SNew(STextBlock).Text(FText::FromString(Resources[2]))
                        ]
                    ]
                    + SUniformGridPanel::Slot(1, 1)
                    [
                        SNew(SBorder)
                        .Padding(8)
                        [
                            SNew(STextBlock).Text(FText::FromString(Resources[3]))
                        ]
                    ]
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Template metadata and chunk context flow to Agent + LLM Runtime, enabling follow-up prompts & resource focus.")))
                    .AutoWrapText(true)
                ]
            ]
        ]
    ];
}

FReply SCopilotPanel::OnSendPromptClicked() const
{
    // Placeholder for RPC to Copilot HTTP client.
    return FReply::Handled();
}
