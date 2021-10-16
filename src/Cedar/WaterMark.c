﻿// IPA-DN-Ultra Library Source Code
// 
// License: The Apache License, Version 2.0
// https://www.apache.org/licenses/LICENSE-2.0
// 
// Copyright (c) IPA CyberLab of Industrial Cyber Security Center.
// Copyright (c) NTT-East Impossible Telecom Mission Group.
// Copyright (c) Daiyuu Nobori.
// Copyright (c) SoftEther VPN Project, University of Tsukuba, Japan.
// Copyright (c) SoftEther Corporation.
// Copyright (c) all contributors on IPA-DN-Ultra Library and SoftEther VPN Project in GitHub.
// 
// All Rights Reserved.
// 
// DISCLAIMER
// ==========
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// 
// THIS SOFTWARE IS DEVELOPED IN JAPAN, AND DISTRIBUTED FROM JAPAN, UNDER
// JAPANESE LAWS. YOU MUST AGREE IN ADVANCE TO USE, COPY, MODIFY, MERGE, PUBLISH,
// DISTRIBUTE, SUBLICENSE, AND/OR SELL COPIES OF THIS SOFTWARE, THAT ANY
// JURIDICAL DISPUTES WHICH ARE CONCERNED TO THIS SOFTWARE OR ITS CONTENTS,
// AGAINST US (IPA, NTT-EAST, SOFTETHER PROJECT, SOFTETHER CORPORATION, DAIYUU NOBORI
// OR OTHER SUPPLIERS), OR ANY JURIDICAL DISPUTES AGAINST US WHICH ARE CAUSED BY ANY
// KIND OF USING, COPYING, MODIFYING, MERGING, PUBLISHING, DISTRIBUTING, SUBLICENSING,
// AND/OR SELLING COPIES OF THIS SOFTWARE SHALL BE REGARDED AS BE CONSTRUED AND
// CONTROLLED BY JAPANESE LAWS, AND YOU MUST FURTHER CONSENT TO EXCLUSIVE
// JURISDICTION AND VENUE IN THE COURTS SITTING IN TOKYO, JAPAN. YOU MUST WAIVE
// ALL DEFENSES OF LACK OF PERSONAL JURISDICTION AND FORUM NON CONVENIENS.
// PROCESS MAY BE SERVED ON EITHER PARTY IN THE MANNER AUTHORIZED BY APPLICABLE
// LAW OR COURT RULE.
// 
// USE ONLY IN JAPAN. DO NOT USE THIS SOFTWARE IN ANOTHER COUNTRY UNLESS YOU HAVE
// A CONFIRMATION THAT THIS SOFTWARE DOES NOT VIOLATE ANY CRIMINAL LAWS OR CIVIL
// RIGHTS IN THAT PARTICULAR COUNTRY. USING THIS SOFTWARE IN OTHER COUNTRIES IS
// COMPLETELY AT YOUR OWN RISK. IPA AND NTT-EAST HAS DEVELOPED AND
// DISTRIBUTED THIS SOFTWARE TO COMPLY ONLY WITH THE JAPANESE LAWS AND EXISTING
// CIVIL RIGHTS INCLUDING PATENTS WHICH ARE SUBJECTS APPLY IN JAPAN. OTHER
// COUNTRIES' LAWS OR CIVIL RIGHTS ARE NONE OF OUR CONCERNS NOR RESPONSIBILITIES.
// WE HAVE NEVER INVESTIGATED ANY CRIMINAL REGULATIONS, CIVIL LAWS OR
// INTELLECTUAL PROPERTY RIGHTS INCLUDING PATENTS IN ANY OF OTHER 200+ COUNTRIES
// AND TERRITORIES. BY NATURE, THERE ARE 200+ REGIONS IN THE WORLD, WITH
// DIFFERENT LAWS. IT IS IMPOSSIBLE TO VERIFY EVERY COUNTRIES' LAWS, REGULATIONS
// AND CIVIL RIGHTS TO MAKE THE SOFTWARE COMPLY WITH ALL COUNTRIES' LAWS BY THE
// PROJECT. EVEN IF YOU WILL BE SUED BY A PRIVATE ENTITY OR BE DAMAGED BY A
// PUBLIC SERVANT IN YOUR COUNTRY, THE DEVELOPERS OF THIS SOFTWARE WILL NEVER BE
// LIABLE TO RECOVER OR COMPENSATE SUCH DAMAGES, CRIMINAL OR CIVIL
// RESPONSIBILITIES. NOTE THAT THIS LINE IS NOT LICENSE RESTRICTION BUT JUST A
// STATEMENT FOR WARNING AND DISCLAIMER.
// 
// READ AND UNDERSTAND THE 'WARNING.TXT' FILE BEFORE USING THIS SOFTWARE.
// SOME SOFTWARE PROGRAMS FROM THIRD PARTIES ARE INCLUDED ON THIS SOFTWARE WITH
// LICENSE CONDITIONS WHICH ARE DESCRIBED ON THE 'THIRD_PARTY.TXT' FILE.
// 
// ---------------------
// 
// If you find a bug or a security vulnerability please kindly inform us
// about the problem immediately so that we can fix the security problem
// to protect a lot of users around the world as soon as possible.
// 
// Our e-mail address for security reports is:
// daiyuu.securityreport [at] dnobori.jp
// 
// Thank you for your cooperation.


