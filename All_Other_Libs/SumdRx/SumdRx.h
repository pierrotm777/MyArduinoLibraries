/****************************************************************************
 *
 *   Copyright (c) 2015 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file sumd.h
 *
 * RC protocol definition for Graupner HoTT transmitter
 *
 * @author Marco Bauer <marco@wtns.de>
 */
 
#ifndef SUMDRX_H
#define SUMDRX_H

#include "Arduino.h"

#pragma once

#include <stdint.h>


#define SUMD_MAX_CHANNELS	32
#define SUMD_HEADER_LENGTH	3
#define SUMD_HEADER_ID		0xA8
#define SUMD_ID_SUMH		0x00
#define SUMD_ID_SUMD		0x01
#define SUMD_ID_FAILSAFE	0x81




#pragma pack(push, 1)
typedef struct {
	uint8_t	header;							///< 0xA8 for a valid packet
	uint8_t	status;							///< 0x01 valid and live SUMD data frame / 0x00 = SUMH / 0x81 = Failsafe
	uint8_t	length;							///< Channels
	uint8_t	sumd_data[SUMD_MAX_CHANNELS * 2];	///< ChannelData (High Byte/ Low Byte)
	uint8_t	crc16_high;						///< High Byte of 16 Bit CRC
	uint8_t	crc16_low;						///< Low Byte of 16 Bit CRC
	uint8_t	telemetry;						///< Telemetry request
	uint8_t	crc8;							///< SUMH CRC8
} ReceiverFcPacketHoTT;
#pragma pack(pop)


/**
 * CRC16 implementation for SUMD protocol
 *
 * @param crc Initial CRC Value
 * @param value to accumulate in the checksum
 * @return the checksum
 */
uint16_t sumd_crc16(uint16_t crc, uint8_t value);

/**
 * CRC8 implementation for SUMH protocol
 *
 * @param crc Initial CRC Value
 * @param value to accumulate in the checksum
 * @return the checksum
 */
uint8_t sumd_crc8(uint8_t crc, uint8_t value);

/**
 * Decoder for SUMD/SUMH protocol
 *
 * @param byte current char to read
 * @param rssi pointer to a byte where the RSSI value is written back to
 * @param rx_count pointer to a byte where the receive count of packets signce last wireless frame is written back to
 * @param channels pointer to a datastructure of size max_chan_count where channel values (12 bit) are written back to
 * @param max_chan_count maximum channels to decode - if more channels are decoded, the last n are skipped and success (0) is returned
 * @return 0 for success (a decoded packet), 1 for no packet yet (accumulating), 2 for unknown packet, 3 for out of sync, 4 for checksum error
 */
/*
__EXPORT int sumd_decode(uint8_t byte, uint8_t *rssi, uint8_t *rx_count, uint16_t *channel_count,
				 uint16_t *channels, uint16_t max_chan_count);
*/
int sumd_decode(uint8_t byte, uint8_t *rssi, uint8_t *rx_count, uint16_t *channel_count,
			 uint16_t *channels, uint16_t max_chan_count);



#endif