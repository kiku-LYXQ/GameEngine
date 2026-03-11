#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"

struct FCopilotRequestPayload
{
    FString Endpoint;
    FString Payload;
    FString ChunkId;
    FString RequestId;
    TMap<FString, FString> Metadata;
};

class FCopilotHttpClient
{
public:
    static void PostRequest(const FCopilotRequestPayload& Payload);
};
