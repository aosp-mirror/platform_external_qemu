/*
 * Goldfish 'events' device model
 *
 * Copyright (C) 2007-2008 The Android Open Source Project
 * Copyright (c) 2014 Linaro Limited
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "ui/input.h"
#include "ui/console.h"
#include "hw/input/android_keycodes.h"
#include "hw/input/linux_keycodes.h"

#define MAX_EVENTS (256 * 4)

typedef struct {
    const char *name;
    int value;
} RotaryEventCodeInfo;

/* Absolute axes */
#define ABS_X                   0x00
#define ABS_Y                   0x01
#define ABS_Z                   0x02
#define ABS_MT_SLOT             0x2f    /* MT slot being modified */
#define ABS_MT_TOUCH_MAJOR      0x30    /* Major axis of touching ellipse */
#define ABS_MT_TOUCH_MINOR      0x31    /* Minor axis (omit if circular) */
#define ABS_MT_WIDTH_MAJOR      0x32    /* Major axis of approaching ellipse */
#define ABS_MT_WIDTH_MINOR      0x33    /* Minor axis (omit if circular) */
#define ABS_MT_ORIENTATION      0x34    /* Ellipse orientation */
#define ABS_MT_POSITION_X       0x35    /* Center X ellipse position */
#define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE        0x37    /* Type of touching device */
#define ABS_MT_BLOB_ID          0x38    /* Group a set of packets as a blob */
#define ABS_MT_TRACKING_ID      0x39    /* Unique ID of initiated contact */
#define ABS_MT_PRESSURE         0x3a    /* Pressure on contact area */
#define ABS_MT_DISTANCE         0x3b    /* Contact hover distance */
#define ABS_MAX                 0x3f

#define BTN_TOUCH 0x14a
#define BTN_MOUSE 0x110

#define EV_CODE(_code)    {#_code, (_code)}
#define EV_CODE_END     {NULL, 0}
static const RotaryEventCodeInfo ev_abs_codes_table[] = {
    EV_CODE(ABS_X),
    EV_CODE(ABS_Y),
    EV_CODE(ABS_Z),
    EV_CODE(ABS_MT_SLOT),
    EV_CODE(ABS_MT_TOUCH_MAJOR),
    EV_CODE(ABS_MT_TOUCH_MINOR),
    EV_CODE(ABS_MT_WIDTH_MAJOR),
    EV_CODE(ABS_MT_WIDTH_MINOR),
    EV_CODE(ABS_MT_ORIENTATION),
    EV_CODE(ABS_MT_POSITION_X),
    EV_CODE(ABS_MT_POSITION_Y),
    EV_CODE(ABS_MT_TOOL_TYPE),
    EV_CODE(ABS_MT_BLOB_ID),
    EV_CODE(ABS_MT_TRACKING_ID),
    EV_CODE(ABS_MT_PRESSURE),
    EV_CODE(ABS_MT_DISTANCE),
    EV_CODE(ABS_MAX),
    EV_CODE_END,
};

/* Relative axes */
#define REL_WHEEL               0x08
static const RotaryEventCodeInfo ev_rel_codes_table[] = {
    EV_CODE(REL_WHEEL),
    EV_CODE_END,
};


#define KEY_CODE(_name, _val) {#_name, _val}
#define BTN_CODE(_code) {#_code, (_code)}
typedef struct {
    const char *name;
    int value;
    const RotaryEventCodeInfo *codes;
} RotaryEventTypeInfo;

/* Event types (as per Linux input event layer) */
#define EV_SYN                  0x00
#define EV_KEY                  0x01
#define EV_REL                  0x02
#define EV_ABS                  0x03
#define EV_MSC                  0x04
#define EV_SW                   0x05
#define EV_LED                  0x11
#define EV_SND                  0x12
#define EV_REP                  0x14
#define EV_FF                   0x15
#define EV_PWR                  0x16
#define EV_FF_STATUS            0x17
#define EV_MAX                  0x1f