// WaterMark.c
// SoftEther protocol digital watermark data

#include <GlobalConst.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

#include <Mayaqua/Mayaqua.h>
#include <Cedar/Cedar.h>

// Digital watermark image data (JPEG)
static BYTE WaterMark[] =
{
	0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0xC8, 0x00, 0x33, 0x00, 0xF2, 0x00, 0x00, 0x36, 0x37, 0x34, 
	0x79, 0x68, 0x54, 0x80, 0x80, 0x80, 0xAF, 0x7F, 0x5B, 0xB3, 0xA8, 0x9D, 0xD5, 0xD5, 0xD4, 0xFF, 
	0xFF, 0xFF, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x33, 0x00, 0x00, 0x03, 
	0xFE, 0x08, 0x1A, 0xDC, 0x34, 0x0A, 0x04, 0x41, 0x6B, 0x65, 0x31, 0x4F, 0x11, 0x80, 0xF9, 0x60, 
	0x28, 0x8E, 0x64, 0x69, 0x9E, 0x68, 0xAA, 0xAE, 0x6C, 0xEB, 0x9A, 0x4B, 0xE3, 0x0C, 0x0C, 0x25, 
	0x6F, 0x56, 0xA7, 0xE9, 0xD2, 0xEB, 0xFF, 0xC0, 0xA0, 0x70, 0xC8, 0x8A, 0xDC, 0x2C, 0x9C, 0xC6, 
	0x05, 0xC7, 0x31, 0x66, 0x24, 0x04, 0xA2, 0x74, 0x4A, 0xAD, 0x4E, 0x05, 0xB1, 0x0D, 0x61, 0xCB, 
	0x25, 0xD4, 0xB8, 0x49, 0x1B, 0xE6, 0x19, 0xB1, 0x9A, 0xCF, 0xE8, 0xF4, 0x07, 0x2B, 0x11, 0x74, 
	0x09, 0x85, 0x78, 0xFC, 0x0D, 0x6E, 0x90, 0x9F, 0xEA, 0x02, 0x81, 0x12, 0x35, 0xEF, 0x29, 0x6A, 
	0x81, 0x2C, 0x04, 0x0A, 0x6E, 0x5C, 0x72, 0x88, 0x7A, 0x7A, 0x6F, 0x4D, 0x77, 0x19, 0x25, 0x71, 
	0x16, 0x71, 0x2F, 0x05, 0x92, 0x06, 0x95, 0x80, 0x22, 0x48, 0x16, 0x7D, 0x98, 0x02, 0x9A, 0x7C, 
	0x82, 0x06, 0x16, 0x23, 0x7F, 0x02, 0x05, 0x6B, 0x48, 0x70, 0x23, 0x15, 0x7D, 0x1F, 0x98, 0xA8, 
	0x21, 0x7F, 0x87, 0x89, 0xB5, 0x8B, 0x7C, 0x7B, 0x3C, 0x8E, 0x23, 0x9E, 0x9B, 0xAE, 0x2B, 0xAD, 
	0x20, 0xA6, 0xAC, 0x9B, 0x14, 0xB1, 0xC3, 0x21, 0x15, 0xB1, 0x81, 0x9E, 0x22, 0x9E, 0xAE, 0xC5, 
	0x99, 0x20, 0x96, 0xAF, 0xC6, 0xA0, 0x70, 0xB6, 0xB6, 0x5B, 0x03, 0x1C, 0x16, 0x8E, 0x65, 0x21, 
	0xBD, 0x9B, 0xCB, 0x2A, 0x9E, 0xCB, 0xC1, 0xE1, 0xD1, 0xA7, 0xA9, 0x6E, 0xE9, 0xD6, 0x82, 0xCD, 
	0xC9, 0xCA, 0xD5, 0xD1, 0xAE, 0xBD, 0xCB, 0x7F, 0xAC, 0xB4, 0xD9, 0x73, 0x34, 0x37, 0x76, 0xDF, 
	0x3C, 0xC8, 0x9A, 0x07, 0x42, 0x4E, 0x38, 0x4C, 0xAB, 0x0A, 0xFA, 0x12, 0x17, 0xEA, 0x52, 0x05, 
	0x12, 0x0C, 0xDB, 0x35, 0xD3, 0xF3, 0xCE, 0xD9, 0x2C, 0x72, 0x13, 0xB7, 0x40, 0x22, 0xE8, 0xFE, 
	0xB0, 0x61, 0xC7, 0x4F, 0xEC, 0x40, 0x7E, 0x94, 0xF6, 0x50, 0x13, 0x36, 0x83, 0xA8, 0x6A, 0x79, 
	0xF9, 0x77, 0xE3, 0x1B, 0x28, 0x69, 0x1B, 0x55, 0x09, 0x1B, 0x67, 0x8A, 0x1A, 0xA9, 0x52, 0xC5, 
	0x50, 0x71, 0x42, 0x82, 0x31, 0xDA, 0xB4, 0x56, 0x15, 0x9D, 0x71, 0xBC, 0x19, 0xF2, 0x27, 0x49, 
	0x3E, 0xEF, 0x3C, 0x4E, 0xDB, 0x92, 0xED, 0x52, 0xBF, 0x01, 0xFE, 0x02, 0x44, 0x95, 0xB1, 0x6B, 
	0xA0, 0x32, 0x72, 0x0A, 0x25, 0x72, 0x1C, 0xE5, 0x11, 0x99, 0x3C, 0x5F, 0x33, 0x61, 0x72, 0x75, 
	0x93, 0x92, 0x28, 0x42, 0xA3, 0x7D, 0x72, 0x9A, 0x20, 0x68, 0x8A, 0x1C, 0x3A, 0x73, 0x3F, 0xE1, 
	0x84, 0x82, 0x55, 0xEA, 0xE4, 0xA5, 0xBB, 0x89, 0xDE, 0x4C, 0x60, 0x30, 0x75, 0x0C, 0x9E, 0x97, 
	0xD4, 0x8C, 0xC6, 0x32, 0x3B, 0xB4, 0x64, 0xD6, 0x71, 0x46, 0x45, 0x7E, 0x3C, 0x67, 0xB8, 0x30, 
	0x20, 0xB8, 0x29, 0x82, 0x3D, 0x73, 0xE7, 0x93, 0x1E, 0xAA, 0x3F, 0x91, 0xD6, 0x89, 0x60, 0x9A, 
	0xC8, 0x69, 0x36, 0xA8, 0x1B, 0xA4, 0xFE, 0x23, 0x03, 0x51, 0xED, 0xC7, 0xC4, 0x87, 0x19, 0xB7, 
	0xA3, 0xCC, 0x13, 0x2D, 0x65, 0xD5, 0xB1, 0x22, 0x4A, 0xDE, 0xBA, 0xF6, 0xA1, 0x57, 0x7A, 0x0B, 
	0xB3, 0x96, 0x3D, 0x95, 0xAF, 0x2E, 0x4A, 0xBC, 0x2A, 0xB9, 0x25, 0x61, 0x09, 0x10, 0x1C, 0x24, 
	0x53, 0x7D, 0xBC, 0xA2, 0x33, 0xE0, 0x15, 0x72, 0x58, 0xC5, 0xAF, 0xAD, 0x8A, 0x84, 0x5C, 0x13, 
	0xF1, 0xED, 0x13, 0xE6, 0x68, 0x57, 0x3F, 0x85, 0xB5, 0xF7, 0x58, 0xC3, 0xB2, 0x3A, 0xA7, 0x54, 
	0xB9, 0x87, 0x86, 0x98, 0xBD, 0xA3, 0x8D, 0xD7, 0xCE, 0x44, 0xD4, 0xF1, 0x74, 0xDA, 0x44, 0x85, 
	0x06, 0x25, 0x7C, 0x54, 0xEC, 0x57, 0xE8, 0x26, 0x18, 0xFE, 0x2A, 0xBA, 0xFE, 0xB9, 0xFE, 0xE6, 
	0xCD, 0x88, 0x00, 0x57, 0x0B, 0x54, 0xFE, 0x20, 0x31, 0x1A, 0x0F, 0x01, 0x14, 0x94, 0xD0, 0x61, 
	0x69, 0x95, 0x14, 0x0F, 0x3B, 0xAE, 0x5C, 0x37, 0x16, 0x56, 0xCF, 0xBD, 0x14, 0xA1, 0x61, 0x12, 
	0x0E, 0xA6, 0x14, 0x76, 0x88, 0xBD, 0x44, 0xA1, 0x3C, 0xF6, 0x04, 0x76, 0x90, 0x78, 0xE4, 0x81, 
	0x26, 0x80, 0x70, 0x0F, 0x10, 0xA7, 0xC4, 0x61, 0x95, 0x2D, 0xC6, 0x5C, 0x45, 0xCE, 0x89, 0x28, 
	0x1B, 0x34, 0x1C, 0xC5, 0xE8, 0xD1, 0x64, 0xAF, 0xAC, 0xE2, 0x1C, 0x0A, 0xE2, 0xEC, 0xE7, 0x62, 
	0x4C, 0xE4, 0xB4, 0x05, 0x51, 0x80, 0x93, 0x04, 0xE7, 0x8F, 0x70, 0x01, 0x6C, 0xA1, 0x62, 0x0D, 
	0xFE, 0x75, 0xF8, 0xC1, 0x76, 0x3D, 0x55, 0x54, 0x5D, 0x27, 0xD1, 0xE0, 0x23, 0x13, 0x64, 0x3B, 
	0x6E, 0x67, 0xCD, 0x8E, 0x28, 0x20, 0x51, 0x5A, 0x50, 0xF2, 0x45, 0x89, 0xDF, 0x2B, 0xB5, 0x78, 
	0x26, 0x07, 0x17, 0x04, 0x8A, 0xE6, 0x46, 0x5F, 0x2C, 0x1D, 0x84, 0xDC, 0x24, 0xBC, 0x60, 0xD6, 
	0x1D, 0x78, 0x1F, 0x25, 0xA4, 0xE5, 0x7F, 0x75, 0x5E, 0x66, 0x18, 0x97, 0x73, 0xF0, 0x01, 0xA7, 
	0x84, 0x27, 0x88, 0x58, 0xA1, 0x09, 0xDE, 0xC5, 0x05, 0x09, 0x3F, 0x88, 0xA0, 0x79, 0x24, 0x54, 
	0x0F, 0x80, 0xC6, 0x66, 0x07, 0xA2, 0x44, 0x2A, 0xE9, 0xA4, 0x23, 0x22, 0x3A, 0xC7, 0x36, 0x0D, 
	0x0C, 0xD0, 0x28, 0x81, 0xA0, 0xB5, 0x44, 0xE9, 0xA7, 0xA0, 0xA2, 0x71, 0x52, 0x36, 0x70, 0xE8, 
	0x25, 0x55, 0x9A, 0x9C, 0x46, 0xE5, 0x8F, 0x40, 0xA1, 0xB6, 0xEA, 0x6A, 0x10, 0xA3, 0x9E, 0x49, 
	0x9E, 0x92, 0xA7, 0xA6, 0xCA, 0xA9, 0xA7, 0xAF, 0xE6, 0xAA, 0xEB, 0x0A, 0xA5, 0x4E, 0x99, 0x57, 
	0x1D, 0xB5, 0x6E, 0x8A, 0xEA, 0x18, 0xBB, 0x16, 0x6B, 0xAC, 0x3E, 0x71, 0x20, 0xFE, 0x48, 0x16, 
	0x36, 0x5D, 0x24, 0xC1, 0xA9, 0xB0, 0x69, 0xEA, 0x70, 0xEC, 0xB4, 0xC6, 0x26, 0xD9, 0x45, 0x0D, 
	0x1C, 0x8C, 0x0A, 0x2C, 0x81, 0xD0, 0x76, 0x2A, 0x2D, 0xB5, 0xE0, 0xBE, 0x9A, 0xA4, 0x21, 0xB9, 
	0x0C, 0x47, 0x6E, 0x9F, 0xB5, 0xDA, 0xEA, 0x28, 0xB1, 0x25, 0x88, 0x54, 0xD2, 0x98, 0x8D, 0xD5, 
	0xA7, 0x09, 0x31, 0xF6, 0x25, 0x33, 0x4A, 0x48, 0x9F, 0x80, 0x34, 0xA6, 0x0A, 0x74, 0x56, 0xA1, 
	0xAF, 0x0F, 0x6D, 0x10, 0x27, 0x41, 0x1B, 0x4C, 0x79, 0xA1, 0x2E, 0x5F, 0x9D, 0xAA, 0x67, 0xEF, 
	0x1A, 0xD3, 0x30, 0xBC, 0xF0, 0xBD, 0xEE, 0xDE, 0xEB, 0x30, 0x57, 0xF3, 0x36, 0x4C, 0xC2, 0xBF, 
	0x12, 0x5B, 0xBC, 0x6F, 0x97, 0x16, 0x9B, 0xB1, 0xB1, 0x0A, 0x59, 0xC8, 0x30, 0x9C, 0xC8, 0xDB, 
	0x68, 0x9A, 0xEA, 0x02, 0x09, 0x2B, 0x70, 0x71, 0xC7, 0x15, 0xB3, 0x92, 0x71, 0xBE, 0x1A, 0x67, 
	0x3C, 0xF1, 0x57, 0xF8, 0xC2, 0x6C, 0x14, 0xC4, 0xEE, 0xB2, 0x27, 0x33, 0xBC, 0x3A, 0xC3, 0x2C, 
	0x2F, 0xC4, 0xEC, 0x8C, 0x25, 0xF1, 0xBB, 0xFD, 0x7E, 0x10, 0xB2, 0x12, 0xC4, 0x91, 0x5B, 0x32, 
	0x54, 0x46, 0x14, 0xB7, 0xF2, 0xCC, 0x0F, 0xCF, 0x1B, 0x71, 0xC4, 0x40, 0x83, 0xF2, 0x30, 0xC6, 
	0xFA, 0x92, 0x92, 0x35, 0xC3, 0x53, 0x43, 0x87, 0x5F, 0xD7, 0xA9, 0x70, 0xDD, 0xB0, 0xCE, 0x62, 
	0x57, 0x6D, 0xF6, 0x98, 0x4D, 0x8B, 0x3C, 0x32, 0xD2, 0xE4, 0xA6, 0x8A, 0xB0, 0x5F, 0x4F, 0xCB, 
	0x1C, 0x75, 0xCC, 0x65, 0x57, 0xBD, 0x2F, 0xD9, 0x43, 0x3B, 0xEC, 0xF5, 0xC4, 0xF9, 0x6A, 0xED, 
	0x72, 0xCB, 0x36, 0xBF, 0x2C, 0xB8, 0x62, 0x7E, 0x9F, 0x2D, 0xF8, 0x08, 0x69, 0x87, 0xB1, 0xF6, 
	0x3F, 0x6B, 0xAA, 0x0B, 0x9A, 0xC2, 0x7C, 0xB7, 0xFB, 0xF7, 0xE0, 0x63, 0xFE, 0xC7, 0x27, 0x35, 
	0xDD, 0x18, 0xD3, 0x6D, 0x36, 0xD4, 0x72, 0x53, 0x1E, 0xF9, 0xD4, 0x1D, 0xDB, 0x1C, 0xF8, 0xE8, 
	0x24, 0x2C, 0xB0, 0x44, 0x0E, 0x2C, 0x99, 0xDE, 0x6D, 0x9A, 0x90, 0xEF, 0x1C, 0x7A, 0xCB, 0x9E, 
	0xBB, 0x1E, 0x35, 0xE9, 0x79, 0xCB, 0x9D, 0x39, 0xE9, 0xF0, 0x8E, 0xAD, 0x7B, 0xD8, 0x86, 0x53, 
	0x0D, 0xC8, 0xBF, 0xA0, 0x73, 0x6E, 0x80, 0x12, 0x39, 0x9C, 0x27, 0x72, 0x07, 0x3A, 0xB4, 0xED, 
	0x76, 0xEB, 0x5E, 0xC3, 0x44, 0xF8, 0x4D, 0xF1, 0xEE, 0x0D, 0xD8, 0xCD, 0x7A, 0xF7, 0xFD, 0xD0, 
	0xEF, 0x1A, 0xE3, 0xFD, 0x12, 0xF5, 0x60, 0x07, 0xBD, 0xB3, 0xCF, 0xA2, 0xE3, 0x9D, 0xB9, 0x01, 
	0xA6, 0x9F, 0x6E, 0x7C, 0x0D, 0x18, 0xE8, 0x60, 0x2D, 0xB4, 0xEC, 0x4E, 0x1E, 0x77, 0xB8, 0x81, 
	0x7C, 0x9C, 0x06, 0xF1, 0x17, 0xD8, 0x60, 0x6E, 0x68, 0x03, 0x2F, 0xA0, 0x68, 0x54, 0x2A, 0x4B, 
	0xFE, 0x3E, 0xFC, 0x6A, 0x90, 0x1F, 0x1A, 0xCA, 0x57, 0xBF, 0xD0, 0x98, 0x2B, 0x09, 0xF9, 0x03, 
	0x80, 0x21, 0x6E, 0xD5, 0x3A, 0x00, 0x3A, 0x30, 0x0D, 0x04, 0xB4, 0x1F, 0x0E, 0x8E, 0xE0, 0x17, 
	0x23, 0x48, 0xF0, 0x11, 0x67, 0x20, 0xDC, 0xF7, 0xDE, 0xF5, 0x3F, 0xF9, 0x79, 0x29, 0x52, 0x02, 
	0x7C, 0x60, 0x1A, 0x70, 0x37, 0xBB, 0xB5, 0xC0, 0xEE, 0x7D, 0x21, 0x94, 0x42, 0x0A, 0x45, 0xE8, 
	0xB1, 0xD8, 0xB9, 0x6E, 0x6B, 0xE0, 0x13, 0x9A, 0x0C, 0x59, 0x96, 0xB5, 0x9C, 0xD9, 0x50, 0x6C, 
	0xBE, 0x3B, 0x4A, 0xE7, 0x58, 0x28, 0x0A, 0x12, 0x26, 0x06, 0x78, 0x61, 0xEB, 0x59, 0xE4, 0x7E, 
	0xF8, 0xB9, 0xDD, 0xE1, 0xAC, 0x88, 0x65, 0xAB, 0x17, 0x0F, 0x03, 0x18, 0x33, 0x0D, 0xC6, 0xCE, 
	0x87, 0x14, 0xAB, 0x98, 0x0D, 0xD9, 0x33, 0xC5, 0xC0, 0xD9, 0xAD, 0x55, 0x70, 0x3B, 0x5C, 0xE2, 
	0x08, 0xA1, 0x27, 0xBB, 0xBC, 0x05, 0x6F, 0x73, 0xB6, 0xD3, 0x9C, 0x14, 0x61, 0x27, 0x3A, 0xC0, 
	0x69, 0x11, 0x84, 0x97, 0x73, 0xA2, 0x17, 0x83, 0xB8, 0x3B, 0xAA, 0x0D, 0xF1, 0x8B, 0x50, 0x1C, 
	0xE2, 0x15, 0xCF, 0xD8, 0xC3, 0x34, 0x96, 0x10, 0x86, 0x83, 0xAB, 0x21, 0x19, 0xBD, 0x37, 0x43, 
	0x0E, 0xCE, 0x4E, 0x87, 0xE3, 0xA3, 0x63, 0xB8, 0x56, 0x28, 0xC8, 0x42, 0x82, 0xB0, 0x68, 0x86, 
	0x4C, 0xA4, 0x22, 0x17, 0xC9, 0xC8, 0x46, 0x3A, 0xF2, 0x91, 0x90, 0x8C, 0xA4, 0x24, 0x75, 0x95, 
	0x00, 0x00, 0x3B, 
};

UCHAR* GetWaterMark()
{
	UINT alternative_watermark_size = 0;
	UCHAR* alternative_watermark = Vars_ActivePatch_GetData2("AlternativeWaterMark", &alternative_watermark_size);
	if (alternative_watermark != NULL)
	{
		return alternative_watermark;
	}

	return WaterMark;
}

UINT SizeOfWaterMark()
{
	UINT alternative_watermark_size = 0;
	UCHAR* alternative_watermark = Vars_ActivePatch_GetData2("AlternativeWaterMark", &alternative_watermark_size);
	if (alternative_watermark != NULL)
	{
		return alternative_watermark_size;
	}

	return sizeof(WaterMark);
}



