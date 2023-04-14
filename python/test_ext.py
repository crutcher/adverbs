import _py_adverbs


def test_basic() -> None:
    for idx, device in enumerate(_py_adverbs.list_devices()):
        if idx == 0:
            print(device.__doc__)
            print(dir(device))

        print(device)
        context = device.open()
        print(context)
