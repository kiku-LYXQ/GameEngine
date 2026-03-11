#include "CopilotHttpClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Guid.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogCopilotHttp, Log, All);

void FCopilotHttpClient::PostRequest(const FCopilotRequestPayload& Payload)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Payload.Endpoint);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    const FString RequestId = Payload.RequestId.IsEmpty() ? FGuid::NewGuid().ToString() : Payload.RequestId;
    Request->SetHeader(TEXT("X-Copilot-Request-Id"), RequestId);
    if (!Payload.ChunkId.IsEmpty())
    {
        Request->SetHeader(TEXT("X-Copilot-Chunk"), Payload.ChunkId);
    }
    if (Payload.Metadata.Num())
    {
        FString MetadataString;
        for (const auto& Entry : Payload.Metadata)
        {
            MetadataString += Entry.Key + TEXT("=") + Entry.Value + TEXT(";");
        }
        Request->SetHeader(TEXT("X-Copilot-Metadata"), MetadataString);
    }
    Request->SetContentAsString(Payload.Payload);
    Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess) {
        if (!bSuccess || !Resp.IsValid())
        {
            UE_LOG(LogCopilotHttp, Warning, TEXT("Copilot HTTP request failed"));
            return;
        }
        UE_LOG(LogCopilotHttp, Log, TEXT("Copilot response: %s"), *Resp->GetContentAsString());
    });
    Request->ProcessRequest();
}
