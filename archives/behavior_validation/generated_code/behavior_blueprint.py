"""Auto-generated behavior blueprint for Demo Behavior"""

BEHAVIOR_ID = "demo-behavior"
BEHAVIOR_NAME = "Demo Behavior"
STEP_OVERVIEW = [
    {
        "name": "Detect Target",
        "type": "perception",
        "target": "primary_enemy"
    },
    {
        "name": "Navigate To Cover",
        "type": "navigation",
        "target": "movement"
    },
    {
        "name": "Engage",
        "type": "combat",
        "target": "primary_enemy"
    }
]
RESOURCE_OVERVIEW = [
    {
        "name": "primary_enemy",
        "type": "character",
        "path": "/Game/Characters/EnemyBoss",
        "tags": null
    },
    {
        "name": "cover_slot",
        "type": "environment",
        "path": "/Game/Props/Cover/CoverBase",
        "tags": [
            "tactical",
            "reusable"
        ]
    }
]
BINDING_OVERVIEW = [
    {
        "behavior": "demo-behavior",
        "resource": "primary_enemy",
        "mode": "read"
    },
    {
        "behavior": "demo-behavior",
        "resource": "cover_slot",
        "mode": "reserve"
    }
]

def describe_behavior():
    return {
        "id": BEHAVIOR_ID,
        "name": BEHAVIOR_NAME,
        "steps": STEP_OVERVIEW,
        "resources": RESOURCE_OVERVIEW,
        "bindings": BINDING_OVERVIEW,
    }
