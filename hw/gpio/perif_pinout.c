#include "qemu/osdep.h"
#include "hw/gpio/perif_pinout.h"
#include "qapi/error.h"
#include "qapi/qmp/qstring.h"

static QDict *sc_interface_config_to_dict(SCInterfaceConfig *obj)
{
    QDict *res = qdict_new();
    qdict_put(res, "Pin Configuration", qdict_clone_shallow(obj->pin_config));
    return res;
}

static void sc_interface_config_init(Object *obj)
{
    SCInterfaceConfig *o = SC_INTERFACE_CONFIG(obj);
    o->to_dict           = sc_interface_config_to_dict;
    o->pin_config        = qdict_new();
}
static void sc_interface_config_finalize(Object *obj)
{
    SCInterfaceConfig *o = SC_INTERFACE_CONFIG(obj);
    qdict_unref(o->pin_config);
}
static const TypeInfo sc_interface_config_info = {
    .name              = TYPE_SC_INTERFACE_CONFIG,
    .parent            = TYPE_OBJECT,
    .instance_size     = sizeof(SCInterfaceConfig),
    .instance_init     = sc_interface_config_init,
    .instance_finalize = sc_interface_config_finalize,
};
static void sc_interface_config_register_types(void)
{
    type_register_static(&sc_interface_config_info);
}
type_init(sc_interface_config_register_types);

static QDict *sc_data_to_dict(SCData *obj)
{
    return qdict_clone_shallow(obj->pin_value);
}

static void sc_data_init(Object *obj)
{
    SCData *o    = SC_DATA(obj);
    o->to_dict   = sc_data_to_dict;
    o->pin_value = qdict_new();
}
static const TypeInfo sc_data_info = {
    .name          = TYPE_SC_DATA,
    .parent        = TYPE_OBJECT,
    .instance_size = sizeof(SCData),
    .instance_init = sc_data_init,
};
static void sc_data_register_types(void)
{
    type_register_static(&sc_data_info);
}
type_init(sc_data_register_types);

SCDataPack *sc_datapack_new(const char *typename, const char *interface_typename, const char *data_typename)
{
    if (!typename) {
        typename           = "Abstract";
        interface_typename = TYPE_SC_INTERFACE_CONFIG;
        data_typename      = TYPE_SC_DATA;
    }
    SCDataPack *obj       = (SCDataPack *)g_malloc0(sizeof(SCDataPack));
    obj->pins             = qlist_new();
    obj->type             = g_string_new(typename);
    obj->interface_config = SC_INTERFACE_CONFIG(object_new(interface_typename));
    obj->data             = SC_DATA(object_new(data_typename));
    return obj;
}

void sc_datapack_free(SCDataPack *obj)
{
    object_unref(obj->data);
    object_unref(obj->interface_config);
    g_string_free(obj->type, true);
    qlist_unref(obj->pins);
    g_free(obj);
}

static GString *scdatapack_to_json(SCDataPack *obj, bool pretty)
{
    QDict *ic        = obj->interface_config->to_dict(obj->interface_config);
    QDict *data      = obj->data->to_dict(obj->data);
    QDict *data_pack = qdict_from_jsonf_nofail(
        "{"
        "    'Pins': %p,"
        "    'BeginTime': %d,"
        "    'EndTime': %d,"
        "    'Type': %s,"
        "    'Interface Configuration': %p,"
        "    'Data': %p"
        "}",
        qlist_copy(obj->pins),
        obj->begin_time,
        obj->end_Time,
        obj->type->str,
        ic,
        data);
    GString *str = qobject_to_json_pretty(QOBJECT(data_pack), pretty);
    qobject_unref(data_pack);
    return str;
}

