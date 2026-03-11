#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Views/SListView.h"

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
    FReply OnSendPromptClicked() const;
    FReply OnResourceCardClicked(FString ChunkPath, FString ChunkId);

private:
    FString SelectedChunkId;
    TSharedPtr<SEditableTextBox> PromptInput;
    TArray<TSharedPtr<FAgentCapabilityRecord>> CapabilityItems;
    TSharedPtr<SListView<TSharedPtr<FAgentCapabilityRecord>>> CapabilityListView;
};
