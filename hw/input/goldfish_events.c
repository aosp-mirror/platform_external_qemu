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

#include "hw/sysbus.h"
#include "ui/input.h"
#include "ui/console.h"
#include "hw/input/linux_keycodes.h"

/* Multitouch specific code is defined out via EVDEV_MULTITOUCH currently,
 * because upstream has no multitouch related APIs.
 */
/* #define EVDEV_MULTITOUCH */

#define MAX_EVENTS (256 * 4)

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

/* Relative axes */
#define REL_X                   0x00
#define REL_Y                   0x01

#define BTN_TOUCH 0x14a
#define BTN_MOUSE 0x110


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

#define TYPE_GOLDFISHEVDEV "goldfish-events"
#define GOLDFISHEVDEV(obj) \
    OBJECT_CHECK(GoldfishEvDevState, (obj), TYPE_GOLDFISHEVDEV)

typedef struct GoldfishEvDevState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    MemoryRegion iomem;
    qemu_irq irq;

    /* Device properties (TODO: actually make these props) */
    bool have_dpad;
    bool have_trackball;
    bool have_camera;
    bool have_keyboard;
    bool have_keyboard_lid;
    bool have_touch;
    bool have_multitouch;

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
} GoldfishEvDevState;

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

static const VMStateDescription vmstate_gf_evdev = {
    .name = "goldfish-events",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_INT32(page, GoldfishEvDevState),
        VMSTATE_UINT32_ARRAY(events, GoldfishEvDevState, MAX_EVENTS),
        VMSTATE_UINT32(first, GoldfishEvDevState),
        VMSTATE_UINT32(last, GoldfishEvDevState),
        VMSTATE_UINT32(state, GoldfishEvDevState),
        VMSTATE_UINT32(modifier_state, GoldfishEvDevState),
        VMSTATE_END_OF_LIST()
    }
};

static void enqueue_event(GoldfishEvDevState *s,
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

static unsigned dequeue_event(GoldfishEvDevState *s)
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

static int get_page_len(GoldfishEvDevState *s)
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

static int get_page_data(GoldfishEvDevState *s, int offset)
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

static uint64_t events_read(void *opaque, hwaddr offset, unsigned size)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;

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
                      "goldfish events device read: bad offset %x\n",
                      (int)offset);
        return 0;
    }
}

static void events_write(void *opaque, hwaddr offset,
                         uint64_t val, unsigned size)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;
    switch (offset) {
    case REG_SET_PAGE:
        s->page = val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "goldfish events device write: bad offset %x\n",
                      (int)offset);
        break;
    }
}

static const MemoryRegionOps gf_evdev_ops = {
    .read = events_read,
    .write = events_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void gf_evdev_put_mouse(void *opaque,
                               int dx, int dy, int dz, int buttons_state)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;

    /* Note that unlike the "classic" Android emulator, we don't
     * have the "dz == 0 for touchscreen, == 1 for trackball"
     * distinction.
     */
#ifdef EVDEV_MULTITOUCH
    if (s->have_multitouch) {
        /* Convert mouse event into multi-touch event */
        multitouch_update_pointer(MTES_MOUSE, 0, dx, dy,
                                  (buttons_state & 1) ? 0x81 : 0);
        return;
    }
#endif
    if (s->have_touch) {
        enqueue_event(s, EV_ABS, ABS_X, dx);
        enqueue_event(s, EV_ABS, ABS_Y, dy);
        enqueue_event(s, EV_ABS, ABS_Z, dz);
        enqueue_event(s, EV_KEY, BTN_TOUCH, buttons_state & 1);
        enqueue_event(s, EV_SYN, 0, 0);
        return;
    }
    if (s->have_trackball) {
        enqueue_event(s, EV_REL, REL_X, dx);
        enqueue_event(s, EV_REL, REL_Y, dy);
        enqueue_event(s, EV_SYN, 0, 0);
        return;
    }
}

/* set bits [bitl..bith] in the ev_bits[type] array
 */
static void
events_set_bits(GoldfishEvDevState *s, int type, int bitl, int bith)
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
events_set_bit(GoldfishEvDevState *s, int  type, int  bit)
{
    events_set_bits(s, type, bit, bit);
}

static void
events_clr_bit(GoldfishEvDevState *s, int type, int bit)
{
    int ii = bit / 8;
    if (ii < s->ev_bits[type].len) {
        uint8_t *bits = s->ev_bits[type].bits;
        uint8_t  mask = 0x01U << (bit & 7);
        bits[ii] &= ~mask;
    }
}

