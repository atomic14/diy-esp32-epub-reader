#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

	.bss

	.global gpio_status

gpio_status:
	.long 0

	.text

	.global up_mask

up_mask:
	.long 0

	.global down_mask

down_mask:
	.long 0

	.global select_mask

select_mask:
	.long 0

exit:
	halt

wakeup:
	// r1 should contain the bitmask of the pin that woke us up
	MOVE r0, gpio_status
	ST r1, r0, 0
try_wakeup:
	/* try to wakeup SoC, if not => halt */
	STAGE_INC  1
	JUMPS exit, 10, GT

	READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
	AND r0, r0, 1
	JUMP try_wakeup, eq
	wake

	.global entry
entry:
	STAGE_RST

  // clear the hold bit on GPIO2
  WRITE_RTC_REG(RTC_IO_TOUCH_PAD2_REG, RTC_IO_TOUCH_PAD2_HOLD_S, 1, 0)
	// make sure it is on
  WRITE_RTC_REG(RTC_GPIO_OUT_REG,RTC_GPIO_OUT_DATA_S+12,1,1)
  // set the hold bit on GPIO2
  WRITE_RTC_REG(RTC_IO_TOUCH_PAD2_REG, RTC_IO_TOUCH_PAD2_HOLD_S, 1, 1)

	// this will read the first 16 bits of the RTC GPIO pins
	// if you need the higher 16 bits then you'll need to adjust
	// this
	READ_RTC_REG (RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)
	move r2, r0
	move r1, up_mask
	ld r1, r1, 0
	and r0, r0, r1
	jump wakeup, eq

	move r0, r2
	move r1, down_mask
	ld r1, r1, 0
	and r0, r0, r1
	jump wakeup, eq

	move r0, r2
	move r1, select_mask
	ld r1, r1, 0
	and r0, r0, r1
	jump wakeup, eq

	halt
