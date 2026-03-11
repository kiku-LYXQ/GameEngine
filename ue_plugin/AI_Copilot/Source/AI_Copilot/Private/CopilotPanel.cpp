#include "CopilotPanel.h"
#include "CopilotHttpClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpManager.h"
#include "Logging/LogMacros.h"
#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "Json.h"
#include "Misc/Guid.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogCopilotPanel, Log, All);

void SCopilotPanel::Construct(const FArguments& InArgs)
{
    const TArray<FString> Templates = {TEXT("Sprint Ability"), TEXT("Burst Damage"), TEXT("AI Patrol"), TEXT("Dialogue Event")};
    const TArray<FString> Resources = {TEXT("Content/VFX/explosion_fx"), TEXT("Blueprints/BP_Player"), TEXT("Materials/M_Spirit"), TEXT("Sound/Ability/impact")};

    RequestCapabilities();
    RequestMetrics();
    RequestNpcSample();

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
                        SNew(STextBlock)
                        .Text(FText::FromString(FString::Join(Templates, TEXT(" | "))))
                        .AutoWrapText(true)
                    ]
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                [
                    SAssignNew(PromptInput, SEditableTextBox)
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
                        .Text(FText::FromString(TEXT("Chunk follow-up: select a chunk and it auto-appends to the prompt.")))
                        .AutoWrapText(true)
                    ]
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Agent Capabilities")))
                    .Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
                [
                    SAssignNew(MetricsText, STextBlock)
                    .Text(FText::FromString(TEXT("Metrics loading...")))
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SAssignNew(CapabilityListView, SListView<TSharedPtr<FAgentCapabilityRecord>>)
                    .ListItemsSource(&CapabilityItems)
                    .SelectionMode(ESelectionMode::None)
                    .OnGenerateRow(this, &SCopilotPanel::OnGenerateCapabilityRow)
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SNew(SExpandableArea)
                    .InitiallyCollapsed(false)
                    .HeaderContent()
                    [
                        SNew(STextBlock).Text(FText::FromString(TEXT("NPC Behavior Plan"))).Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
                    ]
                    .BodyContent()
                    [
                        SAssignNew(BehaviorListView, SListView<TSharedPtr<FString>>)
                        .ListItemsSource(&BehaviorEntries)
                        .SelectionMode(ESelectionMode::None)
                        .OnGenerateRow(this, &SCopilotPanel::OnGenerateBehaviorRow)
                    ]
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SNew(SExpandableArea)
                    .InitiallyCollapsed(true)
                    .HeaderContent()
                    [
                        SNew(STextBlock).Text(FText::FromString(TEXT("Copilot Output Preview"))).Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
                    ]
                    .BodyContent()
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(2)
                        [
                            SAssignNew(CopilotSummaryText, STextBlock)
                            .Text(FText::FromString(TEXT("No result yet")))
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(2)
                        [
                            SAssignNew(CopilotFileListView, SListView<TSharedPtr<FString>>)
                            .ListItemsSource(&CopilotFileEntries)
                            .SelectionMode(ESelectionMode::None)
                            .OnGenerateRow(this, &SCopilotPanel::OnGenerateCopilotFileRow)
                        ]
                    ]
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SNew(SExpandableArea)
                    .InitiallyCollapsed(false)
                    .HeaderContent()
                    [
                        SNew(STextBlock).Text(FText::FromString(TEXT("Execution Logs"))).Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
                    ]
                    .BodyContent()
                    [
                        SAssignNew(LogListView, SListView<TSharedPtr<FString>>)
                        .ListItemsSource(&LogEntries)
                        .SelectionMode(ESelectionMode::None)
                        .OnGenerateRow(this, &SCopilotPanel::OnGenerateLogRow)
                    ]
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Resource & Chunk Cards")))
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
                            SNew(SButton)
                            .Text(FText::FromString(Resources[0]))
                            .OnClicked(this, &SCopilotPanel::OnResourceCardClicked, Resources[0], TEXT("chunk-001"))
                        ]
                    ]
                    + SUniformGridPanel::Slot(1, 0)
                    [
                        SNew(SBorder)
                        .Padding(8)
                        [
                            SNew(SButton)
                            .Text(FText::FromString(Resources[1]))
                            .OnClicked(this, &SCopilotPanel::OnResourceCardClicked, Resources[1], TEXT("chunk-002"))
                        ]
                    ]
                    + SUniformGridPanel::Slot(0, 1)
                    [
                        SNew(SBorder)
                        .Padding(8)
                        [
                            SNew(SButton)
                            .Text(FText::FromString(Resources[2]))
                            .OnClicked(this, &SCopilotPanel::OnResourceCardClicked, Resources[2], TEXT("chunk-003"))
                        ]
                    ]
                    + SUniformGridPanel::Slot(1, 1)
                    [
                        SNew(SBorder)
                        .Padding(8)
                        [
                            SNew(SButton)
                            .Text(FText::FromString(Resources[3]))
                            .OnClicked(this, &SCopilotPanel::OnResourceCardClicked, Resources[3], TEXT("chunk-004"))
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

void SCopilotPanel::RequestCapabilities()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://127.0.0.1:7000/agents/capabilities"));
    Request->SetVerb(TEXT("GET"));
    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess) {
        if (!bSuccess || !Resp.IsValid())
        {
            AppendLog(TEXT("Failed to fetch agent capabilities"));
            return;
        }

        TArray<TSharedPtr<FAgentCapabilityRecord>> Parsed;
        const FString Payload = Resp->GetContentAsString();
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Payload);
        TSharedPtr<FJsonObject> Root;
        if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* Arr;
            if (Root->TryGetArrayField(TEXT("capabilities"), Arr))
            {
                for (const auto& Value : *Arr)
                {
                    if (const TSharedPtr<FJsonObject> Obj = Value->AsObject())
                    {
                        TSharedPtr<FAgentCapabilityRecord> Record = MakeShared<FAgentCapabilityRecord>();
                        Record->Name = Obj->GetStringField(TEXT("name"));
                        Record->SuccessRate = Obj->GetNumberField(TEXT("success_rate"));
                        Record->LatencyMs = Obj->GetIntegerField(TEXT("avg_latency_ms"));
                        Record->Tokens = Obj->GetIntegerField(TEXT("avg_tokens"));
                        Parsed.Add(Record);
                    }
                }
            }
        }

        if (Parsed.Num() == 0)
        {
            Parsed.Add(MakeShared<FAgentCapabilityRecord>(FAgentCapabilityRecord{TEXT("Planner"), 0.9f, 180, 80}));
        }

        AsyncTask(ENamedThreads::GameThread, [this, Parsed = MoveTemp(Parsed)]() mutable {
            UpdateCapabilities(Parsed);
            AppendLog(TEXT("Fetched agent capabilities"));
        });
    });
    Request->ProcessRequest();
}

