//
// Created by u137524 on 01/12/2022.
//

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <openxr/openxr_platform.h>

namespace Device {
    enum eCPUGPUPower : int {
        LVL_0_POWER_SAVING = XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT,
        LVL_1_LOW_POWER = XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT,
        LVL_2_HIGH_POWER = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT,
        LVL_3_BOOST = XR_PERF_SETTINGS_LEVEL_BOOST_EXT
    };
}

#endif //__DEVICE_H__
