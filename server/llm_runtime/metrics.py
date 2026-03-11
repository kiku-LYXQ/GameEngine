from __future__ import annotations

from collections import defaultdict
from typing import Dict

class MetricsStore:
    def __init__(self) -> None:
        self._counters: Dict[str, int] = defaultdict(int)

    def increment(self, key: str, value: int = 1) -> None:
        self._counters[key] += value

    def snapshot(self) -> Dict[str, int]:
        return dict(self._counters)


metrics_store = MetricsStore()