#define EV_TYPE(_type, _codes)    {#_type, (_type), _codes}
#define EV_TYPE_END     {NULL, 0}
static const RotaryEventTypeInfo ev_type_table[] = {
    EV_TYPE(EV_SYN, NULL),
    EV_TYPE(EV_KEY, NULL),
    EV_TYPE(EV_REL, ev_rel_codes_table),
    EV_TYPE(EV_ABS, ev_abs_codes_table),
    EV_TYPE(EV_MSC, NULL),
    EV_TYPE(EV_SW, NULL),
    EV_TYPE(EV_LED, NULL),
    EV_TYPE(EV_SND, NULL),
    EV_TYPE(EV_REP, NULL),
    EV_TYPE(EV_FF, NULL),
    EV_TYPE(EV_PWR, NULL),
    EV_TYPE(EV_FF_STATUS, NULL),
    EV_TYPE(EV_MAX, NULL),
    EV_TYPE_END
};

enum {
    REG_READ        = 0x00,
    REG_SET_PAGE    = 0x00,
    REG_LEN         = 0x04,
    REG_DATA        = 0x08,

    PAGE_NAME       = 0x00000,
    PAGE_EVBITS     = 0x10000,
    PAGE_ABSDATA    = 0x20000 | EV_ABS,
};

/* These corresponds to the state of the driver.
 * Unfortunately, we have to buffer events coming
 * from the UI, since the kernel driver is not
 * capable of receiving them until XXXXXX
 */
enum {
    STATE_INIT = 0,  /* The device is initialized */
    STATE_BUFFERED,  /* Events have been buffered, but no IRQ raised yet */
    STATE_LIVE       /* Events can be sent directly to the kernel */
};

/* NOTE: The ev_bits arrays are used to indicate to the kernel
 *       which events can be sent by the emulated hardware.
 */

#define TYPE_ROTARYEVDEV "rotary-events"
#define ROTARYEVDEV(obj) \
    OBJECT_CHECK(RotaryEvDevState, (obj), TYPE_ROTARYEVDEV)

typedef struct RotaryEvDevState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    MemoryRegion iomem;
    qemu_irq irq;

    /* Actual device state */
    int32_t page;
    uint32_t events[MAX_EVENTS];
    uint32_t first;
    uint32_t last;
    uint32_t state;

    uint32_t modifier_state;

    /* All data below here is set up at realize and not modified thereafter */

    const char *name;

    struct {
        size_t   len;
        uint8_t *bits;
    } ev_bits[EV_MAX + 1];

    int32_t *abs_info;
    size_t abs_info_count;
} RotaryEvDevState;

/* Bitfield meanings for modifier_state. */
#define MODSTATE_SHIFT (1 << 0)
#define MODSTATE_CTRL (1 << 1)
#define MODSTATE_ALT (1 << 2)
#define MODSTATE_MASK (MODSTATE_SHIFT | MODSTATE_CTRL | MODSTATE_ALT)

/* An entry in the array of ABS_XXX values */
typedef struct ABSEntry {
    /* Minimum ABS_XXX value. */
    uint32_t    min;
    /* Maximum ABS_XXX value. */
    uint32_t    max;
    /* 'fuzz;, and 'flat' ABS_XXX values are always zero here. */
    uint32_t    fuzz;
    uint32_t    flat;
} ABSEntry;

/* Pointer to the global device instance. Also serves as an initialization
 * flag in rotary_event_send() to filter-out events that are sent from
 * the UI before the device was properly realized.
 */
static RotaryEvDevState* s_evdev = NULL;

static const VMStateDescription vmstate_rotary_evdev = {
    .name = "rotary-events",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_INT32(page, RotaryEvDevState),
        VMSTATE_UINT32_ARRAY(events, RotaryEvDevState, MAX_EVENTS),
        VMSTATE_UINT32(first, RotaryEvDevState),
        VMSTATE_UINT32(last, RotaryEvDevState),
        VMSTATE_UINT32(state, RotaryEvDevState),
        VMSTATE_UINT32(modifier_state, RotaryEvDevState),
        VMSTATE_END_OF_LIST()
    }
};

static void enqueue_event(RotaryEvDevState *s,
                          unsigned int type, unsigned int code, int value)
{
    int  enqueued = s->last - s->first;

    if (enqueued < 0) {
        enqueued += MAX_EVENTS;
    }

    if (enqueued + 3 > MAX_EVENTS) {
        fprintf(stderr, "##KBD: Full queue, lose event\n");
        return;
    }

    if (s->first == s->last) {
        if (s->state == STATE_LIVE) {
            qemu_irq_raise(s->irq);
        } else {
            s->state = STATE_BUFFERED;
        }
    }

    s->events[s->last] = type;
    s->last = (s->last + 1) & (MAX_EVENTS-1);
    s->events[s->last] = code;
    s->last = (s->last + 1) & (MAX_EVENTS-1);
    s->events[s->last] = value;
    s->last = (s->last + 1) & (MAX_EVENTS-1);

}

