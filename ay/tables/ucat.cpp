/*
 * uclass.cpp
 *
 *  Created on: Jan 26, 2015
 *      Author: polter
 */

uint32_t table_unicode_punct[] = {
	 0x00021,0x00022,0x00023,0x00025,0x00026,0x00027,0x00028,0x00029,0x0002a,
	 0x0002c,0x0002d,0x0002e,0x0002f,0x0003a,0x0003b,0x0003f,0x00040,0x0005b,
	 0x0005c,0x0005d,0x0005f,0x0007b,0x0007d,0x000a1,0x000a7,0x000ab,0x000b6,
	 0x000b7,0x000bb,0x000bf,0x0037e,0x00387,0x0055a,0x0055b,0x0055c,0x0055d,
	 0x0055e,0x0055f,0x00589,0x0058a,0x005be,0x005c0,0x005c3,0x005c6,0x005f3,
	 0x005f4,0x00609,0x0060a,0x0060c,0x0060d,0x0061b,0x0061e,0x0061f,0x0066a,
	 0x0066b,0x0066c,0x0066d,0x006d4,0x00700,0x00701,0x00702,0x00703,0x00704,
	 0x00705,0x00706,0x00707,0x00708,0x00709,0x0070a,0x0070b,0x0070c,0x0070d,
	 0x007f7,0x007f8,0x007f9,0x00830,0x00831,0x00832,0x00833,0x00834,0x00835,
	 0x00836,0x00837,0x00838,0x00839,0x0083a,0x0083b,0x0083c,0x0083d,0x0083e,
	 0x0085e,0x00964,0x00965,0x00970,0x00af0,0x00df4,0x00e4f,0x00e5a,0x00e5b,
	 0x00f04,0x00f05,0x00f06,0x00f07,0x00f08,0x00f09,0x00f0a,0x00f0b,0x00f0c,
	 0x00f0d,0x00f0e,0x00f0f,0x00f10,0x00f11,0x00f12,0x00f14,0x00f3a,0x00f3b,
	 0x00f3c,0x00f3d,0x00f85,0x00fd0,0x00fd1,0x00fd2,0x00fd3,0x00fd4,0x00fd9,
	 0x00fda,0x0104a,0x0104b,0x0104c,0x0104d,0x0104e,0x0104f,0x010fb,0x01360,
	 0x01361,0x01362,0x01363,0x01364,0x01365,0x01366,0x01367,0x01368,0x01400,
	 0x0166d,0x0166e,0x0169b,0x0169c,0x016eb,0x016ec,0x016ed,0x01735,0x01736,
	 0x017d4,0x017d5,0x017d6,0x017d8,0x017d9,0x017da,0x01800,0x01801,0x01802,
	 0x01803,0x01804,0x01805,0x01806,0x01807,0x01808,0x01809,0x0180a,0x01944,
	 0x01945,0x01a1e,0x01a1f,0x01aa0,0x01aa1,0x01aa2,0x01aa3,0x01aa4,0x01aa5,
	 0x01aa6,0x01aa8,0x01aa9,0x01aaa,0x01aab,0x01aac,0x01aad,0x01b5a,0x01b5b,
	 0x01b5c,0x01b5d,0x01b5e,0x01b5f,0x01b60,0x01bfc,0x01bfd,0x01bfe,0x01bff,
	 0x01c3b,0x01c3c,0x01c3d,0x01c3e,0x01c3f,0x01c7e,0x01c7f,0x01cc0,0x01cc1,
	 0x01cc2,0x01cc3,0x01cc4,0x01cc5,0x01cc6,0x01cc7,0x01cd3,0x02010,0x02011,
	 0x02012,0x02013,0x02014,0x02015,0x02016,0x02017,0x02018,0x02019,0x0201a,
	 0x0201b,0x0201c,0x0201d,0x0201e,0x0201f,0x02020,0x02021,0x02022,0x02023,
	 0x02024,0x02025,0x02026,0x02027,0x02030,0x02031,0x02032,0x02033,0x02034,
	 0x02035,0x02036,0x02037,0x02038,0x02039,0x0203a,0x0203b,0x0203c,0x0203d,
	 0x0203e,0x0203f,0x02040,0x02041,0x02042,0x02043,0x02045,0x02046,0x02047,
	 0x02048,0x02049,0x0204a,0x0204b,0x0204c,0x0204d,0x0204e,0x0204f,0x02050,
	 0x02051,0x02053,0x02054,0x02055,0x02056,0x02057,0x02058,0x02059,0x0205a,
	 0x0205b,0x0205c,0x0205d,0x0205e,0x0207d,0x0207e,0x0208d,0x0208e,0x02308,
	 0x02309,0x0230a,0x0230b,0x02329,0x0232a,0x02768,0x02769,0x0276a,0x0276b,
	 0x0276c,0x0276d,0x0276e,0x0276f,0x02770,0x02771,0x02772,0x02773,0x02774,
	 0x02775,0x027c5,0x027c6,0x027e6,0x027e7,0x027e8,0x027e9,0x027ea,0x027eb,
	 0x027ec,0x027ed,0x027ee,0x027ef,0x02983,0x02984,0x02985,0x02986,0x02987,
	 0x02988,0x02989,0x0298a,0x0298b,0x0298c,0x0298d,0x0298e,0x0298f,0x02990,
	 0x02991,0x02992,0x02993,0x02994,0x02995,0x02996,0x02997,0x02998,0x029d8,
	 0x029d9,0x029da,0x029db,0x029fc,0x029fd,0x02cf9,0x02cfa,0x02cfb,0x02cfc,
	 0x02cfe,0x02cff,0x02d70,0x02e00,0x02e01,0x02e02,0x02e03,0x02e04,0x02e05,
	 0x02e06,0x02e07,0x02e08,0x02e09,0x02e0a,0x02e0b,0x02e0c,0x02e0d,0x02e0e,
	 0x02e0f,0x02e10,0x02e11,0x02e12,0x02e13,0x02e14,0x02e15,0x02e16,0x02e17,
	 0x02e18,0x02e19,0x02e1a,0x02e1b,0x02e1c,0x02e1d,0x02e1e,0x02e1f,0x02e20,
	 0x02e21,0x02e22,0x02e23,0x02e24,0x02e25,0x02e26,0x02e27,0x02e28,0x02e29,
	 0x02e2a,0x02e2b,0x02e2c,0x02e2d,0x02e2e,0x02e30,0x02e31,0x02e32,0x02e33,
	 0x02e34,0x02e35,0x02e36,0x02e37,0x02e38,0x02e39,0x02e3a,0x02e3b,0x02e3c,
	 0x02e3d,0x02e3e,0x02e3f,0x02e40,0x02e41,0x02e42,0x03001,0x03002,0x03003,
	 0x03008,0x03009,0x0300a,0x0300b,0x0300c,0x0300d,0x0300e,0x0300f,0x03010,
	 0x03011,0x03014,0x03015,0x03016,0x03017,0x03018,0x03019,0x0301a,0x0301b,
	 0x0301c,0x0301d,0x0301e,0x0301f,0x03030,0x0303d,0x030a0,0x030fb,0x0a4fe,
	 0x0a4ff,0x0a60d,0x0a60e,0x0a60f,0x0a673,0x0a67e,0x0a6f2,0x0a6f3,0x0a6f4,
	 0x0a6f5,0x0a6f6,0x0a6f7,0x0a874,0x0a875,0x0a876,0x0a877,0x0a8ce,0x0a8cf,
	 0x0a8f8,0x0a8f9,0x0a8fa,0x0a92e,0x0a92f,0x0a95f,0x0a9c1,0x0a9c2,0x0a9c3,
	 0x0a9c4,0x0a9c5,0x0a9c6,0x0a9c7,0x0a9c8,0x0a9c9,0x0a9ca,0x0a9cb,0x0a9cc,
	 0x0a9cd,0x0a9de,0x0a9df,0x0aa5c,0x0aa5d,0x0aa5e,0x0aa5f,0x0aade,0x0aadf,
	 0x0aaf0,0x0aaf1,0x0abeb,0x0fd3e,0x0fd3f,0x0fe10,0x0fe11,0x0fe12,0x0fe13,
	 0x0fe14,0x0fe15,0x0fe16,0x0fe17,0x0fe18,0x0fe19,0x0fe30,0x0fe31,0x0fe32,
	 0x0fe33,0x0fe34,0x0fe35,0x0fe36,0x0fe37,0x0fe38,0x0fe39,0x0fe3a,0x0fe3b,
	 0x0fe3c,0x0fe3d,0x0fe3e,0x0fe3f,0x0fe40,0x0fe41,0x0fe42,0x0fe43,0x0fe44,
	 0x0fe45,0x0fe46,0x0fe47,0x0fe48,0x0fe49,0x0fe4a,0x0fe4b,0x0fe4c,0x0fe4d,
	 0x0fe4e,0x0fe4f,0x0fe50,0x0fe51,0x0fe52,0x0fe54,0x0fe55,0x0fe56,0x0fe57,
	 0x0fe58,0x0fe59,0x0fe5a,0x0fe5b,0x0fe5c,0x0fe5d,0x0fe5e,0x0fe5f,0x0fe60,
	 0x0fe61,0x0fe63,0x0fe68,0x0fe6a,0x0fe6b,0x0ff01,0x0ff02,0x0ff03,0x0ff05,
	 0x0ff06,0x0ff07,0x0ff08,0x0ff09,0x0ff0a,0x0ff0c,0x0ff0d,0x0ff0e,0x0ff0f,
	 0x0ff1a,0x0ff1b,0x0ff1f,0x0ff20,0x0ff3b,0x0ff3c,0x0ff3d,0x0ff3f,0x0ff5b,
	 0x0ff5d,0x0ff5f,0x0ff60,0x0ff61,0x0ff62,0x0ff63,0x0ff64,0x0ff65,0x10100,
	 0x10101,0x10102,0x1039f,0x103d0,0x1056f,0x10857,0x1091f,0x1093f,0x10a50,
	 0x10a51,0x10a52,0x10a53,0x10a54,0x10a55,0x10a56,0x10a57,0x10a58,0x10a7f,
	 0x10af0,0x10af1,0x10af2,0x10af3,0x10af4,0x10af5,0x10af6,0x10b39,0x10b3a,
	 0x10b3b,0x10b3c,0x10b3d,0x10b3e,0x10b3f,0x10b99,0x10b9a,0x10b9b,0x10b9c,
	 0x11047,0x11048,0x11049,0x1104a,0x1104b,0x1104c,0x1104d,0x110bb,0x110bc,
	 0x110be,0x110bf,0x110c0,0x110c1,0x11140,0x11141,0x11142,0x11143,0x11174,
	 0x11175,0x111c5,0x111c6,0x111c7,0x111c8,0x111cd,0x11238,0x11239,0x1123a,
	 0x1123b,0x1123c,0x1123d,0x114c6,0x115c1,0x115c2,0x115c3,0x115c4,0x115c5,
	 0x115c6,0x115c7,0x115c8,0x115c9,0x11641,0x11642,0x11643,0x12470,0x12471,
	 0x12472,0x12473,0x12474,0x16a6e,0x16a6f,0x16af5,0x16b37,0x16b38,0x16b39,
	 0x16b3a,0x16b3b,0x16b44,0x1bc9f
};

