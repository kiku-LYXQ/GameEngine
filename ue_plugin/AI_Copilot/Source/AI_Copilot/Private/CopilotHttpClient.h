#pragma once

#include "CoreMinimal.h"

struct FCopilotRequestPayload
{
    FString Endpoint;
    FString Payload;
};

class FCopilotHttpClient
{
public:
    static void PostRequest(const FCopilotRequestPayload& Payload);
};
