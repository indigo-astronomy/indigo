# Test and regression plan

Status: final

`xml_client_adapter_switches_protocol_with_a_named_request` in `unit/test_protocol_xml.c`: the named request asks for
the switch, a `switchProtocol` reply makes the next `Idle` IDLE and the next request plain; a legacy server keeps `Idle`
as OK. The whole unit suite, `test_detach_abort`, and `indigo_prop_tool get_state` against the GIOTTO.