uint32_t table_unicode_space[] = {
	0x0020,0x00a0,0x1680,0x2000,0x2001,0x2002,0x2003,0x2004,0x2005,0x2006,
	0x2007,0x2008,0x2009,0x200a,0x2028,0x2029,0x202f,0x205f,0x3000
};

uint32_t table_unicode_numbers_dec[] = {
	0x00030,0x00031,0x00032,0x00033,0x00034,0x00035,0x00036,0x00037,0x00038,
	0x00039,0x00660,0x00661,0x00662,0x00663,0x00664,0x00665,0x00666,0x00667,
	0x00668,0x00669,0x006f0,0x006f1,0x006f2,0x006f3,0x006f4,0x006f5,0x006f6,
	0x006f7,0x006f8,0x006f9,0x007c0,0x007c1,0x007c2,0x007c3,0x007c4,0x007c5,
	0x007c6,0x007c7,0x007c8,0x007c9,0x00966,0x00967,0x00968,0x00969,0x0096a,
	0x0096b,0x0096c,0x0096d,0x0096e,0x0096f,0x009e6,0x009e7,0x009e8,0x009e9,
	0x009ea,0x009eb,0x009ec,0x009ed,0x009ee,0x009ef,0x00a66,0x00a67,0x00a68,
	0x00a69,0x00a6a,0x00a6b,0x00a6c,0x00a6d,0x00a6e,0x00a6f,0x00ae6,0x00ae7,
	0x00ae8,0x00ae9,0x00aea,0x00aeb,0x00aec,0x00aed,0x00aee,0x00aef,0x00b66,
	0x00b67,0x00b68,0x00b69,0x00b6a,0x00b6b,0x00b6c,0x00b6d,0x00b6e,0x00b6f,
	0x00be6,0x00be7,0x00be8,0x00be9,0x00bea,0x00beb,0x00bec,0x00bed,0x00bee,
	0x00bef,0x00c66,0x00c67,0x00c68,0x00c69,0x00c6a,0x00c6b,0x00c6c,0x00c6d,
	0x00c6e,0x00c6f,0x00ce6,0x00ce7,0x00ce8,0x00ce9,0x00cea,0x00ceb,0x00cec,
	0x00ced,0x00cee,0x00cef,0x00d66,0x00d67,0x00d68,0x00d69,0x00d6a,0x00d6b,
	0x00d6c,0x00d6d,0x00d6e,0x00d6f,0x00de6,0x00de7,0x00de8,0x00de9,0x00dea,
	0x00deb,0x00dec,0x00ded,0x00dee,0x00def,0x00e50,0x00e51,0x00e52,0x00e53,
	0x00e54,0x00e55,0x00e56,0x00e57,0x00e58,0x00e59,0x00ed0,0x00ed1,0x00ed2,
	0x00ed3,0x00ed4,0x00ed5,0x00ed6,0x00ed7,0x00ed8,0x00ed9,0x00f20,0x00f21,
	0x00f22,0x00f23,0x00f24,0x00f25,0x00f26,0x00f27,0x00f28,0x00f29,0x01040,
	0x01041,0x01042,0x01043,0x01044,0x01045,0x01046,0x01047,0x01048,0x01049,
	0x01090,0x01091,0x01092,0x01093,0x01094,0x01095,0x01096,0x01097,0x01098,
	0x01099,0x017e0,0x017e1,0x017e2,0x017e3,0x017e4,0x017e5,0x017e6,0x017e7,
	0x017e8,0x017e9,0x01810,0x01811,0x01812,0x01813,0x01814,0x01815,0x01816,
	0x01817,0x01818,0x01819,0x01946,0x01947,0x01948,0x01949,0x0194a,0x0194b,
	0x0194c,0x0194d,0x0194e,0x0194f,0x019d0,0x019d1,0x019d2,0x019d3,0x019d4,
	0x019d5,0x019d6,0x019d7,0x019d8,0x019d9,0x01a80,0x01a81,0x01a82,0x01a83,
	0x01a84,0x01a85,0x01a86,0x01a87,0x01a88,0x01a89,0x01a90,0x01a91,0x01a92,
	0x01a93,0x01a94,0x01a95,0x01a96,0x01a97,0x01a98,0x01a99,0x01b50,0x01b51,
	0x01b52,0x01b53,0x01b54,0x01b55,0x01b56,0x01b57,0x01b58,0x01b59,0x01bb0,
	0x01bb1,0x01bb2,0x01bb3,0x01bb4,0x01bb5,0x01bb6,0x01bb7,0x01bb8,0x01bb9,
	0x01c40,0x01c41,0x01c42,0x01c43,0x01c44,0x01c45,0x01c46,0x01c47,0x01c48,
	0x01c49,0x01c50,0x01c51,0x01c52,0x01c53,0x01c54,0x01c55,0x01c56,0x01c57,
	0x01c58,0x01c59,0x0a620,0x0a621,0x0a622,0x0a623,0x0a624,0x0a625,0x0a626,
	0x0a627,0x0a628,0x0a629,0x0a8d0,0x0a8d1,0x0a8d2,0x0a8d3,0x0a8d4,0x0a8d5,
	0x0a8d6,0x0a8d7,0x0a8d8,0x0a8d9,0x0a900,0x0a901,0x0a902,0x0a903,0x0a904,
	0x0a905,0x0a906,0x0a907,0x0a908,0x0a909,0x0a9d0,0x0a9d1,0x0a9d2,0x0a9d3,
	0x0a9d4,0x0a9d5,0x0a9d6,0x0a9d7,0x0a9d8,0x0a9d9,0x0a9f0,0x0a9f1,0x0a9f2,
	0x0a9f3,0x0a9f4,0x0a9f5,0x0a9f6,0x0a9f7,0x0a9f8,0x0a9f9,0x0aa50,0x0aa51,
	0x0aa52,0x0aa53,0x0aa54,0x0aa55,0x0aa56,0x0aa57,0x0aa58,0x0aa59,0x0abf0,
	0x0abf1,0x0abf2,0x0abf3,0x0abf4,0x0abf5,0x0abf6,0x0abf7,0x0abf8,0x0abf9,
	0x0ff10,0x0ff11,0x0ff12,0x0ff13,0x0ff14,0x0ff15,0x0ff16,0x0ff17,0x0ff18,
	0x0ff19,0x104a0,0x104a1,0x104a2,0x104a3,0x104a4,0x104a5,0x104a6,0x104a7,
	0x104a8,0x104a9,0x11066,0x11067,0x11068,0x11069,0x1106a,0x1106b,0x1106c,
	0x1106d,0x1106e,0x1106f,0x110f0,0x110f1,0x110f2,0x110f3,0x110f4,0x110f5,
	0x110f6,0x110f7,0x110f8,0x110f9,0x11136,0x11137,0x11138,0x11139,0x1113a,
	0x1113b,0x1113c,0x1113d,0x1113e,0x1113f,0x111d0,0x111d1,0x111d2,0x111d3,
	0x111d4,0x111d5,0x111d6,0x111d7,0x111d8,0x111d9,0x112f0,0x112f1,0x112f2,
	0x112f3,0x112f4,0x112f5,0x112f6,0x112f7,0x112f8,0x112f9,0x114d0,0x114d1,
	0x114d2,0x114d3,0x114d4,0x114d5,0x114d6,0x114d7,0x114d8,0x114d9,0x11650,
	0x11651,0x11652,0x11653,0x11654,0x11655,0x11656,0x11657,0x11658,0x11659,
	0x116c0,0x116c1,0x116c2,0x116c3,0x116c4,0x116c5,0x116c6,0x116c7,0x116c8,
	0x116c9,0x118e0,0x118e1,0x118e2,0x118e3,0x118e4,0x118e5,0x118e6,0x118e7,
	0x118e8,0x118e9,0x16a60,0x16a61,0x16a62,0x16a63,0x16a64,0x16a65,0x16a66,
	0x16a67,0x16a68,0x16a69,0x16b50,0x16b51,0x16b52,0x16b53,0x16b54,0x16b55,
	0x16b56,0x16b57,0x16b58,0x16b59,0x1d7ce,0x1d7cf,0x1d7d0,0x1d7d1,0x1d7d2,
	0x1d7d3,0x1d7d4,0x1d7d5,0x1d7d6,0x1d7d7,0x1d7d8,0x1d7d9,0x1d7da,0x1d7db,
	0x1d7dc,0x1d7dd,0x1d7de,0x1d7df,0x1d7e0,0x1d7e1,0x1d7e2,0x1d7e3,0x1d7e4,
	0x1d7e5,0x1d7e6,0x1d7e7,0x1d7e8,0x1d7e9,0x1d7ea,0x1d7eb,0x1d7ec,0x1d7ed,
	0x1d7ee,0x1d7ef,0x1d7f0,0x1d7f1,0x1d7f2,0x1d7f3,0x1d7f4,0x1d7f5,0x1d7f6,
	0x1d7f7,0x1d7f8,0x1d7f9,0x1d7fa,0x1d7fb,0x1d7fc,0x1d7fd,0x1d7fe,0x1d7ff
};