void SCopilotPanel::UpdateCapabilities(const TArray<TSharedPtr<FAgentCapabilityRecord>>& NewItems)
{
    CapabilityItems = NewItems;
    if (CapabilityListView.IsValid())
    {
        CapabilityListView->RequestListRefresh();
    }
}

TSharedRef<ITableRow> SCopilotPanel::OnGenerateCapabilityRow(TSharedPtr<FAgentCapabilityRecord> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
    return SNew(STableRow<TSharedPtr<FAgentCapabilityRecord>>, OwnerTable)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(4)
        [
            SNew(SProgressBar)
            .Percent(Item.IsValid() ? Item->SuccessRate : 0.f)
            .FillColorAndOpacity(FLinearColor::Green)
            .Thickness(8.f)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(8)
        [
            SNew(STextBlock)
            .Text_Lambda([Item]() {
                if (!Item.IsValid())
                {
                    return FText::FromString(TEXT("Unknown"));
                }
                return FText::FromString(FString::Printf(TEXT("%s · %.0f%% · %dms · %d tokens"), *Item->Name, Item->SuccessRate * 100.f, Item->LatencyMs, Item->Tokens));
            })
        ]
    ];
}

void SCopilotPanel::RequestMetrics()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://127.0.0.1:7001/metrics"));
    Request->SetVerb(TEXT("GET"));
    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess) {
        if (!bSuccess || !Resp.IsValid())
        {
            UpdateMetricsText(TEXT("Metrics unavailable"));
            return;
        }
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());
        TSharedPtr<FJsonObject> Root;
        if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        {
            UpdateMetricsText(TEXT("Metrics parse error"));
            return;
        }
        FString Summary;
        for (const auto& Pair : Root->Values)
        {
            if (Pair.Value.IsValid() && Pair.Value->Type == EJson::Number)
            {
                Summary += FString::Printf(TEXT("%s=%.2f "), *Pair.Key, Pair.Value->AsNumber());
            }
        }
        UpdateMetricsText(Summary);
    });
    Request->ProcessRequest();
}

