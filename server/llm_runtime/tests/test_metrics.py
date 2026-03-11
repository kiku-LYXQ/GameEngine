from fastapi.testclient import TestClient
from server.llm_runtime import app


def test_metrics_endpoint_contains_evaluation_counter() -> None:
    client = TestClient(app)
    response = client.get("/metrics")
    assert response.status_code == 200
def test_variated_metrics_keys() -> None:
    client = TestClient(app)
    response = client.get("/metrics")
    metrics = response.json()
    assert "evaluation.requests" in metrics
