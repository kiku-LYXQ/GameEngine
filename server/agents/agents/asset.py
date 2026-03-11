from __future__ import annotations

from typing import List


def find_assets(keywords: List[str]) -> List[str]:
    return [f"Content/VFX/{keyword}_fx" for keyword in keywords]