static unsigned dequeue_event(RotaryEvDevState *s)
{
    unsigned n;

    if (s->first == s->last) {
        return 0;
    }

    n = s->events[s->first];

    s->first = (s->first + 1) & (MAX_EVENTS - 1);

    if (s->first == s->last) {
        qemu_irq_lower(s->irq);
    }
#ifdef TARGET_I386
    /*
     * Adding the logic to handle edge-triggered interrupts for x86
     * because the exisiting goldfish events device basically provides
     * level-trigger interrupts only.
     *
     * Logic: When an event (including the type/code/value) is fetched
     * by the driver, if there is still another event in the event
     * queue, the goldfish event device will re-assert the IRQ so that
     * the driver can be notified to fetch the event again.
     */
    else if (((s->first + 2) & (MAX_EVENTS - 1)) < s->last ||
               (s->first & (MAX_EVENTS - 1)) > s->last) {
        /* if there still is an event */
        qemu_irq_lower(s->irq);
        qemu_irq_raise(s->irq);
    }
#endif
    return n;
}

static int get_page_len(RotaryEvDevState *s)
{
    int page = s->page;
    if (page == PAGE_NAME) {
        const char *name = s->name;
        return strlen(name);
    }
    if (page >= PAGE_EVBITS && page <= PAGE_EVBITS + EV_MAX) {
        return s->ev_bits[page - PAGE_EVBITS].len;
    }
    if (page == PAGE_ABSDATA) {
        return s->abs_info_count * sizeof(s->abs_info[0]);
    }
    return 0;
}

static int get_page_data(RotaryEvDevState *s, int offset)
{
    int page_len = get_page_len(s);
    int page = s->page;
    if (offset > page_len) {
        return 0;
    }
    if (page == PAGE_NAME) {
        const char *name = s->name;
        return name[offset];
    }
    if (page >= PAGE_EVBITS && page <= PAGE_EVBITS + EV_MAX) {
        return s->ev_bits[page - PAGE_EVBITS].bits[offset];
    }
    if (page == PAGE_ABSDATA) {
        return s->abs_info[offset / sizeof(s->abs_info[0])];
    }
    return 0;
}

int rotary_event_send(int type, int code, int value)
{
    RotaryEvDevState *dev = s_evdev;

    if (dev) {
        enqueue_event(dev, type, code, value);
    }

    return 0;
}

static uint64_t events_read(void *opaque, hwaddr offset, unsigned size)
{
    RotaryEvDevState *s = (RotaryEvDevState *)opaque;

    /* This gross hack below is used to ensure that we
     * only raise the IRQ when the kernel driver is
     * properly ready! If done before this, the driver
     * becomes confused and ignores all input events
     * as soon as one was buffered!
     */
    if (offset == REG_LEN && s->page == PAGE_ABSDATA) {
        if (s->state == STATE_BUFFERED) {
            qemu_irq_raise(s->irq);
        }
        s->state = STATE_LIVE;
    }

    switch (offset) {
    case REG_READ:
        return dequeue_event(s);
    case REG_LEN:
        return get_page_len(s);
    default:
        if (offset >= REG_DATA) {
            return get_page_data(s, offset - REG_DATA);
        }
        qemu_log_mask(LOG_GUEST_ERROR,
                      "rotary events device read: bad offset %x\n",
                      (int)offset);
        return 0;
    }
}

static void events_write(void *opaque, hwaddr offset,
                         uint64_t val, unsigned size)
{
    RotaryEvDevState *s = (RotaryEvDevState *)opaque;
    switch (offset) {
    case REG_SET_PAGE:
        s->page = val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "rotary events device write: bad offset %x\n",
                      (int)offset);
        break;
    }
}

