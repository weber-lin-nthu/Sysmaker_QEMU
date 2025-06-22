#include "qemu/osdep.h"
#include "hw/gpio/perif_pinout.h"
#include "qapi/error.h"
#include "qapi/qmp/qstring.h"
#include "sysemu/cpu-timers.h"

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
static void sc_data_finalize(Object *obj)
{
    SCData *o = SC_DATA(obj);
    qdict_unref(o->pin_value);
}
static const TypeInfo sc_data_info = {
    .name              = TYPE_SC_DATA,
    .parent            = TYPE_OBJECT,
    .instance_size     = sizeof(SCData),
    .instance_init     = sc_data_init,
    .instance_finalize = sc_data_finalize,
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
        "    'BeginTime': %ld,"
        "    'EndTime': %ld,"
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

/*
 * Send a single packet to SystemC
 */
static void systemc_write(PerifPinoutDeviceClass *klass, GString *message)
{
    if (!klass->systemc_addr) {
        return;
    }
    int64_t len = message->len;
    qio_channel_write(QIO_CHANNEL(klass->systemc_addr), (char *)&len, 8, &error_fatal);
    qio_channel_write(QIO_CHANNEL(klass->systemc_addr), message->str, len, &error_fatal);
}

/*
 * Read a single packet from SystemC
 * Return the resulting GString, the caller is responsible for freeing it.
 */
static GString *systemc_read(PerifPinoutDeviceClass *klass)
{
    if (!klass->systemc_addr) {
        return NULL;
    }
    uint64_t len = 0;
    qio_channel_read(QIO_CHANNEL(klass->systemc_addr), (char *)&len, 8, &error_fatal);
    GString *msg = g_string_sized_new(len);
    for (ssize_t read = 0; read < len;) {
        read += qio_channel_read(QIO_CHANNEL(klass->systemc_addr), &msg->str[read], len - read, &error_fatal);
    }
    return msg;
}

/*
 * Send a data_pack to SystemC, and receive new data_pack from SystemC
 * Return the resulting data_pack, the caller is responsible for freeing it.
 */
static SCDataPack *transport(PerifPinoutDeviceClass *klass, const char *perif_name, SCDataPack *data_pack)
{
    if (!klass->systemc_addr) {
        return NULL;
    }
    const QDict *pin_to_external_hw = klass->external_hw_pins.pin_to_external_hw;
    const QDict *external_hw_to_pin = klass->external_hw_pins.external_hw_to_pin;
    QDict *pin_set                  = qdict_clone_shallow(qdict_get_qdict(klass->peripheral_pins, perif_name));
    QDict *hw_set                   = qdict_new();
    QDict *pin_value                = data_pack->data->pin_value;

    data_pack->begin_time = icount_get(); // in nano second

    // g_autoptr(GString) before_msg = scdatapack_to_json(data_pack, true);
    // qemu_log("Before annotation: \n%s\n", before_msg->str);

    int max_iter = 512;
    for (bool updated = true; updated && max_iter--;) {
        updated = false;
        const QDictEntry *it;
        for (it = qdict_first(pin_set); it;
             it = qdict_next(pin_set, it)) {
            const char *pin_name     = qdict_entry_key(it);
            const QList *external_hw = qdict_get_qlist(pin_to_external_hw, pin_name);
            const QListEntry *eit;
            if (!external_hw) {
                continue;
            }
            for (eit = qlist_first(external_hw); eit;
                 eit = qlist_next(eit)) {
                const char *hw_name = qstring_get_str(qobject_to(QString, qlist_entry_obj(eit)));
                qdict_put_null(hw_set, hw_name);
            }
        }

        for (it = qdict_first(hw_set); it;
             it = qdict_next(hw_set, it)) {
            const char *hw_name = qdict_entry_key(it);
            const QList *pins   = qdict_get_qlist(external_hw_to_pin, hw_name);
            const QListEntry *pit;
            if (!pins) {
                continue;
            }
            for (pit = qlist_first(pins); pit;
                 pit = qlist_next(pit)) {
                const char *pin_name = qstring_get_str(qobject_to(QString, qlist_entry_obj(pit)));
                if (!qdict_get(pin_set, pin_name)) { // new pin found!
                    qdict_put_null(pin_set, pin_name);
                    const char *cur_pin_val = qdict_get_try_str(klass->pin_value, pin_name);
                    if (!cur_pin_val) {
                        cur_pin_val = PERIF_PIN_DEFAULT_VAL;
                    }
                    qdict_put_str(pin_value, pin_name, cur_pin_val);
                    updated = true;
                }
            }
        }
    }
    qdict_unref(hw_set);
    if (max_iter < 0) {
        Error *err = NULL;
        error_setg(&err, "In SystemC transport: pin set supplementation max iteration count reached! Pin set might not be complete.\n");
        warn_report_err(err);
    }

    const QDictEntry *it;
    for (it = qdict_first(pin_set); it;
         it = qdict_next(pin_set, it)) {
        const char *pin_name = qdict_entry_key(it);
        qlist_append_str(data_pack->pins, pin_name);
    }
    data_pack->interface_config->pin_config = qdict_clone_shallow(pin_set);

    g_autoptr(GString) msg = scdatapack_to_json(data_pack, true);
    systemc_write(klass, msg);

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
    if (g_strcmp0(value, PERIF_PIN_DEFAULT_VAL) != 0 || qdict_get_try_str(klass->pin_value, pin_name)) {
        qdict_put_str(klass->pin_value, pin_name, value);
    }
}

static void perif_pinout_netlist_init(PerifPinoutDeviceClass *k)
{
    // recv netlist.json
    QDict *netlist                 = NULL;
    g_autoptr(GString) netlist_str = systemc_read(k);
    if (netlist_str) {
        Error *err = NULL;
        netlist    = qobject_to(QDict, qobject_from_json(netlist_str->str, &err));
        if (err) {
            error_append_hint(&err, "Cannot convert netlist received from SystemC to dictionary!\n");
            error_append_hint(&err, "Netlist size: %ld, Netlist received: \n%s\n", netlist_str->len, netlist_str->str);
            error_propagate(&error_fatal, err);
        }
    } else {
        netlist = qdict_new();
    }

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

    g_autoptr(GString) setting = qobject_to_json_pretty(QOBJECT(pin_to_external_hw), true);
    qemu_log("pin_to_external_hw: \n%s\n", setting->str);
    g_string_free(setting, true);
    setting = qobject_to_json_pretty(QOBJECT(external_hw_to_pin), true);
    qemu_log("external_hw_to_pin: \n%s\n", setting->str);
}

const char *systemc_path;

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
    k->systemc_addr                        = NULL;
}
static void perif_pinout_device_init(Object *obj)
{
    PerifPinoutDeviceClass *k = PERIF_PINOUT_DEVICE_CLASS(obj->class);
    if (!k->systemc_addr && systemc_path) {
        // Connect to SystemC
        g_autoptr(SocketAddress) addr = g_new0(SocketAddress, 1);
        addr->type                    = SOCKET_ADDRESS_TYPE_UNIX;
        addr->u.q_unix.path           = g_strdup(systemc_path);
        qio_channel_socket_listen_sync(k->iocs, addr, 1, &error_abort);
        qemu_log("Connecting to SystemC...\n");
        k->systemc_addr = qio_channel_socket_accept(k->iocs, &error_abort);
        qemu_log("Connected\n");

        perif_pinout_netlist_init(k);

        // [test] send test data
        g_autoptr(SCDataPack) data_pack = sc_datapack_new(NULL, NULL, NULL);
        g_autoptr(GString) buf          = scdatapack_to_json(data_pack, true);
        systemc_write(k, buf);
    }
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