/* Keycode mappings to match the classic Android emulator as documented in
 * http://developer.android.com/tools/help/emulator.html
 */
static const uint16_t hardbutton_map[Q_KEY_CODE_MAX] = {
    [Q_KEY_CODE_HOME] = LINUX_KEY_HOME,
    [Q_KEY_CODE_F2] = LINUX_KEY_SOFT1, /* Android menu key */
    [Q_KEY_CODE_PGUP] = LINUX_KEY_SOFT1,
    [Q_KEY_CODE_PGDN] = LINUX_KEY_SOFT2,
    [Q_KEY_CODE_ESC] = LINUX_KEY_BACK,
    [Q_KEY_CODE_F3] = LINUX_KEY_SEND, /* dial */
    [Q_KEY_CODE_F4] = LINUX_KEY_END, /* end call */
    [Q_KEY_CODE_F5] = LINUX_KEY_SEARCH,
    [Q_KEY_CODE_F7] = LINUX_KEY_POWER,
    [Q_KEY_CODE_KP_ADD] = LINUX_KEY_VOLUMEUP,
    [Q_KEY_CODE_KP_SUBTRACT] = LINUX_KEY_VOLUMEDOWN,
};

static const uint16_t hardbutton_shift_map[Q_KEY_CODE_MAX] = {
    [Q_KEY_CODE_F2] = LINUX_KEY_SOFT2,
};

static const uint16_t hardbutton_control_map[Q_KEY_CODE_MAX] = {
    [Q_KEY_CODE_F5] = LINUX_KEY_VOLUMEUP,
    [Q_KEY_CODE_F6] = LINUX_KEY_VOLUMEDOWN,
    /* ctrl-kp5, ctrl-f3 -> LINUX_KEY_CAMERA (if have_camera) */
};

static const int dpad_map[Q_KEY_CODE_MAX] = {
    [Q_KEY_CODE_KP_4] = LINUX_KEY_LEFT,
    [Q_KEY_CODE_KP_6] = LINUX_KEY_RIGHT,
    [Q_KEY_CODE_KP_8] = LINUX_KEY_UP,
    [Q_KEY_CODE_KP_2] = LINUX_KEY_DOWN,
    [Q_KEY_CODE_KP_5] = LINUX_KEY_CENTER,
};

static void gf_evdev_handle_keyevent(DeviceState *dev, QemuConsole *src,
                                     InputEvent *evt)
{
    /* Handle a key event. Minimal implementation which just handles
     * the required hardware buttons and the dpad.
     * This should be extended to also honour have_keyboard, and
     * possibly also the control keys which affect the emulator itself.
     */

    GoldfishEvDevState *s = GOLDFISHEVDEV(dev);
    int qcode;
    int lkey = 0;
    int mod;

    assert(evt->kind == INPUT_EVENT_KIND_KEY);

    qcode = qemu_input_key_value_to_qcode(evt->key->key);

    /* Keep our modifier state up to date */
    switch (qcode) {
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
        if (evt->key->down) {
            s->modifier_state |= mod;
        } else {
            s->modifier_state &= ~mod;
        }
    }

    if (s->modifier_state & MODSTATE_ALT) {
        /* No alt-keys defined currently */
    } else if (s->modifier_state & MODSTATE_CTRL) {
        lkey = hardbutton_control_map[qcode];
    } else if (s->modifier_state & MODSTATE_SHIFT) {
        lkey = hardbutton_shift_map[qcode];
    } else {
        lkey = hardbutton_map[qcode];
    }

    if (!lkey && s->have_dpad && s->modifier_state == 0) {
        lkey = dpad_map[qcode];
    }

    if (lkey) {
        enqueue_event(s, EV_KEY, lkey, evt->key->down);
    }
}

static QemuInputHandler gf_evdev_key_input_handler = {
    .name = "goldfish event device key handler",
    .mask = INPUT_EVENT_MASK_KEY,
    .event = gf_evdev_handle_keyevent,
};

static void gf_evdev_init(Object *obj)
{
    GoldfishEvDevState *s = GOLDFISHEVDEV(obj);
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &gf_evdev_ops, s,
                          "goldfish-events", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    qemu_input_handler_register(dev, &gf_evdev_key_input_handler);
    qemu_add_mouse_event_handler(gf_evdev_put_mouse, s, 1, "goldfish-events");
}

