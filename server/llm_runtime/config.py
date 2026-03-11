from __future__ import annotations

from typing import Optional

from pydantic import BaseSettings, Field


class Settings(BaseSettings):
    external_llm_endpoint: Optional[str] = Field(None, env="EXTERNAL_LLM_ENDPOINT")
    external_llm_api_key: Optional[str] = Field(None, env="EXTERNAL_LLM_API_KEY")

    class Config:
        env_file = ".env"


settings = Settings()
