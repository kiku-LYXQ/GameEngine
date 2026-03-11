#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SCopilotPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCopilotPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void RequestCapabilities() const;
    FReply OnSendPromptClicked() const;
    FReply OnResourceCardClicked(FString ChunkPath, FString ChunkId);

private:
    FString SelectedChunkId;
};
