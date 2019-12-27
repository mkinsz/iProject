#ifndef EDID_CEA861_H
#define EDID_CEA861_H

#include <cstdint>

#define CEA861_NO_DTDS_PRESENT                          (0x04)

enum cea861_data_block_type {
	CEA861_DATA_BLOCK_TYPE_RESERVED0,
	CEA861_DATA_BLOCK_TYPE_AUDIO,
	CEA861_DATA_BLOCK_TYPE_VIDEO,
	CEA861_DATA_BLOCK_TYPE_VENDOR_SPECIFIC,
	CEA861_DATA_BLOCK_TYPE_SPEAKER_ALLOCATION,
	CEA861_DATA_BLOCK_TYPE_VESA_DTC,
	CEA861_DATA_BLOCK_TYPE_RESERVED6,
	CEA861_DATA_BLOCK_TYPE_EXTENDED,
};

enum cea861_audio_format {
	CEA861_AUDIO_FORMAT_RESERVED,
	CEA861_AUDIO_FORMAT_LPCM,
	CEA861_AUDIO_FORMAT_AC_3,
	CEA861_AUDIO_FORMAT_MPEG_1,
	CEA861_AUDIO_FORMAT_MP3,
	CEA861_AUDIO_FORMAT_MPEG2,
	CEA861_AUDIO_FORMAT_AAC_LC,
	CEA861_AUDIO_FORMAT_DTS,
	CEA861_AUDIO_FORMAT_ATRAC,
	CEA861_AUDIO_FORMAT_DSD,
	CEA861_AUDIO_FORMAT_E_AC_3,
	CEA861_AUDIO_FORMAT_DTS_HD,
	CEA861_AUDIO_FORMAT_MLP,
	CEA861_AUDIO_FORMAT_DST,
	CEA861_AUDIO_FORMAT_WMA_PRO,
	CEA861_AUDIO_FORMAT_EXTENDED,
};

struct cea861_timing_block {
	/* CEA Extension Header */
	uint8_t  tag;
	uint8_t  revision;
	uint8_t  dtd_offset;

	/* Global Declarations */
	uint8_t native_dtds : 4;
	uint8_t yuv_422_supported : 1;
	uint8_t yuv_444_supported : 1;
	uint8_t basic_audio_supported : 1;
	uint8_t underscan_supported : 1;

	uint8_t  data[123];

	uint8_t  checksum;
};

struct cea861_data_block_header {
	uint8_t length : 5;
	uint8_t tag : 3;
};

struct cea861_short_video_descriptor {
	uint8_t video_identification_code : 7;
	uint8_t native : 1;
};

struct cea861_video_data_block {
	struct cea861_data_block_header      header;
	struct cea861_short_video_descriptor svd[];
};

struct cea861_short_audio_descriptor {
	uint8_t channels : 3; /* = value + 1 */
	uint8_t audio_format : 4;
	uint8_t : 1;

	uint8_t sample_rate_32_kHz : 1;
	uint8_t sample_rate_44_1_kHz : 1;
	uint8_t sample_rate_48_kHz : 1;
	uint8_t sample_rate_88_2_kHz : 1;
	uint8_t sample_rate_96_kHz : 1;
	uint8_t sample_rate_176_4_kHz : 1;
	uint8_t sample_rate_192_kHz : 1;
	uint8_t : 1;

	union {
		struct {
			uint8_t bitrate_16_bit : 1;
			uint8_t bitrate_20_bit : 1;
			uint8_t bitrate_24_bit : 1;
			uint8_t : 5;
		} lpcm;

		uint8_t maximum_bit_rate;       /* formats 2-8; = value * 8 kHz */
		uint8_t format_dependent;       /* formats 9-13; */

		struct {
			uint8_t profile : 3;
			uint8_t : 5;
		} wma_pro;

		struct {
			uint8_t : 3;
			uint8_t code : 5;
		} extension;
	} flags;
};

struct cea861_audio_data_block {
	struct cea861_data_block_header      header;
	struct cea861_short_audio_descriptor sad[];
};

struct cea861_speaker_allocation {
	uint8_t front_left_right : 1;
	uint8_t front_lfe : 1;   /* low frequency effects */
	uint8_t front_center : 1;
	uint8_t rear_left_right : 1;
	uint8_t rear_center : 1;
	uint8_t front_left_right_center : 1;
	uint8_t rear_left_right_center : 1;
	uint8_t front_left_right_wide : 1;

	uint8_t front_left_right_high : 1;
	uint8_t top_center : 1;
	uint8_t front_center_high : 1;
	uint8_t : 5;

