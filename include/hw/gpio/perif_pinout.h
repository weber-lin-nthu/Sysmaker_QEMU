#ifndef HW_PERIF_PINOUT_H
#define HW_PERIF_PINOUT_H

#include "hw/qdev-core.h"
#include "qom/object.h"
#include "qemu/log.h"
#include "qapi/qmp/qjson.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qlist.h"
#include "io/channel-socket.h"

#define TYPE_PERIF_PINOUT_DEVICE "perif-pinout-device"
OBJECT_DECLARE_TYPE(PerifPinoutDevice, PerifPinoutDeviceClass, PERIF_PINOUT_DEVICE)

#define TYPE_SC_INTERFACE_CONFIG "sc-interface-config"
OBJECT_DECLARE_SIMPLE_TYPE(SCInterfaceConfig, SC_INTERFACE_CONFIG)

#define TYPE_SC_DATA "sc-data"
OBJECT_DECLARE_SIMPLE_TYPE(SCData, SC_DATA)

#define PERIF_PIN_DEFAULT_VAL "0V"

struct SCInterfaceConfig {
    /*< private >*/
    Object parent;
    /*< public >*/
    QDict *pin_config;

    QDict *(*to_dict)(SCInterfaceConfig *obj);
    void (*from_dict)(SCInterfaceConfig *obj, QDict *d);
};

struct SCData {
    /*< private >*/
    Object parent;
    /*< public >*/
    QDict *pin_value;

    QDict *(*to_dict)(SCData *obj);
    void (*from_dict)(SCData *obj, QDict *d);
};

typedef struct SCDataPack {
    QList *pins;
    GString *begin_time;
    GString *end_time;
    GString *type;
    SCInterfaceConfig *interface_config;
    SCData *data;
} SCDataPack;

SCDataPack *sc_datapack_new(const char *typename, const char *interface_typename, const char *data_typename);
void sc_datapack_free(SCDataPack *obj);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(SCDataPack, sc_datapack_free);

extern const char *systemc_path;

struct PerifPinoutDeviceClass {
    /*< private >*/
    DeviceClass parent_class;
    QIOChannelSocket *iocs;
    QIOChannelSocket *systemc_addr;

    struct ExternalHWPins {
        // should be {"pin_name": ["hw_name1", "hw_name2"]}
        QDict *pin_to_external_hw;
        // should be {"hw_name": ["pin_name1", "pin_name2"]}
        QDict *external_hw_to_pin;
    } external_hw_pins;
    /* `peripheral_pins` should be like:
        {
            "perif_name1": {"pin_name1": "function", "pin_name2": "function"},
            "perif_name2": {"pin_name3": "function", "pin_name4": "function"}
        }
    */
    QDict *peripheral_pins;
    // should be {"pin_name1": "value1", "pin_name2": "value2"}
    QDict *pin_value;

    /*< public >*/
    void (*transport)(PerifPinoutDeviceClass *klass, const char *perif_name, SCDataPack *data_pack, SCDataPack *resp_data_pack);
    void (*register_perif_pin)(PerifPinoutDeviceClass *klass, const char *perif_name, const char *pin_name, const char *func_name);
    void (*unregister_perif_pin)(PerifPinoutDeviceClass *klass, const char *perif_name, const char *pin_name);
    void (*set_pin_value)(PerifPinoutDeviceClass *klass, const char *pin_name, const char *value);
};

struct PerifPinoutDevice {
    /*< private >*/
    DeviceState parent_obj;
    /*< public >*/
};

#endif
