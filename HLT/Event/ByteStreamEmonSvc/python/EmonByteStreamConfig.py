#
# Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration
#


from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from SGComps.SGInputLoaderConfig import SGInputLoaderCfg

def EmonByteStreamCfg(flags, type_names=[]):
    acc = ComponentAccumulator()

    bytestream_conversion = CompFactory.ByteStreamCnvSvc()
    acc.addService(bytestream_conversion, primary=True)

    bytestream_input = CompFactory.ByteStreamEmonInputSvc("ByteStreamInputSvc")
    acc.addService(bytestream_input)

    event_selector = CompFactory.EventSelectorByteStream(
        name='EventSelector',
        ByteStreamInputSvc=bytestream_input.name,
        FileBased=False)
    acc.addService(event_selector)
    acc.setAppProperty("EvtSel", event_selector.name)

    event_persistency = CompFactory.EvtPersistencySvc(
        name="EventPersistencySvc",
        CnvServices=[bytestream_conversion.name])
    acc.addService(event_persistency)

    address_provider = CompFactory.ByteStreamAddressProviderSvc(
        TypeNames=type_names)
    acc.addService(address_provider)

    proxy = CompFactory.ProxyProviderSvc()
    proxy.ProviderNames += [address_provider.name]
    acc.addService(proxy)

    acc.merge(SGInputLoaderCfg(flags, address_provider.TypeNames))

    return acc
