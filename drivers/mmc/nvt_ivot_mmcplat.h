#ifndef __NVT_IVOT_MMCPLAT_H__
#define __NVT_IVOT_MMCPLAT_H__

#if (defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64))
#include "nvt_ivot_mmc_na51102.h"
#elif (defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64))
#include "nvt_ivot_mmc_ns02201.h"
#elif (defined(CONFIG_TARGET_NS02302) || defined(CONFIG_TARGET_NS02302_A64))
#include "nvt_ivot_mmc_ns02302.h"
#endif

#endif
