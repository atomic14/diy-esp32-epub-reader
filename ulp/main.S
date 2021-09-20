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

wakeup_pin4:
	MOVE r0, 4
	MOVE r1, gpio_status
	ST r0, r1, 0
	JUMP wakeup

wakeup_pin3:
	MOVE r0, 3
	MOVE r1, gpio_status
	ST r0, r1, 0
	JUMP wakeup

wakeup_pin5:
	MOVE r0, 5
	MOVE r1, gpio_status
	ST r0, r1, 0
	JUMP wakeup

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

	READ_RTC_REG (RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S+4, 1)
	and r0, r0, 1
	jump wakeup_pin4, eq


	READ_RTC_REG (RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S+3, 1)
	and r0, r0, 1
	jump wakeup_pin3, eq

	READ_RTC_REG (RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S+5, 1)
	and r0, r0, 1
	jump wakeup_pin5, eq

	halt
