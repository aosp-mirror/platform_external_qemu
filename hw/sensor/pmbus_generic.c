/*
 * A Generic PMBus sensor that will try to respond to all standard commands
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/i2c/pmbus_device.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qemu/log.h"
#include "qom/object.h"

#define TYPE_PMBUS_GENERIC "pmbus-generic"
OBJECT_DECLARE_SIMPLE_TYPE(PMBusGenericState, PMBUS_GENERIC)

#define DEFAULT_OPERATION       0x80
#define DEFAULT_CAPABILITY      0x20
#define PB_GENERIC_NUM_PAGES    (PB_MAX_PAGES + 1)

typedef struct PMBusGenericState {
    PMBusDevice parent;
} PMBusGenericState;

static uint8_t pmbus_generic_read_byte(PMBusDevice *pmdev)
{
    qemu_log_mask(LOG_UNIMP,
                  "%s: %s is reading from an unimplemented register: 0x%02x\n",
                  __func__, DEVICE(pmdev)->canonical_path, pmdev->code);
    return 0xFF;
}

static int pmbus_generic_write_data(PMBusDevice *pmdev, const uint8_t *buf,
                              uint8_t len)
{
    qemu_log_mask(LOG_UNIMP,
                  "%s: %s is writing to an unimplemented register: 0x%02x\n",
                  __func__, DEVICE(pmdev)->canonical_path, pmdev->code);
    return 0;
}

static void pmbus_generic_exit_reset(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);

    pmdev->capability = DEFAULT_CAPABILITY;

    for (int i = 0; i < PB_GENERIC_NUM_PAGES; i++) {
        pmdev->pages[i].operation = DEFAULT_OPERATION;
        /* TODO: set any registers with expected non-zero defaults */
    }
}

static const VMStateDescription vmstate_pmbus_generic = {
    .name = TYPE_PMBUS_GENERIC,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]){
        VMSTATE_PMBUS_DEVICE(parent, PMBusGenericState),
        VMSTATE_END_OF_LIST()
    }
};

static void pmbus_generic_get16(Object *obj, Visitor *v, const char *name,
                              void *opaque, Error **errp)
{
    uint16_t value = *(uint16_t *)opaque;
    visit_type_uint16(v, name, &value, errp);
}

static void pmbus_generic_get8(Object *obj, Visitor *v, const char *name,
                              void *opaque, Error **errp)
{
    uint8_t value = *(uint8_t *)opaque;
    visit_type_uint8(v, name, &value, errp);
}

static void pmbus_generic_set16(Object *obj, Visitor *v, const char *name,
                              void *opaque, Error **errp)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint16_t *internal = opaque;
    uint16_t value;

    if (!visit_type_uint16(v, name, &value, errp)) {
        return;
    } else {
        *internal = value;
    }

    pmbus_check_limits(pmdev);
}

static void pmbus_generic_set8(Object *obj, Visitor *v, const char *name,
                              void *opaque, Error **errp)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint8_t *internal = opaque;
    uint8_t value;

    if (!visit_type_uint8(v, name, &value, errp)) {
        return;
    } else if (strcmp(name, "num_pages") == 0) {
        *internal = value < PB_GENERIC_NUM_PAGES ? value : PB_GENERIC_NUM_PAGES;
    } else {
        *internal = value;
    }

    pmbus_check_limits(pmdev);
}

