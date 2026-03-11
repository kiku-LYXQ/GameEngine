#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

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
    void RequestCapabilities() const;
    void UpdateCapabilities(const TArray<TSharedPtr<FAgentCapabilityRecord>>& NewItems);
    TSharedRef<ITableRow> OnGenerateCapabilityRow(TSharedPtr<FAgentCapabilityRecord> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

    void RequestMetrics() const;
    void UpdateMetricsText(const FString& Text);

    void RequestNpcSample() const;
    void UpdateBehaviorPlan(const TMap<FString, FString>& Plan);
    TSharedRef<ITableRow> OnGenerateBehaviorRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

    void AppendLog(const FString& Entry);
    TSharedRef<ITableRow> OnGenerateLogRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

    void UpdateCopilotResult(const FString& Summary, const TArray<TSharedPtr<FString>>& Files);
    TSharedRef<ITableRow> OnGenerateCopilotFileRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

    void HandleCopilotResponse(FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess);

    FReply OnSendPromptClicked() const;
    FReply OnResourceCardClicked(FString ChunkPath, FString ChunkId);

private:
    FString SelectedChunkId;
    TSharedPtr<SEditableTextBox> PromptInput;
    TWeakPtr<SListView<TSharedPtr<FAgentCapabilityRecord>>> CapabilityListView;
    TWeakPtr<SListView<TSharedPtr<FString>>> LogListView;
    TWeakPtr<SListView<TSharedPtr<FString>>> BehaviorListView;
    TWeakPtr<SListView<TSharedPtr<FString>>> CopilotFileListView;
    TWeakPtr<STextBlock> CopilotSummaryText;
    TWeakPtr<STextBlock> MetricsText;
    TArray<TSharedPtr<FAgentCapabilityRecord>> CapabilityItems;
    TArray<TSharedPtr<FString>> LogEntries;
    TArray<TSharedPtr<FString>> BehaviorEntries;
    TArray<TSharedPtr<FString>> CopilotFileEntries;
};