static SCDataPack *transport(PerifPinoutDeviceClass *klass, const char *perif_name, SCDataPack *data_pack)
{
    const QDict *pin_to_external_hw = klass->external_hw_pins.pin_to_external_hw;
    const QDict *external_hw_to_pin = klass->external_hw_pins.external_hw_to_pin;
    QDict *pin_set                  = qdict_clone_shallow(qdict_get_qdict(klass->peripheral_pins, perif_name));
    QDict *hw_set                   = qdict_new();
    QDict *pin_value                = data_pack->data->pin_value;

    int max_iter = 100;
    for (bool updated = true; updated && max_iter--;) {
        updated = false;
        const QDictEntry *it;
        for (it = qdict_first(pin_set); it;
             it = qdict_next(pin_set, it)) {
            const char *pin_name = qdict_entry_key(it);
            QList *external_hw   = qdict_get_qlist(pin_to_external_hw, pin_name);
            const QListEntry *eit;
            for (eit = qlist_first(external_hw); eit;
                 eit = qlist_next(eit)) {
                const char *hw_name = qstring_get_str(qobject_to(QString, qlist_entry_obj(eit)));
                qdict_put_null(hw_set, hw_name);
            }
        }

        for (it = qdict_first(hw_set); it;
             it = qdict_next(hw_set, it)) {
            const char *hw_name = qdict_entry_key(it);
            QList *pins         = qdict_get_qlist(external_hw_to_pin, hw_name);
            const QListEntry *pit;
            for (pit = qlist_first(pins); pit;
                 pit = qlist_next(pit)) {
                const char *pin_name = qstring_get_str(qobject_to(QString, qlist_entry_obj(pit)));
                if (!qdict_get(pin_set, pin_name)) {
                    qdict_put_null(pin_set, pin_name);
                    if (qdict_get_try_str(klass->pin_value, pin_name))
                        qdict_put_str(pin_value, pin_name, qdict_get_str(klass->pin_value, pin_name));
                    updated = true;
                }
            }
        }
    }
    qdict_unref(hw_set);
    if (max_iter < 0)
        qemu_log("In transport: pin set expansion max iteration count reached!\n");

    const QDictEntry *it;
    for (it = qdict_first(pin_set); it;
         it = qdict_next(pin_set, it)) {
        const char *pin_name = qdict_entry_key(it);
        qlist_append_str(data_pack->pins, pin_name);
    }
    data_pack->interface_config->pin_config = qdict_clone_shallow(pin_set);

    GString *msg = scdatapack_to_json(data_pack, true);
    qio_channel_write(QIO_CHANNEL(klass->systemc_addr), msg->str, msg->len, &error_abort);
    return sc_datapack_new(NULL, NULL, NULL);
}

static void register_perif_pin(PerifPinoutDeviceClass *klass, const char *perif_name, const char *pin_name, const char *func_name)
{
    QDict *pins = qdict_get_qdict(klass->peripheral_pins, perif_name);
    if (!pins) {
        pins = qdict_new();
        qdict_put(klass->peripheral_pins, perif_name, pins);
    }
    qdict_put_str(pins, pin_name, func_name);
}

static void unregister_perif_pin(PerifPinoutDeviceClass *klass, const char *perif_name, const char *pin_name)
{
    QDict *pins = qdict_get_qdict(klass->peripheral_pins, perif_name);
    if (pins) {
        qdict_del(pins, pin_name);
    }
}

static void set_pin_value(PerifPinoutDeviceClass *klass, const char *pin_name, const char *value)
{
    if (g_strcmp0(value, "0V") != 0 || qdict_get_try_str(klass->pin_value, pin_name)) {
        qdict_put_str(klass->pin_value, pin_name, value);
    }
}