void SCopilotPanel::UpdateMetricsText(const FString& Text)
{
    if (MetricsText.IsValid())
    {
        MetricsText->SetText(FText::FromString(Text));
    }
}

void SCopilotPanel::RequestNpcSample()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://127.0.0.1:7000/agents/npc/task"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(TEXT("{\"npc_id\":\"npc_01\",\"behavior\":\"patrol\",\"intent\":\"guard the gate\",\"chunk_id\":\"chunk-001\"}"));
    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess) {
        if (!bSuccess || !Resp.IsValid())
        {
            AppendLog(TEXT("NPC sample request failed"));
            return;
        }
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());
        TSharedPtr<FJsonObject> Root;
        if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        {
            AppendLog(TEXT("NPC sample parse failed"));
            return;
        }
        const TSharedPtr<FJsonObject>* BehaviorPlan = nullptr;
        if (Root->TryGetObjectField(TEXT("behavior_plan"), BehaviorPlan))
        {
            UpdateBehaviorPlan(*BehaviorPlan);
        }
        const TArray<TSharedPtr<FJsonValue>>* Dialogue;
        if (Root->TryGetArrayField(TEXT("dialogue"), Dialogue) && Dialogue->Num())
        {
            AppendLog(TEXT("NPC dialogue generated"));
        }
    });
    Request->ProcessRequest();
}

void SCopilotPanel::UpdateBehaviorPlan(const TSharedPtr<FJsonObject>& Plan)
{
    BehaviorEntries.Empty();
    if (!Plan.IsValid())
    {
        if (BehaviorListView.IsValid())
        {
            BehaviorListView->RequestListRefresh();
        }
        return;
    }
    for (const auto& Entry : Plan->Values)
    {
        FString ValueString = TEXT("<empty>");
        if (Entry.Value.IsValid())
        {
            switch (Entry.Value->Type)
            {
            case EJson::String:
                ValueString = Entry.Value->AsString();
                break;
            case EJson::Number:
                ValueString = FString::Printf(TEXT("%.2f"), Entry.Value->AsNumber());
                break;
            default:
                ValueString = TEXT("[complex value]");
                break;
            }
        }
        BehaviorEntries.Add(MakeShared<FString>(FString::Printf(TEXT("%s → %s"), *Entry.Key, *ValueString)));
    }
    if (BehaviorListView.IsValid())
    {
        BehaviorListView->RequestListRefresh();
    }
}

TSharedRef<ITableRow> SCopilotPanel::OnGenerateBehaviorRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
    return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
    [
        SNew(STextBlock)
        .Text_Lambda([Item]() {
            return Item.IsValid() ? FText::FromString(*Item) : FText::FromString(TEXT("Behavior detail"));
        })
    ];
}

