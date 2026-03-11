from __future__ import annotations

from typing import Dict, List

from .models import ModelInfo


class ModelRegistry:
    def __init__(self) -> None:
        self._models: Dict[str, ModelInfo] = {}

    def register(self, info: ModelInfo) -> None:
        self._models[info.name] = info

    def get(self, name: str) -> ModelInfo:
        if name not in self._models:
            raise KeyError(name)
        return self._models[name]

    def list(self) -> List[ModelInfo]:
        return list(self._models.values())


registry = ModelRegistry()
