// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Core->OpenXDK.cpp
// *
// *  This file is part of the Cxbx project.
// *
// *  Cxbx and Cxbe are free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Bostom, MA 02111-1307, USA.
// *
// *  (c) 2002-2003 Aaron Robinson <caustik@caustik.com>
// *
// *  All rights reserved
// *
// ******************************************************************
#include "Xbe.h"

// OpenXDK logo bitmap
uint08 OpenXDK[] = {
    0x5A, 0x06, 0x23, 0x49, 0x13, 0x0F, 0x33, 0x49, 0x13, 0x0F, 0x13, 0x4F, 0x33, 0x0D, 0x13, 0x49, 0x23, 0x46, 0x00,
    0x23, 0x4D, 0x33, 0x0D, 0x13, 0x43, 0x22, 0x00, 0x43, 0x13, 0x22, 0x00, 0xC3, 0x22, 0xF0, 0xC3, 0x05, 0x33, 0xD3,
    0x22, 0xF0, 0x83, 0x09, 0x73, 0x2A, 0xF0, 0x07, 0x83, 0x22, 0xF0, 0xD3, 0x33, 0x03, 0x73, 0xC3, 0x33, 0x0B, 0x33,
    0xC3, 0x73, 0x05, 0x73, 0x22, 0xF0, 0xE3, 0x63, 0x07, 0x43, 0xF3, 0x22, 0x00, 0xF3, 0x43, 0x22, 0x00, 0xF3, 0x73,
    0x33, 0x09, 0x33, 0x73, 0xF3, 0x05, 0x43, 0xF3, 0x43, 0x23, 0x07, 0x13, 0x43, 0x93, 0xB3, 0x09, 0x73, 0xC3, 0x13,
    0x0D, 0x45, 0x07, 0xB3, 0x93, 0x43, 0x09, 0x23, 0x43, 0xF3, 0x43, 0x03, 0x13, 0xA3, 0xF3, 0x73, 0x07, 0x73, 0xF3,
    0xA3, 0x13, 0x05, 0x73, 0xB3, 0x0B, 0x13, 0x43, 0xC3, 0x73, 0x07, 0x43, 0xF3, 0x22, 0x00, 0xF3, 0x43, 0x22, 0x00,
    0xF3, 0x43, 0x0D, 0x43, 0xF3, 0x23, 0x73, 0x93, 0xF3, 0x7F, 0xB5, 0x05, 0x75, 0xB3, 0xD3, 0x7D, 0x63, 0x0B, 0xB3,
    0x73, 0x0F, 0xF3, 0x43, 0x07, 0x53, 0xE3, 0xC3, 0x63, 0xC3, 0xE3, 0x53, 0x09, 0x73, 0xB3, 0x0F, 0xB3, 0x73, 0x03,
    0x63, 0x73, 0x93, 0xF3, 0x7B, 0x83, 0xB3, 0xD3, 0xF3, 0x43, 0x22, 0x00, 0xF3, 0x43, 0x0D, 0x43, 0xF3, 0x33, 0xB3,
    0xC3, 0xF3, 0xBD, 0xA3, 0x73, 0x63, 0x05, 0xB5, 0xD3, 0xE3, 0xBD, 0x83, 0x0B, 0xB3, 0x73, 0x0F, 0xF3, 0x43, 0x09,
    0x13, 0xE3, 0xF3, 0xE3, 0x13, 0x0B, 0x73, 0xB3, 0x0F, 0xB3, 0x73, 0x03, 0x83, 0xB3, 0xC3, 0xF3, 0xBB, 0xD3, 0xF5,
    0xB3, 0x23, 0x22, 0x00, 0xF3, 0x43, 0x0D, 0x43, 0xF3, 0x05, 0x43, 0xF3, 0x36, 0x00, 0x73, 0xB3, 0x32, 0x00, 0xB3,
    0x73, 0x0F, 0xF3, 0x43, 0x07, 0x33, 0xC3, 0xE3, 0x73, 0xE3, 0xC3, 0x33, 0x09, 0x73, 0xB3, 0x0F, 0xB3, 0x73, 0x07,
    0x43, 0xF3, 0x0D, 0x13, 0x43, 0xF3, 0x43, 0x22, 0x00, 0xF3, 0xC3, 0xA3, 0x79, 0xA3, 0xC3, 0xF3, 0x05, 0x43, 0xF3,
    0x36, 0x00, 0x73, 0xE3, 0x93, 0x7B, 0x93, 0xB3, 0xC3, 0x07, 0xB3, 0x73, 0x0F, 0xF3, 0x43, 0x05, 0x73, 0xF3, 0xA3,
    0x13, 0x03, 0x13, 0xA3, 0xF3, 0x73, 0x07, 0x73, 0xD3, 0x7B, 0x83, 0xB3, 0xE3, 0x73, 0x07, 0x43, 0xF3, 0x22, 0x00,
    0xF3, 0x43, 0x22, 0x00, 0x53, 0x73, 0x83, 0xB9, 0x93, 0x73, 0x53, 0x05, 0x33, 0xB3, 0x36, 0x00, 0x43, 0x73, 0xA3,
    0xBD, 0x83, 0x73, 0x07, 0x83, 0x63, 0x0F, 0xB3, 0x33, 0x03, 0x63, 0xE3, 0x53, 0x0B, 0x53, 0xE3, 0x63, 0x05, 0x63,
    0xBF, 0x75, 0x23, 0x07, 0x33, 0xB3, 0x22, 0x00, 0xB3, 0x33, 0xFA, 0x00, 0x13, 0x0F, 0x13, 0xBE, 0x06, 0x03,
};

// size, in bytes, of the OpenXDK logo bitmap
uint32 dwSizeOfOpenXDK = 0x0000017B;