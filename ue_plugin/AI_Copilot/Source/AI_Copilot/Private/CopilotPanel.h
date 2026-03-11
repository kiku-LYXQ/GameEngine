#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

class FJsonObject;

struct FAgentCapabilityRecord
{
    FString Name;
    float SuccessRate = 0.f;
    int32 LatencyMs = 0;
    int32 Tokens = 0;
};

class SCopilotPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCopilotPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void RequestCapabilities();
    void UpdateCapabilities(const TArray<TSharedPtr<FAgentCapabilityRecord>>& NewItems);
    TSharedRef<ITableRow> OnGenerateCapabilityRow(TSharedPtr<FAgentCapabilityRecord> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

    void RequestMetrics();
    void UpdateMetricsText(const FString& Text);

    void RequestNpcSample();
    void UpdateBehaviorPlan(const TSharedPtr<FJsonObject>& Plan);
    TSharedRef<ITableRow> OnGenerateBehaviorRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

    void AppendLog(const FString& Entry);
    TSharedRef<ITableRow> OnGenerateLogRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

    void UpdateCopilotResult(const FString& Summary, const TArray<TSharedPtr<FString>>& Files);
    TSharedRef<ITableRow> OnGenerateCopilotFileRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

    void HandleCopilotResponse(FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess);

    FReply OnSendPromptClicked();
    FReply OnResourceCardClicked(FString ChunkPath, FString ChunkId);

private:
    FString SelectedChunkId;
    TSharedPtr<SEditableTextBox> PromptInput;
    TSharedPtr<SListView<TSharedPtr<FAgentCapabilityRecord>>> CapabilityListView;
    TSharedPtr<SListView<TSharedPtr<FString>>> LogListView;
    TSharedPtr<SListView<TSharedPtr<FString>>> BehaviorListView;
    TSharedPtr<SListView<TSharedPtr<FString>>> CopilotFileListView;
    TSharedPtr<STextBlock> CopilotSummaryText;
    TSharedPtr<STextBlock> MetricsText;
    TArray<TSharedPtr<FAgentCapabilityRecord>> CapabilityItems;
    TArray<TSharedPtr<FString>> LogEntries;
    TArray<TSharedPtr<FString>> BehaviorEntries;
    TArray<TSharedPtr<FString>> CopilotFileEntries;
};
