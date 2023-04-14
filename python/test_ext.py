import _py_adverbs


def test_basic() -> None:
    for idx, device in enumerate(_py_adverbs.list_devices()):
        context = device.open()
        attr = context.attr()

        print(device)
        print(" * fw ver:", attr.fw_ver)

        for port in context.ports():
            print(f" * port:{port.lid} state:{port.state} max_mtu:{port.max_mtu}")
            print(f" * flags: {port.expand_flags()}")
            print(f" * flags2: {port.expand_flags2()}")
