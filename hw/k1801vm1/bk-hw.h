#ifndef _BK_0010_HW_H_
#define _BK_0010_HW_H_


#include <bits/stdint-uintn.h>
#include "exec/hwaddr.h"


#define RAM_NAME                "bk0010.ram"
#define RAM_BASE                0000000
#define RAM_SIZE                0040000

#define VIDEO_NAME              "bk0010.display"
#define VIDEO_BASE              0040000
#define VIDEO_SIZE              0100000 - VIDEO_BASE

#define MONITOR_ROM_NAME        "bk0010.monitor-rom"
#define MONITOR_ROM_BASE        0100000
#define MONITOR_ROM_SIZE        0120000 - MONITOR_ROM_BASE
#define MONITOR_ROM_FILENAME    "rom/monit10.rom"

#define BASIC_ROM_NAME          "bk0010.basic-rom"
#define BASIC_ROM_BASE          0120000
#define BASIC_ROM_SIZE          0200000 - BASIC_ROM_BASE
#define BASIC_ROM_FILENAME      "rom/basic10.rom"

#define SYSREGS_NAME            "bk0010.sysregs"
#define SYSREGS_BASE            0177600
#define SYSREGS_SIZE            0177777 - SYSREGS_BASE

#define CPU_INTERRUPT_KEYBOARD  CPU_INTERRUPT_TGT_EXT_0


extern void bk_display_init(void);
extern void bk_sysregs_init(void);
extern void bk_keyboard_init(void);


#endif
