import adverbs_pyext


def test_basic() -> None:
    assert isinstance(adverbs_pyext.device_names(), list)