void SCopilotPanel::AppendLog(const FString& Entry)
{
    LogEntries.Add(MakeShared<FString>(Entry));
    if (LogListView.IsValid())
    {
        LogListView->RequestListRefresh();
    }
}

TSharedRef<ITableRow> SCopilotPanel::OnGenerateLogRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
    return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
    [
        SNew(STextBlock)
        .Text_Lambda([Item]() {
            return Item.IsValid() ? FText::FromString(*Item) : FText::FromString(TEXT("Log entry"));
        })
    ];
}

void SCopilotPanel::UpdateCopilotResult(const FString& Summary, const TArray<TSharedPtr<FString>>& Files)
{
    CopilotFileEntries = Files;
    if (CopilotFileListView.IsValid())
    {
        CopilotFileListView->RequestListRefresh();
    }
    if (CopilotSummaryText.IsValid())
    {
        CopilotSummaryText->SetText(FText::FromString(Summary));
    }
}

TSharedRef<ITableRow> SCopilotPanel::OnGenerateCopilotFileRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
    return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
    [
        SNew(STextBlock)
        .Text_Lambda([Item]() {
            return Item.IsValid() ? FText::FromString(FString::Printf(TEXT("File: %s"), **Item)) : FText::FromString(TEXT("Generated file"));
        })
    ];
}

FReply SCopilotPanel::OnSendPromptClicked()
{
    if (!PromptInput.IsValid())
    {
        return FReply::Handled();
    }

    const FString Prompt = PromptInput->GetText().ToString();
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("prompt"), Prompt);
    Root->SetStringField(TEXT("schema"), TEXT("code"));
    TSharedPtr<FJsonObject> Context = MakeShared<FJsonObject>();
    Context->SetStringField(TEXT("chunk_id"), SelectedChunkId);
    Root->SetObjectField(TEXT("context"), Context);
    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://127.0.0.1:7000/api/copilot/generate"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(Content);
    Request->OnProcessRequestComplete().BindSP(this, &SCopilotPanel::HandleCopilotResponse);

    Request->ProcessRequest();

    AppendLog(FString::Printf(TEXT("Sent copilot prompt (chunk %s)"), *SelectedChunkId));
    return FReply::Handled();
}

void SCopilotPanel::HandleCopilotResponse(FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
{
    if (!bSuccess || !Resp.IsValid() || Resp->GetResponseCode() != 200)
    {
        AppendLog(TEXT("Copilot response failed"));
        return;
    }
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        AppendLog(TEXT("Copilot response parse error"));
        return;
    }
    FString Summary = Root->GetStringField(TEXT("summary"));
    TArray<TSharedPtr<FString>> Files;
    const TArray<TSharedPtr<FJsonValue>>* FileArray;
    if (Root->TryGetArrayField(TEXT("files"), FileArray))
    {
        for (const auto& Value : *FileArray)
        {
            if (const TSharedPtr<FJsonObject> Obj = Value->AsObject())
            {
                Files.Add(MakeShared<FString>(Obj->GetStringField(TEXT("path"))));
            }
        }
    }
    AsyncTask(ENamedThreads::GameThread, [this, Summary = MoveTemp(Summary), Files = MoveTemp(Files)]() mutable {
        UpdateCopilotResult(Summary, Files);
        AppendLog(TEXT("Copilot rendered result"));
    });
}

FReply SCopilotPanel::OnResourceCardClicked(FString ChunkPath, FString ChunkId)
{
    SelectedChunkId = ChunkId;
    if (PromptInput.IsValid())
    {
        PromptInput->SetText(FText::FromString(FString::Printf(TEXT("Follow up on %s"), *ChunkPath)));
    }
    AppendLog(FString::Printf(TEXT("Selected chunk: %s (%s)"), *ChunkId, *ChunkPath));
    return FReply::Handled();
}
