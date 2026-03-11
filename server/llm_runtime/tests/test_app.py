from fastapi.testclient import TestClient

from server.llm_runtime import app


def test_models_endpoint() -> None:
    client = TestClient(app)
    response = client.get("/models")
    assert response.status_code == 200
    payload = response.json()
    assert "models" in payload


def test_completion_endpoint() -> None:
    client = TestClient(app)
    response = client.post("/v1/completions", json={
        "model": "game-qwen-7b",
        "prompt": "Describe player movement.",
    })
    assert response.status_code == 200
    body = response.json()
    assert body["model"] == "game-qwen-7b"


def test_lora_endpoint() -> None:
    client = TestClient(app)
    response = client.post("/models/game-qwen-7b/lora", json={
        "base_model": "game-qwen-7b",
        "dataset_path": "dataset/game_project",
    })
    assert response.status_code == 200
    assert "job_id" in response.json()