	uint8_t : 8;
};

struct cea861_speaker_allocation_data_block {
	struct cea861_data_block_header  header;
	struct cea861_speaker_allocation payload;
};

struct cea861_vendor_specific_data_block {
	struct cea861_data_block_header  header;
	uint8_t                          ieee_registration[3];
	uint8_t                          data[30];
};

typedef enum {
	INTERLACED,
	PROGRESSIVE,
} EMODE;

static const struct cea861_timing {
	const uint16_t hactive;
	const uint16_t vactive;
	const EMODE	   mode;
	const uint16_t htotal;
	const uint16_t hblank;
	const uint16_t vtotal;
	const double   vblank;
	const double   hfreq;
	const double   vfreq;
	const double   pixclk;
} cea861_timings[] = {
	{  640,  480, PROGRESSIVE,  800,  160,  525, 45.0,  31.469,  59.940,  25.175 },
	{  720,  480, PROGRESSIVE,  858,  138,  525, 45.0,  31.469,  59.940,  27.000 },
	{  720,  480, PROGRESSIVE,  858,  138,  525, 45.0,  31.469,  59.940,  27.000 },
	{ 1280,  720, PROGRESSIVE, 1650,  370,  750, 30.0,  45.000,  60.000,  74.250 },
	{ 1920, 1080,  INTERLACED, 2200,  280, 1125, 22.5,  33.750,  60.000,  72.250 },
	{ 1440,  480,  INTERLACED, 1716,  276,  525, 22.5,  15.734,  59.940,  27.000 },
	{ 1440,  480,  INTERLACED, 1716,  276,  525, 22.5,  15.734,  59.940,  27.000 },
	{ 1440,  240, PROGRESSIVE, 1716,  276,  262, 22.0,  15.734,  60.054,  27.000 },  /* 9 */
	{ 1440,  240, PROGRESSIVE, 1716,  276,  262, 22.0,  15.734,  59.826,  27.000 },  /* 8 */
	{ 2880,  480,  INTERLACED, 3432,  552,  525, 22.5,  15.734,  59.940,  54.000 },
	{ 2880,  480,  INTERLACED, 3432,  552,  525, 22.5,  15.734,  59.940,  54.000 },
	{ 2880,  240, PROGRESSIVE, 3432,  552,  262, 22.0,  15.734,  60.054,  54.000 },  /* 13 */
	{ 2880,  240, PROGRESSIVE, 3432,  552,  262, 22.0,  15.734,  59.826,  54.000 },  /* 12 */
	{ 1440,  480, PROGRESSIVE, 1716,  276,  525, 45.0,  31.469,  59.940,  54.000 },
	{ 1440,  480, PROGRESSIVE, 1716,  276,  525, 45.0,  31.469,  59.940,  54.000 },
	{ 1920, 1080, PROGRESSIVE, 2200,  280, 1125, 45.0,  67.500,  60.000, 148.500 },
	{  720,  576, PROGRESSIVE,  864,  144,  625, 49.0,  31.250,  50.000,  27.000 },
	{  720,  576, PROGRESSIVE,  864,  144,  625, 49.0,  31.250,  50.000,  27.000 },
	{ 1280,  720, PROGRESSIVE, 1980,  700,  750, 30.0,  37.500,  50.000,  74.250 },
	{ 1920, 1080,  INTERLACED, 2640,  720, 1125, 22.5,  28.125,  50.000,  74.250 },
	{ 1440,  576,  INTERLACED, 1728,  288,  625, 24.5,  15.625,  50.000,  27.000 },
	{ 1440,  576,  INTERLACED, 1728,  288,  625, 24.5,  15.625,  50.000,  27.000 },
	{ 1440,  288, PROGRESSIVE, 1728,  288,  312, 24.0,  15.625,  50.080,  27.000 },
	{ 1440,  288, PROGRESSIVE, 1728,  288,  313, 25.0,  15.625,  49.920,  27.000 },
	{ 2880,  576,  INTERLACED, 3456,  576,  625, 24.5,  15.625,  50.000,  54.000 },
	{ 2880,  576,  INTERLACED, 3456,  576,  625, 24.5,  15.625,  50.000,  54.000 },
	{ 2880,  288, PROGRESSIVE, 3456,  576,  312, 24.0,  15.625,  50.080,  54.000 },
	{ 2880,  288, PROGRESSIVE, 3456,  576,  313, 25.0,  15.625,  49.920,  54.000 },
	{ 1440,  576, PROGRESSIVE, 1728,  288,  625, 49.0,  31.250,  50.000,  54.000 },
	{ 1440,  576, PROGRESSIVE, 1728,  288,  625, 49.0,  31.250,  50.000,  54.000 },
	{ 1920, 1080, PROGRESSIVE, 2640,  720, 1125, 45.0,  56.250,  50.000, 148.500 },
	{ 1920, 1080, PROGRESSIVE, 2750,  830, 1125, 45.0,  27.000,  24.000,  74.250 },
	{ 1920, 1080, PROGRESSIVE, 2640,  720, 1125, 45.0,  28.125,  25.000,  74.250 },
	{ 1920, 1080, PROGRESSIVE, 2200,  280, 1125, 45.0,  33.750,  30.000,  74.250 },
	{ 2880,  480, PROGRESSIVE, 3432,  552,  525, 45.0,  31.469,  59.940, 108.500 },
	{ 2880,  480, PROGRESSIVE, 3432,  552,  525, 45.0,  31.469,  59.940, 108.500 },
	{ 2880,  576, PROGRESSIVE, 3456,  576,  625, 49.0,  31.250,  50.000, 108.000 },
	{ 2880,  576, PROGRESSIVE, 3456,  576,  625, 49.0,  31.250,  50.000, 108.000 },
	{ 1920, 1080,  INTERLACED, 2304,  384, 1250, 85.0,  31.250,  50.000,  72.000 },
	{ 1920, 1080,  INTERLACED, 2640,  720, 1125, 22.5,  56.250, 100.000, 148.500 },
	{ 1280,  720, PROGRESSIVE, 1980,  700,  750, 30.0,  75.000, 100.000, 148.500 },
	{  720,  576, PROGRESSIVE,  864,  144,  625, 49.0,  62.500, 100.000,  54.000 },
	{  720,  576, PROGRESSIVE,  864,  144,  625, 49.0,  62.500, 100.000,  54.000 },
	{ 1440,  576,  INTERLACED, 1728,  288,  625, 24.5,  31.250, 100.000,  54.000 },
	{ 1440,  576,  INTERLACED, 1728,  288,  625, 24.5,  31.250, 100.000,  54.000 },
	{ 1920, 1080,  INTERLACED, 2200,  280, 1125, 22.5,  67.500, 120.000, 148.500 },
	{ 1280,  720, PROGRESSIVE, 1650,  370,  750, 30.0,  90.000, 120.000, 148.500 },
	{  720,  480, PROGRESSIVE,  858,  138,  525, 45.0,  62.937, 119.880,  54.000 },
	{  720,  480, PROGRESSIVE,  858,  138,  525, 45.0,  62.937, 119.880,  54.000 },
	{ 1440,  480,  INTERLACED, 1716,  276,  525, 22.5,  31.469, 119.880,  54.000 },
	{ 1440,  480,  INTERLACED, 1716,  276,  525, 22.5,  31.469, 119.880,  54.000 },
	{  720,  576, PROGRESSIVE,  864,  144,  625, 49.0, 125.000, 200.000, 108.000 },
	{  720,  576, PROGRESSIVE,  864,  144,  625, 49.0, 125.000, 200.000, 108.000 },
	{ 1440,  576,  INTERLACED, 1728,  288,  625, 24.5,  62.500, 200.000, 108.000 },
	{ 1440,  576,  INTERLACED, 1728,  288,  625, 24.5,  62.500, 200.000, 108.000 },
	{  720,  480, PROGRESSIVE,  858,  138,  525, 45.0, 125.874, 239.760, 108.000 },
	{  720,  480, PROGRESSIVE,  858,  138,  525, 45.0, 125.874, 239.760, 108.000 },
	{ 1440,  480,  INTERLACED, 1716,  276,  525, 22.5,  62.937, 239.760, 108.000 },
	{ 1440,  480,  INTERLACED, 1716,  276,  525, 22.5,  62.937, 239.760, 108.000 },
	{ 1280,  720, PROGRESSIVE, 3300, 2020,  750, 30.0,  18.000,  24.000,  59.400 },
	{ 1280,  720, PROGRESSIVE, 3960, 2680,  750, 30.0,  18.750,  25.000,  74.250 },
	{ 1280,  720, PROGRESSIVE, 3300, 2020,  750, 30.0,  22.500,  30.000,  74.250 },
	{ 1920, 1080, PROGRESSIVE, 2200,  280, 1125, 45.0, 135.000, 120.000, 297.000 },
	{ 1920, 1080, PROGRESSIVE, 2640,  720, 1125, 45.0, 112.500, 100.000, 297.000 },
};

#endif // EDID_CEA861_H