static const MemoryRegionOps rotary_evdev_ops = {
    .read = events_read,
    .write = events_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void rotary_evdev_put_mouse(void *opaque,
                               int dx, int dy, int dz, int buttons_state)
{
    RotaryEvDevState *s = (RotaryEvDevState *)opaque;

    /* Note that, like the "classic" Android emulator, we
     * have dz == 0 for touchscreen, == 1 for trackball
     */
    if (dz == 1) {
        enqueue_event(s, EV_REL, REL_WHEEL, dy);
        enqueue_event(s, EV_SYN, 0, 0);
        return;
    }
}

/* set bits [bitl..bith] in the ev_bits[type] array
 */
static void
events_set_bits(RotaryEvDevState *s, int type, int bitl, int bith)
{
    uint8_t *bits;
    uint8_t maskl, maskh;
    int il, ih;
    il = bitl / 8;
    ih = bith / 8;
    if (ih >= s->ev_bits[type].len) {
        bits = g_malloc0(ih + 1);
        if (bits == NULL) {
            return;
        }
        memcpy(bits, s->ev_bits[type].bits, s->ev_bits[type].len);
        g_free(s->ev_bits[type].bits);
        s->ev_bits[type].bits = bits;
        s->ev_bits[type].len = ih + 1;
    } else {
        bits = s->ev_bits[type].bits;
    }
    maskl = 0xffU << (bitl & 7);
    maskh = 0xffU >> (7 - (bith & 7));
    if (il >= ih) {
        maskh &= maskl;
    } else {
        bits[il] |= maskl;
        while (++il < ih) {
            bits[il] = 0xff;
        }
    }
    bits[ih] |= maskh;
}

static void
events_set_bit(RotaryEvDevState *s, int  type, int  bit)
{
    events_set_bits(s, type, bit, bit);
}

static void
events_clr_bit(RotaryEvDevState *s, int type, int bit)
{
    int ii = bit / 8;
    if (ii < s->ev_bits[type].len) {
        uint8_t *bits = s->ev_bits[type].bits;
        uint8_t  mask = 0x01U << (bit & 7);
        bits[ii] &= ~mask;
    }
}

static const int dpad_map[Q_KEY_CODE__MAX] = {
    [Q_KEY_CODE_KP_4] = LINUX_KEY_LEFT,
    [Q_KEY_CODE_KP_6] = LINUX_KEY_RIGHT,
    [Q_KEY_CODE_KP_8] = LINUX_KEY_UP,
    [Q_KEY_CODE_KP_2] = LINUX_KEY_DOWN,
    [Q_KEY_CODE_KP_5] = LINUX_KEY_CENTER,
};

static void rotary_evdev_handle_keyevent(DeviceState *dev, QemuConsole *src,
                                           InputEvent *evt)
{

    RotaryEvDevState *s = ROTARYEVDEV(dev);
    int lkey = 0;
    int mod;

    assert(evt->type == INPUT_EVENT_KIND_KEY);

    KeyValue* kv = evt->u.key.data->key;

    int qcode = kv->u.qcode.data;

    enqueue_event(s, EV_KEY, qcode, evt->u.key.data->down);

    int qemu2_qcode = qemu_input_key_value_to_qcode(evt->u.key.data->key);

    /* Keep our modifier state up to date */
    switch (qemu2_qcode) {
    case Q_KEY_CODE_SHIFT:
    case Q_KEY_CODE_SHIFT_R:
        mod = MODSTATE_SHIFT;
        break;
    case Q_KEY_CODE_ALT:
    case Q_KEY_CODE_ALT_R:
        mod = MODSTATE_ALT;
        break;
    case Q_KEY_CODE_CTRL:
    case Q_KEY_CODE_CTRL_R:
        mod = MODSTATE_CTRL;
        break;
    default:
        mod = 0;
        break;
    }

    if (mod) {
        if (evt->u.key.data->down) {
            s->modifier_state |= mod;
        } else {
            s->modifier_state &= ~mod;
        }
    }

    if (lkey) {
        enqueue_event(s, EV_KEY, lkey, evt->u.key.data->down);
    }
}

static const RotaryEventTypeInfo *rotary_get_event_type(const char *typename)
{
    const RotaryEventTypeInfo *type = NULL;
    int count = 0;

    /* Find the type descriptor by doing a name match */
    while (ev_type_table[count].name != NULL) {
        if (typename && !strcmp(ev_type_table[count].name, typename)) {
            type = &ev_type_table[count];
            break;
        }
        count++;
    }

    return type;
}

int rotary_get_event_type_count(void)
{
    int count = 0;

    while (ev_type_table[count].name != NULL) {
        count++;
    }

    return count;
}

int rotary_get_event_type_name(int type, char *buf)
{
   g_stpcpy(buf, ev_type_table[type].name);

   return 0;
}

int rotary_get_event_type_value(char *typename)
{
    const RotaryEventTypeInfo *type = rotary_get_event_type(typename);
    int ret = -1;

    if (type) {
        ret = type->value;
    }

    return ret;
}

static const RotaryEventCodeInfo *rotary_get_event_code(int typeval,
                                                      const char *codename)
{
    const RotaryEventTypeInfo *type = &ev_type_table[typeval];
    const RotaryEventCodeInfo *code = NULL;
    int count = 0;

    /* Find the type descriptor by doing a name match */
    while (type->codes[count].name != NULL) {
        if (codename && !strcmp(type->codes[count].name, codename)) {
            code = &type->codes[count];
            break;
        }
        count++;
    }

    return code;
}

int rotary_get_event_code_count(const char *typename)
{
    const RotaryEventTypeInfo *type = NULL;
    const RotaryEventCodeInfo *codes;
    int count = -1;     /* Return -1 if type not found */

    type = rotary_get_event_type(typename);

    /* Count the number of codes for the specified type if found */
    if (type) {
        count = 0;
        codes = type->codes;
        if (codes) {
            while (codes[count].name != NULL) {
                count++;
            }
        }
    }

    return count;
}

int rotary_get_event_code_name(const char *typename, unsigned int code, char *buf)
{
    const RotaryEventTypeInfo *type = rotary_get_event_type(typename);
    int ret = -1;   /* Return -1 if type not found */

    if (type && type->codes && code < rotary_get_event_code_count(typename)) {
        g_stpcpy(buf, type->codes[code].name);
        ret = 0;
    }

    return ret;
}

int rotary_get_event_code_value(int typeval, char *codename)
{
    const RotaryEventCodeInfo *code = rotary_get_event_code(typeval, codename);
    int ret = -1;

    if (code) {
        ret = code->value;
    }

    return ret;
}

static QemuInputHandler rotary_evdev_key_input_handler = {
    .name = "rotary event device key handler",
    .mask = INPUT_EVENT_MASK_KEY,
    .event = rotary_evdev_handle_keyevent,
};

static void rotary_evdev_init(Object *obj)
{
    RotaryEvDevState *s = ROTARYEVDEV(obj);
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &rotary_evdev_ops, s,
                          "rotary-events", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    qemu_input_handler_register(dev, &rotary_evdev_key_input_handler);
    // Register the mouse handler for relative position reports.
    qemu_add_mouse_event_handler(rotary_evdev_put_mouse, s, 0, "rotary-events");
}

static void rotary_evdev_realize(DeviceState *dev, Error **errp)
{

    RotaryEvDevState *s = ROTARYEVDEV(dev);

    /* Initialize the device ID so the event dev can be looked up duringi
     * monitor commands.
     */
    dev->id = g_strdup(TYPE_ROTARYEVDEV);

    /* now set the events capability bits depending on hardware configuration */
    /* apparently, the EV_SYN array is used to indicate which other
     * event classes to consider.
     */

    /* XXX PMM properties ? */
    // TODO(nimrod): Change to rotary
    s->name = "paj9124u1";

    /* configure EV_KEY array */
    events_set_bit(s, EV_SYN, EV_KEY);

    /* configure EV_REL array */
    events_set_bit(s, EV_SYN, EV_REL);
    events_set_bit(s, EV_REL, REL_WHEEL);

    /* Register global variable. */
    assert(s_evdev == NULL);
    s_evdev = s;
}

static void rotary_evdev_reset(DeviceState *dev)
{
    RotaryEvDevState *s = ROTARYEVDEV(dev);

    s->state = STATE_INIT;
    s->first = 0;
    s->last = 0;
    s->state = 0;
}

static Property rotary_evdev_props[] = {
    DEFINE_PROP_END_OF_LIST()
};

static void rotary_evdev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = rotary_evdev_realize;
    dc->reset = rotary_evdev_reset;
    dc->props = rotary_evdev_props;
    dc->vmsd = &vmstate_rotary_evdev;
}

static const TypeInfo rotary_evdev_info = {
    .name = TYPE_ROTARYEVDEV,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RotaryEvDevState),
    .instance_init = rotary_evdev_init,
    .class_init = rotary_evdev_class_init,
};

static void rotary_evdev_register_types(void)
{
    type_register_static(&rotary_evdev_info);
}

type_init(rotary_evdev_register_types)