static void gf_evdev_realize(DeviceState *dev, Error **errp)
{
    GoldfishEvDevState *s = GOLDFISHEVDEV(dev);

    /* now set the events capability bits depending on hardware configuration */
    /* apparently, the EV_SYN array is used to indicate which other
     * event classes to consider.
     */

    /* XXX PMM properties ? */
    s->name = "qwerty2";

    if (s->have_multitouch) {
        s->have_touch = true;
    }

    /* configure EV_KEY array
     *
     * All Android devices must have the following keys:
     *   KEY_HOME, KEY_BACK, KEY_SEND (Call), KEY_END (EndCall),
     *   KEY_SOFT1 (Menu), VOLUME_UP, VOLUME_DOWN
     *
     *   Note that previous models also had a KEY_SOFT2,
     *   and a KEY_POWER  which we still support here.
     *
     *   Newer models have a KEY_SEARCH key, which we always
     *   enable here.
     *
     * A Dpad will send: KEY_DOWN / UP / LEFT / RIGHT / CENTER
     *
     * The KEY_CAMERA button isn't very useful if there is no camera.
     *
     * BTN_MOUSE is sent when the trackball is pressed
     * BTN_TOUCH is sent when the touchscreen is pressed
     */
    events_set_bit(s, EV_SYN, EV_KEY);

    events_set_bit(s, EV_KEY, LINUX_KEY_HOME);
    events_set_bit(s, EV_KEY, LINUX_KEY_BACK);
    events_set_bit(s, EV_KEY, LINUX_KEY_SEND);
    events_set_bit(s, EV_KEY, LINUX_KEY_END);
    events_set_bit(s, EV_KEY, LINUX_KEY_SOFT1);
    events_set_bit(s, EV_KEY, LINUX_KEY_VOLUMEUP);
    events_set_bit(s, EV_KEY, LINUX_KEY_VOLUMEDOWN);
    events_set_bit(s, EV_KEY, LINUX_KEY_SOFT2);
    events_set_bit(s, EV_KEY, LINUX_KEY_POWER);
    events_set_bit(s, EV_KEY, LINUX_KEY_SEARCH);

    if (s->have_dpad) {
        events_set_bit(s, EV_KEY, LINUX_KEY_DOWN);
        events_set_bit(s, EV_KEY, LINUX_KEY_UP);
        events_set_bit(s, EV_KEY, LINUX_KEY_LEFT);
        events_set_bit(s, EV_KEY, LINUX_KEY_RIGHT);
        events_set_bit(s, EV_KEY, LINUX_KEY_CENTER);
    }

    if (s->have_trackball) {
        events_set_bit(s, EV_KEY, BTN_MOUSE);
    }
    if (s->have_touch) {
        events_set_bit(s, EV_KEY, BTN_TOUCH);
    }

    if (s->have_camera) {
        /* Camera emulation is enabled. */
        events_set_bit(s, EV_KEY, LINUX_KEY_CAMERA);
    }

    if (s->have_keyboard) {
        /* since we want to implement Unicode reverse-mapping
         * allow any kind of key, even those not available on
         * the skin.
         *
         * the previous code did set the [1..0x1ff] range, but
         * we don't want to enable certain bits in the middle
         * of the range that are registered for mouse/trackball/joystick
         * events.
         *
         * see "linux_keycodes.h" for the list of events codes.
         */
        events_set_bits(s, EV_KEY, 1, 0xff);
        events_set_bits(s, EV_KEY, 0x160, 0x1ff);

        /* If there is a keyboard, but no DPad, we need to clear the
         * corresponding bits. Doing this is simpler than trying to exclude
         * the DPad values from the ranges above.
         */
        if (!s->have_dpad) {
            events_clr_bit(s, EV_KEY, LINUX_KEY_DOWN);
            events_clr_bit(s, EV_KEY, LINUX_KEY_UP);
            events_clr_bit(s, EV_KEY, LINUX_KEY_LEFT);
            events_clr_bit(s, EV_KEY, LINUX_KEY_RIGHT);
            events_clr_bit(s, EV_KEY, LINUX_KEY_CENTER);
        }
    }

    /* configure EV_REL array
     *
     * EV_REL events are sent when the trackball is moved
     */
    if (s->have_trackball) {
        events_set_bit(s, EV_SYN, EV_REL);
        events_set_bits(s, EV_REL, REL_X, REL_Y);
    }

    /* configure EV_ABS array.
     *
     * EV_ABS events are sent when the touchscreen is pressed
     */
    if (s->have_touch) {
        ABSEntry *abs_values;

        events_set_bit(s, EV_SYN, EV_ABS);
        events_set_bits(s, EV_ABS, ABS_X, ABS_Z);
        /* Allocate the absinfo to report the min/max bounds for each
         * absolute dimension. The array must contain 3, or ABS_MAX tuples
         * of (min,max,fuzz,flat) 32-bit values.
         *
         * min and max are the bounds
         * fuzz corresponds to the device's fuziness, we set it to 0
         * flat corresponds to the flat position for JOEYDEV devices,
         * we also set it to 0.
         *
         * There is no need to save/restore this array in a snapshot
         * since the values only depend on the hardware configuration.
         */
        s->abs_info_count = s->have_multitouch ? ABS_MAX * 4 : 3 * 4;
        s->abs_info = g_new0(int32_t, s->abs_info_count);
        abs_values = (ABSEntry *)s->abs_info;

        /* QEMU provides absolute coordinates in the [0,0x7fff] range
         * regardless of the display resolution.
         */
        abs_values[ABS_X].max = 0x7fff;
        abs_values[ABS_Y].max = 0x7fff;
        abs_values[ABS_Z].max = 1;

#ifdef EVDEV_MULTITOUCH
        if (s->have_multitouch) {
            /*
             * Setup multitouch.
             */
            events_set_bit(s, EV_ABS, ABS_MT_SLOT);
            events_set_bit(s, EV_ABS, ABS_MT_POSITION_X);
            events_set_bit(s, EV_ABS, ABS_MT_POSITION_Y);
            events_set_bit(s, EV_ABS, ABS_MT_TRACKING_ID);
            events_set_bit(s, EV_ABS, ABS_MT_TOUCH_MAJOR);
            events_set_bit(s, EV_ABS, ABS_MT_PRESSURE);

            abs_values[ABS_MT_SLOT].max = multitouch_get_max_slot();
            abs_values[ABS_MT_TRACKING_ID].max
                = abs_values[ABS_MT_SLOT].max + 1;
            abs_values[ABS_MT_POSITION_X].max = abs_values[ABS_X].max;
            abs_values[ABS_MT_POSITION_Y].max = abs_values[ABS_Y].max;
            /* TODO : make next 2 less random */
            abs_values[ABS_MT_TOUCH_MAJOR].max = 0x7fffffff;
            abs_values[ABS_MT_PRESSURE].max = 0x100;
        }
#endif
    }

    /* configure EV_SW array
     *
     * EV_SW events are sent to indicate that the keyboard lid
     * was closed or opened (done when we switch layouts through
     * KP-7 or KP-9).
     *
     * We only support this when hw.keyboard.lid is true.
     */
    if (s->have_keyboard && s->have_keyboard_lid) {
        events_set_bit(s, EV_SYN, EV_SW);
        events_set_bit(s, EV_SW, 0);
    }
}

