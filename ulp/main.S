#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

	.bss

	.global gpio_status

gpio_status:
	.long 0

	.text

exit:
	halt

wakeup:
	/* try to wakeup SoC, if not => halt */
	STAGE_INC  1
	JUMPS exit, 10, GT

	READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
	AND r0, r0, 1
	JUMP wakeup, eq
	wake

	.global entry
entry:
	STAGE_RST

	READ_RTC_REG (RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)
	move r1, gpio_status
	st r0, r1, 0

	move r1, r0
	rsh r1, r1, 4
	and r1, r1, 1

	jump wakeup, eq

	move r1, r0
	rsh r1, r1, 3
	and r1, r1, 1

	jump wakeup, eq

	move r1, r0
	rsh r1, r1, 5
	and r1, r1, 1

	jump wakeup, eq
	halt
