/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Bernd Herzog
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ch.h"
#include "hal.h"

#include "proc_sd_over_usb.hpp"

#include "debug.hpp"
#include "portapack_shared_memory.hpp"

extern "C" {
void start_usb(void);
void stop_usb(void);
void irq_usb(void);
void usb_transfer(void);

CH_IRQ_HANDLER(Vector60) {
	irq_usb();
}
}

//uint8_t buf[512];

int main() {

	sdcStart(&SDCD1, nullptr);
	if (sdcConnect(&SDCD1) == CH_FAILED) chDbgPanic("no sd card #1");

	// memset(&buf[0], 0, 512);
	// if (sdcRead(&SDCD1, 0, &buf[0], 1) == CH_FAILED) chDbgPanic("no sd card #2");


	start_usb();

	// memset(&buf[0], 0, 512);
	// if (sdcRead(&SDCD1, 0, &buf[0], 1) == CH_FAILED) chDbgPanic("no sd card #3");

	//event_dispatcher.run();
	while (true) {
		usb_transfer();
	}
	stop_usb();
	return 0;
}
