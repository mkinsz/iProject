#ifndef CEDID
#define CEDID

#include <cstdint>

#define EDID_I2C_DDC_DATA_ADDRESS               (0x50)

#define EDID_BLOCK_SIZE                         (0x80)
#define EDID_MAX_EXTENSIONS                     (0xfe)

/* ------------------------ VESA ------------------------ */

static const uint8_t EDID_HEADER[] = { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };
static const uint8_t EDID_STANDARD_TIMING_DESCRIPTOR_INVALID[] = { 0x01, 0x01 };

enum edid_extension_type {
	EDID_EXTENSION_TIMING = 0x01, // Timing Extension
	EDID_EXTENSION_CEA = 0x02, // Additional Timing Block Data (CEA EDID Timing Extension)
	EDID_EXTENSION_VTB = 0x10, // Video Timing Block Extension (VTB-EXT)
	EDID_EXTENSION_EDID_2_0 = 0x20, // EDID 2.0 Extension
	EDID_EXTENSION_DI = 0x40, // Display Information Extension (DI-EXT)
	EDID_EXTENSION_LS = 0x50, // Localised String Extension (LS-EXT)
	EDID_EXTENSION_MI = 0x60, // Microdisplay Interface Extension (MI-EXT)
	EDID_EXTENSION_DTCDB_1 = 0xa7, // Display Transfer Characteristics Data Block (DTCDB)
	EDID_EXTENSION_DTCDB_2 = 0xaf,
	EDID_EXTENSION_DTCDB_3 = 0xbf,
	EDID_EXTENSION_BLOCK_MAP = 0xf0, // Block Map
	EDID_EXTENSION_DDDB = 0xff, // Display Device Data Block (DDDB)
};

enum edid_aspect_ratio {
	EDID_ASPECT_RATIO_16_10,
	EDID_ASPECT_RATIO_4_3,
	EDID_ASPECT_RATIO_5_4,
	EDID_ASPECT_RATIO_16_9,
};

enum edid_display_type {
	EDID_DISPLAY_TYPE_MONOCHROME,
	EDID_DISPLAY_TYPE_RGB,
	EDID_DISPLAY_TYPE_NON_RGB,
	EDID_DISPLAY_TYPE_UNDEFINED,
};

enum edid_signal_sync {
	EDID_SIGNAL_SYNC_ANALOG_COMPOSITE,
	EDID_SIGNAL_SYNC_BIPOLAR_ANALOG_COMPOSITE,
	EDID_SIGNAL_SYNC_DIGITAL_COMPOSITE,
	EDID_SIGNAL_SYNC_DIGITAL_SEPARATE,
};

enum edid_stereo_mode {
	EDID_STEREO_MODE_NONE,
	EDID_STEREO_MODE_RESERVED,
	EDID_STEREO_MODE_FIELD_SEQUENTIAL_RIGHT,
	EDID_STEREO_MODE_2_WAY_INTERLEAVED_RIGHT,
	EDID_STEREO_MODE_FIELD_SEQUENTIAL_LEFT,
	EDID_STEREO_MODE_2_WAY_INTERLEAVED_LEFT,
	EDID_STEREO_MODE_4_WAY_INTERLEAVED,
	EDID_STEREO_MODE_SIDE_BY_SIDE_INTERLEAVED,
};

enum edid_monitor_descriptor_type {
	EDID_MONTIOR_DESCRIPTOR_MANUFACTURER_DEFINED = 0x0f,
	EDID_MONITOR_DESCRIPTOR_STANDARD_TIMING_IDENTIFIERS = 0xfa,
	EDID_MONITOR_DESCRIPTOR_COLOR_POINT = 0xfb,
	EDID_MONITOR_DESCRIPTOR_MONITOR_NAME = 0xfc,
	EDID_MONITOR_DESCRIPTOR_MONITOR_RANGE_LIMITS = 0xfd,
	EDID_MONITOR_DESCRIPTOR_ASCII_STRING = 0xfe,
	EDID_MONITOR_DESCRIPTOR_MONITOR_SERIAL_NUMBER = 0xff,
};

enum edid_secondary_timing_support {
	EDID_SECONDARY_TIMING_NOT_SUPPORTED,
	EDID_SECONDARY_TIMING_GFT = 0x02,
};


struct edid_extension {
	uint8_t tag;
	uint8_t revision;
	uint8_t extension_data[125];
	uint8_t checksum;
};

struct edid_standard_timing_descriptor {
	uint8_t  horizontal_active_pixels;         /* = (value + 31) * 8 */

	uint8_t refresh_rate : 6;           /* = value + 60 */
	uint8_t image_aspect_ratio : 2;
};

struct edid_monitor_descriptor {
	uint16_t flag0;
	uint8_t  flag1;
	uint8_t  tag;
	uint8_t  flag2;
	uint8_t  data[13];
};

struct edid_detailed_timing_descriptor {
	uint16_t pixel_clock;                              /* = value * 10000 */

	uint8_t  horizontal_active_lo;
	uint8_t  horizontal_blanking_lo;

	uint8_t horizontal_blanking_hi : 4;
	uint8_t horizontal_active_hi : 4;

	uint8_t  vertical_active_lo;
	uint8_t  vertical_blanking_lo;

	uint8_t vertical_blanking_hi : 4;
	uint8_t vertical_active_hi : 4;

	uint8_t  horizontal_sync_offset_lo;
	uint8_t  horizontal_sync_pulse_width_lo;