static void pmbus_generic_init(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint64_t flags =  PB_HAS_COEFFICIENTS | PB_HAS_VIN | PB_HAS_VOUT |
                      PB_HAS_VOUT_MARGIN | PB_HAS_VIN_RATING |
                      PB_HAS_VOUT_RATING | PB_HAS_VOUT_MODE | PB_HAS_IOUT |
                      PB_HAS_IIN | PB_HAS_IOUT_RATING | PB_HAS_IIN_RATING |
                      PB_HAS_IOUT_GAIN | PB_HAS_POUT | PB_HAS_PIN | PB_HAS_EIN |
                      PB_HAS_EOUT | PB_HAS_POUT_RATING | PB_HAS_PIN_RATING |
                      PB_HAS_TEMPERATURE | PB_HAS_TEMP2 | PB_HAS_TEMP3 |
                      PB_HAS_TEMP_RATING | PB_HAS_FAN | PB_HAS_MFR_INFO;

    for (int i = 0; i < PB_GENERIC_NUM_PAGES; i++) {
        pmbus_page_config(pmdev, i, flags);

        object_property_add(obj, "operation[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].operation);

        object_property_add(obj, "on_off_config[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].on_off_config);

        object_property_add(obj, "write_protect[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].write_protect);

        object_property_add(obj, "phase[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].phase);

        object_property_add(obj, "vout_mode[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].vout_mode);

        object_property_add(obj, "vout_command[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_command);

        object_property_add(obj, "vout_trim[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_trim);

        object_property_add(obj, "vout_cal_offset[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_cal_offset);

        object_property_add(obj, "vout_max[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_max);

        object_property_add(obj, "vout_margin_high[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_margin_high);

        object_property_add(obj, "vout_margin_low[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_margin_low);

        object_property_add(obj, "vout_transition_rate[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_transition_rate);

        object_property_add(obj, "vout_droop[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_droop);

        object_property_add(obj, "vout_scale_loop[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_scale_loop);

        object_property_add(obj, "vout_scale_monitor[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_scale_monitor);

        object_property_add(obj, "vout_min[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_min);

        object_property_add(obj, "pout_max[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].pout_max);

        object_property_add(obj, "max_duty[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].max_duty);

        object_property_add(obj, "frequency_switch[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].frequency_switch);

        object_property_add(obj, "vin_on[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vin_on);

        object_property_add(obj, "vin_off[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vin_off);

        object_property_add(obj, "iout_cal_gain[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].iout_cal_gain);

        object_property_add(obj, "iout_cal_offset[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].iout_cal_offset);

        object_property_add(obj, "fan_config_1_2[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].fan_config_1_2);

        object_property_add(obj, "fan_command_1[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].fan_command_1);

        object_property_add(obj, "fan_command_2[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].fan_command_2);

        object_property_add(obj, "fan_config_3_4[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].fan_config_3_4);

        object_property_add(obj, "fan_command_3[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].fan_command_3);

        object_property_add(obj, "fan_command_4[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].fan_command_4);

        object_property_add(obj, "vout_ov_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_ov_fault_limit);

        object_property_add(obj, "vout_ov_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].vout_ov_fault_response);

        object_property_add(obj, "vout_ov_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_ov_warn_limit);

        object_property_add(obj, "vout_uv_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_uv_warn_limit);

        object_property_add(obj, "vout_uv_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vout_uv_fault_limit);

        object_property_add(obj, "vout_uv_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].vout_uv_fault_response);

        object_property_add(obj, "iout_oc_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].iout_oc_fault_limit);

        object_property_add(obj, "iout_oc_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].iout_oc_fault_response);

        object_property_add(obj, "iout_oc_lv_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].iout_oc_lv_fault_limit);

        object_property_add(obj, "iout_oc_lv_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].iout_oc_lv_fault_response);

        object_property_add(obj, "iout_oc_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].iout_oc_warn_limit);

        object_property_add(obj, "iout_uc_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].iout_uc_fault_limit);

        object_property_add(obj, "iout_uc_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].iout_uc_fault_response);

        object_property_add(obj, "ot_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].ot_fault_limit);

        object_property_add(obj, "ot_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].ot_fault_response);

        object_property_add(obj, "ot_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].ot_warn_limit);

        object_property_add(obj, "ut_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].ut_warn_limit);

        object_property_add(obj, "ut_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].ut_fault_limit);

        object_property_add(obj, "ut_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].ut_fault_response);

        object_property_add(obj, "vin_ov_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vin_ov_fault_limit);

        object_property_add(obj, "vin_ov_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].vin_ov_fault_response);

        object_property_add(obj, "vin_ov_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vin_ov_warn_limit);

        object_property_add(obj, "vin_uv_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vin_uv_warn_limit);

        object_property_add(obj, "vin_uv_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].vin_uv_fault_limit);

        object_property_add(obj, "vin_uv_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].vin_uv_fault_response);

        object_property_add(obj, "iin_oc_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].iin_oc_fault_limit);

        object_property_add(obj, "iin_oc_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].iin_oc_fault_response);

        object_property_add(obj, "iin_oc_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].iin_oc_warn_limit);

        object_property_add(obj, "power_good_on[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].power_good_on);

        object_property_add(obj, "power_good_off[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].power_good_off);

        object_property_add(obj, "ton_delay[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].ton_delay);

        object_property_add(obj, "ton_rise[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].ton_rise);

        object_property_add(obj, "ton_max_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].ton_max_fault_limit);

        object_property_add(obj, "ton_max_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].ton_max_fault_response);

        object_property_add(obj, "toff_delay[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].toff_delay);

        object_property_add(obj, "toff_fall[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].toff_fall);

        object_property_add(obj, "toff_max_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].toff_max_warn_limit);

        object_property_add(obj, "pout_op_fault_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].pout_op_fault_limit);

        object_property_add(obj, "pout_op_fault_response[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].pout_op_fault_response);

        object_property_add(obj, "pout_op_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].pout_op_warn_limit);

        object_property_add(obj, "pin_op_warn_limit[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].pin_op_warn_limit);

        object_property_add(obj, "status_word[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].status_word);

        object_property_add(obj, "status_vout[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].status_vout);

        object_property_add(obj, "status_iout[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].status_iout);

        object_property_add(obj, "status_input[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].status_input);

        object_property_add(obj, "status_temperature[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].status_temperature);

        object_property_add(obj, "status_cml[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].status_cml);

        object_property_add(obj, "status_other[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].status_other);

        object_property_add(obj, "status_mfr_specific[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].status_mfr_specific);

        object_property_add(obj, "status_fans_1_2[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].status_fans_1_2);

        object_property_add(obj, "status_fans_3_4[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].status_fans_3_4);

        object_property_add(obj, "read_vin[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_vin);

        object_property_add(obj, "read_iin[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_iin);

        object_property_add(obj, "read_vcap[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_vcap);

        object_property_add(obj, "read_vout[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_vout);

        object_property_add(obj, "read_iout[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_iout);

        object_property_add(obj, "read_temperature_1[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_temperature_1);

        object_property_add(obj, "read_temperature_2[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_temperature_2);

        object_property_add(obj, "read_temperature_3[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_temperature_3);

        object_property_add(obj, "read_fan_speed_1[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_fan_speed_1);

        object_property_add(obj, "read_fan_speed_2[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_fan_speed_2);

        object_property_add(obj, "read_fan_speed_3[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_fan_speed_3);

        object_property_add(obj, "read_fan_speed_4[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_fan_speed_4);

        object_property_add(obj, "read_duty_cycle[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_duty_cycle);

        object_property_add(obj, "read_frequency[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_frequency);

        object_property_add(obj, "read_pout[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_pout);

        object_property_add(obj, "read_pin[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].read_pin);

        object_property_add(obj, "revision[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].revision);

        object_property_add(obj, "mfr_vin_min[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_vin_min);

        object_property_add(obj, "mfr_vin_max[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_vin_max);

        object_property_add(obj, "mfr_iin_max[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_iin_max);

        object_property_add(obj, "mfr_pin_max[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_pin_max);

        object_property_add(obj, "mfr_vout_min[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_vout_min);

        object_property_add(obj, "mfr_vout_max[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_vout_max);

        object_property_add(obj, "mfr_iout_max[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_iout_max);

        object_property_add(obj, "mfr_pout_max[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_pout_max);

        object_property_add(obj, "mfr_tambient_max[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_tambient_max);

        object_property_add(obj, "mfr_tambient_min[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_tambient_min);

        object_property_add(obj, "mfr_pin_accuracy[*]", "uint8",
                            pmbus_generic_get8, pmbus_generic_set8, NULL,
                            &pmdev->pages[i].mfr_pin_accuracy);

        object_property_add(obj, "mfr_max_temp_1[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_max_temp_1);

        object_property_add(obj, "mfr_max_temp_2[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_max_temp_2);

        object_property_add(obj, "mfr_max_temp_3[*]", "uint16",
                            pmbus_generic_get16, pmbus_generic_set16, NULL,
                            &pmdev->pages[i].mfr_max_temp_3);

    }

    object_property_add(obj, "num_pages", "uint8",
                        pmbus_generic_get8, pmbus_generic_set8,
                        NULL, &pmdev->num_pages);
}

static void pmbus_generic_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    PMBusDeviceClass *k = PMBUS_DEVICE_CLASS(klass);

    dc->desc = "Generic 32 page PMBus Sensor";
    dc->vmsd = &vmstate_pmbus_generic;
    k->write_data = pmbus_generic_write_data;
    k->receive_byte = pmbus_generic_read_byte;
    k->device_num_pages = PB_GENERIC_NUM_PAGES;
    rc->phases.exit = pmbus_generic_exit_reset;
}

static const TypeInfo pmbus_generic_info = {
    .name = TYPE_PMBUS_GENERIC,
    .parent = TYPE_PMBUS_DEVICE,
    .instance_size = sizeof(PMBusGenericState),
    .instance_init = pmbus_generic_init,
    .class_init = pmbus_generic_class_init,
};

static void pmbus_generic_register_types(void)
{
    type_register_static(&pmbus_generic_info);
}

type_init(pmbus_generic_register_types)
