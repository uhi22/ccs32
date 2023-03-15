/*
 * Copyright (C) 2007-2018 Siemens AG
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************
 *
 * @author Daniel.Peintner.EXT@siemens.com
 * @version 2017-03-02 
 * @contact Richard.Kuntschke@siemens.com
 *
 * <p>Code generated by EXIdizer</p>
 * <p>Schema: V2G_CI_MsgDef.xsd</p>
 *
 *
 ********************************************************************/



#ifndef METHODS_BAG_C
#define METHODS_BAG_C

#include "MethodsBag.h"
#include "ErrorCodes.h"

static const uint16_t smallLengths[] = { 0, 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4,
		4, 4, 4 };

int exiGetCodingLength(size_t characteristics, size_t* codingLength) {
	/* Note: we could use range expressions in switch statements but those are non-standard */
	/* e.g., case 1 ... 5: */
	int errn = 0;
	if (characteristics < 17) {
		*codingLength = smallLengths[characteristics];
	} else if (characteristics < 33) {
		/* 17 .. 32 */
		*codingLength = 5;
	} else if (characteristics < 65) {
		/* 33 .. 64 */
		*codingLength = 6;
	} else if (characteristics < 129) {
		/* 65 .. 128 */
		*codingLength = 7;
	} else if (characteristics < 257) {
		/* 129 .. 256 */
		*codingLength = 8;
	} else if (characteristics < 513) {
		/* 257 .. 512 */
		*codingLength = 9;
	} else if (characteristics < 1025) {
		/* 513 .. 1024 */
		*codingLength = 10;
	} else if (characteristics < 2049) {
		/* 1025 .. 2048 */
		*codingLength = 11;
	} else if (characteristics < 4097) {
		/* 2049 .. 4096 */
		*codingLength = 12;
	} else if (characteristics < 8193) {
		/* 4097 .. 8192 */
		*codingLength = 13;
	} else if (characteristics < 16385) {
		/* 8193 .. 16384 */
		*codingLength = 14;
	} else if (characteristics < 32769) {
		/* 16385 .. 32768 */
		*codingLength = 15;
	} else {
		/* 32769 .. 65536 */
		*codingLength = 16;
	}
	return errn;
}


uint8_t numberOf7BitBlocksToRepresent(uint32_t n) {
	/* assert (n >= 0); */

	/* 7 bits */
	if (n < 128) {
		return 1;
	}
	/* 14 bits */
	else if (n < 16384) {
		return 2;
	}
	/* 21 bits */
	else if (n < 2097152) {
		return 3;
	}
	/* 28 bits */
	else if (n < 268435456) {
		return 4;
	}
	/* 35 bits */
	else {
		/* int, 32 bits */
		return 5;
	}
}



#endif

