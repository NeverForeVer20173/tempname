#ifndef APPLICATIONS_CAPSENSE_KEY_H_
#define APPLICATIONS_CAPSENSE_KEY_H_

#include <rtthread.h>
#include "drv_common.h"
#include "cy_capsense_structure.h"
#include "cycfg_capsense.h"

#ifdef BSP_USING_SLIDER
#include "cycfg_capsense.h"
#include "cy_capsense.h"
#include "cybsp.h"
#endif

extern uint8_t  slider_touch_prev;
extern uint16_t slider_pos_prev_x;
extern rt_mailbox_t msg_cs;

void capsense_callback(cy_stc_active_scan_sns_t *ptr);
int CapsenseKey_Init(void);
void CapsenseKey_Process(void);
int CapsenseKeySample_Init(void);
int button_init(void);

#endif /* APPLICATIONS_CAPSENSE_KEY_H_ */