static void gf_evdev_reset(DeviceState *dev)
{
    GoldfishEvDevState *s = GOLDFISHEVDEV(dev);

    s->state = STATE_INIT;
    s->first = 0;
    s->last = 0;
    s->state = 0;
}

static Property gf_evdev_props[] = {
    DEFINE_PROP_BOOL("have-dpad", GoldfishEvDevState, have_dpad, false),
    DEFINE_PROP_BOOL("have-trackball", GoldfishEvDevState,
                     have_trackball, false),
    DEFINE_PROP_BOOL("have-camera", GoldfishEvDevState, have_camera, false),
    DEFINE_PROP_BOOL("have-keyboard", GoldfishEvDevState, have_keyboard, false),
    DEFINE_PROP_BOOL("have-lidswitch", GoldfishEvDevState,
                     have_keyboard_lid, false),
    DEFINE_PROP_BOOL("have-touch", GoldfishEvDevState,
                     have_touch, true),
    DEFINE_PROP_BOOL("have-multitouch", GoldfishEvDevState,
                     have_multitouch, false),
    DEFINE_PROP_END_OF_LIST()
};

static void gf_evdev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = gf_evdev_realize;
    dc->reset = gf_evdev_reset;
    dc->props = gf_evdev_props;
    dc->vmsd = &vmstate_gf_evdev;
}

static const TypeInfo gf_evdev_info = {
    .name = TYPE_GOLDFISHEVDEV,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GoldfishEvDevState),
    .instance_init = gf_evdev_init,
    .class_init = gf_evdev_class_init,
};

static void gf_evdev_register_types(void)
{
    type_register_static(&gf_evdev_info);
}

type_init(gf_evdev_register_types)
