#include "CopilotPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"

void SCopilotPanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SBorder)
        .Padding(16)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                SNew(STextBlock)
                .Text(FText::FromString("AI GameDev Copilot - placeholder UI"))
                .AutoWrapText(true)
            ]
        ]
    ];
}