	uint8_t vertical_sync_pulse_width_lo : 4;
	uint8_t vertical_sync_offset_lo : 4;

	uint8_t vertical_sync_pulse_width_hi : 2;
	uint8_t vertical_sync_offset_hi : 2;
	uint8_t horizontal_sync_pulse_width_hi : 2;
	uint8_t horizontal_sync_offset_hi : 2;

	uint8_t  horizontal_image_size_lo;
	uint8_t  vertical_image_size_lo;

	uint8_t vertical_image_size_hi : 4;
	uint8_t horizontal_image_size_hi : 4;

	uint8_t  horizontal_border;
	uint8_t  vertical_border;

	uint8_t stereo_mode_lo : 1;
	uint8_t signal_pulse_polarity : 1; /* pulse on sync, composite/horizontal polarity */
	uint8_t signal_serration_polarity : 1; /* serrate on sync, vertical polarity */
	uint8_t signal_sync : 2;
	uint8_t stereo_mode_hi : 2;
	uint8_t interlaced : 1;
};

typedef char edid_monitor_descriptor_string[sizeof(((struct edid_monitor_descriptor*)0)->data) + 1];

struct edid_monitor_range_limits {
	uint8_t  minimum_vertical_rate;             /* Hz */
	uint8_t  maximum_vertical_rate;             /* Hz */
	uint8_t  minimum_horizontal_rate;           /* kHz */
	uint8_t  maximum_horizontal_rate;           /* kHz */
	uint8_t  maximum_supported_pixel_clock;     /* = (value * 10) Mhz (round to 10 MHz) */

	/* secondary timing formula */
	uint8_t  secondary_timing_support;
	uint8_t  reserved;
	uint8_t  secondary_curve_start_frequency;   /* horizontal frequency / 2 kHz */
	uint8_t  c;                                 /* = (value >> 1) */
	uint16_t m;
	uint8_t  k;
	uint8_t  j;                                 /* = (value >> 1) */
};


struct edid {
	/* header information */
	uint8_t  header[8];

	/* vendor/product identification */
	uint16_t manufacturer;

	uint8_t  product[2];
	uint8_t  serial_number[4];
	uint8_t  manufacture_week;
	uint8_t  manufacture_year;                  /* = value + 1990 */

	/* EDID version */
	uint8_t  version;
	uint8_t  revision;

	/* basic display parameters and features */
	union {
		struct {
			uint8_t dfp_1x : 1;    /* VESA DFP 1.x */
			uint8_t : 6;
			uint8_t digital : 1;
		} digital;
		struct {
			uint8_t vsync_serration : 1;
			uint8_t green_video_sync : 1;
			uint8_t composite_sync : 1;
			uint8_t separate_sync : 1;
			uint8_t blank_to_black_setup : 1;
			uint8_t signal_level_standard : 2;
			uint8_t digital : 1;
		} analog;
	} video_input_definition;

	uint8_t  maximum_horizontal_image_size;     /* cm */
	uint8_t  maximum_vertical_image_size;       /* cm */

	uint8_t  display_transfer_characteristics;  /* gamma = (value + 100) / 100 */

	struct {
		uint8_t default_gtf : 1; /* generalised timing formula */
		uint8_t preferred_timing_mode : 1;
		uint8_t standard_default_color_space : 1;
		uint8_t display_type : 2;
		uint8_t active_off : 1;
		uint8_t suspend : 1;
		uint8_t standby : 1;
	} feature_support;

	/* color characteristics block */
	uint8_t green_y_low : 2;
	uint8_t green_x_low : 2;
	uint8_t red_y_low : 2;
	uint8_t red_x_low : 2;

	uint8_t white_y_low : 2;
	uint8_t white_x_low : 2;
	uint8_t blue_y_low : 2;
	uint8_t blue_x_low : 2;

	uint8_t  red_x;
	uint8_t  red_y;
	uint8_t  green_x;
	uint8_t  green_y;
	uint8_t  blue_x;
	uint8_t  blue_y;
	uint8_t  white_x;
	uint8_t  white_y;

	/* established timings */
	struct {
		uint8_t timing_800x600_60 : 1;
		uint8_t timing_800x600_56 : 1;
		uint8_t timing_640x480_75 : 1;
		uint8_t timing_640x480_72 : 1;
		uint8_t timing_640x480_67 : 1;
		uint8_t timing_640x480_60 : 1;
		uint8_t timing_720x400_88 : 1;
		uint8_t timing_720x400_70 : 1;

		uint8_t timing_1280x1024_75 : 1;
		uint8_t timing_1024x768_75 : 1;
		uint8_t timing_1024x768_70 : 1;
		uint8_t timing_1024x768_60 : 1;
		uint8_t timing_1024x768_87 : 1;
		uint8_t timing_832x624_75 : 1;
		uint8_t timing_800x600_75 : 1;
		uint8_t timing_800x600_72 : 1;
	} established_timings;

	struct {
		uint8_t reserved : 7;
		uint8_t timing_1152x870_75 : 1;
	} manufacturer_timings;

	/* standard timing id */
	struct  edid_standard_timing_descriptor standard_timing_id[8];

	/* detailed timing */
	union {
		struct edid_monitor_descriptor         monitor;
		struct edid_detailed_timing_descriptor timing;
	} detailed_timings[4];

	uint8_t  extensions;
	uint8_t  checksum;
};

class CEdid
{
public:
	explicit CEdid(const char * const filename);
	~CEdid();

	void parse_edid();

private:
	uint8_t* m_buf;
};

#endif // CEDID