static void perif_pinout_netlist_init(PerifPinoutDeviceClass *k, QDict *netlist)
{
    QDict *pin_to_external_hw = k->external_hw_pins.pin_to_external_hw;
    QDict *external_hw_to_pin = k->external_hw_pins.external_hw_to_pin;
    const QDictEntry *it;
    for (it = qdict_first(netlist); it;
         it = qdict_next(netlist, it)) {
        const char *pin_name = qdict_entry_key(it);
        QDict *hws           = qobject_to(QDict, qdict_entry_value(it));
        QList *external_hw   = qdict_get_qlist(pin_to_external_hw, pin_name);
        if (!external_hw) {
            external_hw = qlist_new();
            qdict_put(pin_to_external_hw, pin_name, external_hw);
        }
        const QDictEntry *hw_it;
        for (hw_it = qdict_first(hws); hw_it;
             hw_it = qdict_next(hws, hw_it)) {
            const char *hw_name = qdict_entry_key(hw_it);
            qlist_append_str(external_hw, hw_name);

            QList *pins = qdict_get_qlist(external_hw_to_pin, hw_name);
            if (!pins) {
                pins = qlist_new();
                qdict_put(external_hw_to_pin, hw_name, pins);
            }
            qlist_append_str(pins, pin_name);
        }
    }

    GString *setting = qobject_to_json_pretty(QOBJECT(pin_to_external_hw), true);
    qemu_log("pin_to_external_hw: \n%s\n", setting->str);
    g_string_free(setting, true);
    setting = qobject_to_json_pretty(QOBJECT(external_hw_to_pin), true);
    qemu_log("external_hw_to_pin: \n%s\n", setting->str);
    g_string_free(setting, true);
}

static void perif_pinout_device_class_init(ObjectClass *klass, void *data)
{
    PerifPinoutDeviceClass *k              = PERIF_PINOUT_DEVICE_CLASS(klass);
    k->transport                           = transport;
    k->register_perif_pin                  = register_perif_pin;
    k->unregister_perif_pin                = unregister_perif_pin;
    k->set_pin_value                       = set_pin_value;
    k->iocs                                = qio_channel_socket_new();
    k->external_hw_pins.pin_to_external_hw = qdict_new();
    k->external_hw_pins.external_hw_to_pin = qdict_new();
    k->peripheral_pins                     = qdict_new();
    k->pin_value                           = qdict_new();

    // Connect to SystemC
    SocketAddress *addr = g_new0(SocketAddress, 1);
    addr->type          = SOCKET_ADDRESS_TYPE_UNIX;
    addr->u.q_unix.path = g_strdup("/tmp/fake_qemu.sock");
    qio_channel_socket_listen_sync(k->iocs, addr, 1, &error_abort);
    qemu_log("Connecting to SystemC...\n");
    k->systemc_addr = qio_channel_socket_accept(k->iocs, &error_abort);
    qemu_log("Connected\n");
    qapi_free_SocketAddress(addr);

    // recv netlist.json
    QDict *netlist       = NULL;
    int netlist_str_size = 0;
    qio_channel_read(QIO_CHANNEL(k->systemc_addr), (char *)&netlist_str_size, sizeof(int), &error_abort);
    if (netlist_str_size != 0) {
        GString *netlist_str = g_string_sized_new(netlist_str_size + 1);
        qio_channel_read(QIO_CHANNEL(k->systemc_addr), netlist_str->str, netlist_str->allocated_len, &error_abort);
        Error *errp = NULL;
        netlist     = qobject_to(QDict, qobject_from_json(netlist_str->str, &errp));
        if (errp != NULL) {
            error_append_hint(&errp, "Cannot convert netlist received from SystemC to dictionary!\n");
            error_append_hint(&errp, "Netlist size: %d\n", netlist_str_size);
            error_append_hint(&errp, "Netlist received: \n%s\n", netlist_str->str);
            error_report_err(errp);
            exit(1);
        }
    } else {
        netlist = qdict_new();
    }
    perif_pinout_netlist_init(k, netlist);

    // [test] send test data
    SCDataPack *data_pack = sc_datapack_new(NULL, NULL, NULL);
    GString *buf          = scdatapack_to_json(data_pack, true);
    qio_channel_write(QIO_CHANNEL(k->systemc_addr), buf->str, buf->len, &error_abort);
}
static void perif_pinout_device_init(Object *obj)
{
    // TODO
}
static const TypeInfo perif_pinout_device_type_info = {
    .name          = TYPE_PERIF_PINOUT_DEVICE,
    .parent        = TYPE_DEVICE,
    .instance_size = sizeof(PerifPinoutDevice),
    .instance_init = perif_pinout_device_init,
    .class_size    = sizeof(PerifPinoutDeviceClass),
    .class_init    = perif_pinout_device_class_init,
};
static void perif_pinout_register_types(void)
{
    type_register_static(&perif_pinout_device_type_info);
}
type_init(perif_pinout_register_types);