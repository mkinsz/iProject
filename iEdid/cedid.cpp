#include "cedid.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <string.h>
#include "utils.h"
#include "hdmi.h"

#define ARRAY_SIZE(arr)			(sizeof(arr) / sizeof(arr[0]))

#define CM_2_MM(cm)                             ((cm) * 10)
#define CM_2_IN(cm)                             ((cm) * 0.3937)

#define HZ_2_MHZ(hz)                            ((hz) / 1000000)

static inline uint32_t
edid_detailed_timing_pixel_clock(const struct edid_detailed_timing_descriptor* const dtb)
{
	return dtb->pixel_clock * 10000;
}

static inline uint16_t
edid_detailed_timing_horizontal_blanking(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->horizontal_blanking_hi << 8) | dtb->horizontal_blanking_lo;
}

static inline uint16_t
edid_detailed_timing_horizontal_active(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->horizontal_active_hi << 8) | dtb->horizontal_active_lo;
}

static inline uint16_t
edid_detailed_timing_vertical_blanking(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->vertical_blanking_hi << 8) | dtb->vertical_blanking_lo;
}

static inline uint16_t
edid_detailed_timing_vertical_active(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->vertical_active_hi << 8) | dtb->vertical_active_lo;
}

static inline uint8_t
edid_detailed_timing_vertical_sync_offset(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->vertical_sync_offset_hi << 4) | dtb->vertical_sync_offset_lo;
}

static inline uint8_t
edid_detailed_timing_vertical_sync_pulse_width(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->vertical_sync_pulse_width_hi << 4) | dtb->vertical_sync_pulse_width_lo;
}

static inline uint8_t
edid_detailed_timing_horizontal_sync_offset(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->horizontal_sync_offset_hi << 4) | dtb->horizontal_sync_offset_lo;
}

static inline uint8_t
edid_detailed_timing_horizontal_sync_pulse_width(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->horizontal_sync_pulse_width_hi << 4) | dtb->horizontal_sync_pulse_width_lo;
}

static inline uint16_t
edid_detailed_timing_horizontal_image_size(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->horizontal_image_size_hi << 8) | dtb->horizontal_image_size_lo;
}

static inline uint16_t
edid_detailed_timing_vertical_image_size(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->vertical_image_size_hi << 8) | dtb->vertical_image_size_lo;
}

static inline uint8_t
edid_detailed_timing_stereo_mode(const struct edid_detailed_timing_descriptor* const dtb)
{
	return (dtb->stereo_mode_hi << 2 | dtb->stereo_mode_lo);
}

static inline bool
edid_verify_checksum(const uint8_t* const block)
{
	uint8_t checksum = 0;
	int i;

	for (i = 0; i < EDID_BLOCK_SIZE; i++)
		checksum += block[i];

	return (checksum == 0);
}

static inline double
edid_decode_fixed_point(uint16_t value)
{
	double result = 0.0;

	assert((~value & 0xfc00) == 0xfc00);    /* edid fraction is 10 bits */

	for (uint8_t i = 0; value && (i < 10); i++, value >>= 1)
		result = result + ((value & 0x1) * (1.0 / (1 << (10 - i))));

	return result;
}

static inline const char* const
_aspect_ratio(const uint16_t hres, const uint16_t vres)
{
#define HAS_RATIO_OF(x, y)  (hres == (vres * (x) / (y)) && !((vres * (x)) % (y)))

	if (HAS_RATIO_OF(16, 10))
		return "16:10";
	if (HAS_RATIO_OF(4, 3))
		return "4:3";
	if (HAS_RATIO_OF(5, 4))
		return "5:4";
	if (HAS_RATIO_OF(16, 9))
		return "16:9";

#undef HAS_RATIO

	return "unknown";
}

static inline void
dump_section(const char* const name,
	const uint8_t* const buffer,
	const uint8_t offset,
	const uint8_t length)
{
	/*!  \todo remove magic number usage */

	const uint8_t* value = buffer + offset;

	printf("%33.33s: ", name);
	//OutputLog("%33.33s: ", name);

	for (uint8_t i = 0, l = 35; i < length; i++) {
		if ((l += 3) > 89) {
			printf("\b\n%35s", "");
			//OutputLog("\b\n%35s", "");
			l = 35;
		}
		printf("%02x ", *value++);
		//OutputLog("%02x ", *value++);
	}

	printf("\b\n");
	//OutputLog("\n");
}

static void
dump_edid(const uint8_t* const buffer)
{
	printf("edid info: \n");

	dump_section("header", buffer, 0x00, 0x08);
	dump_section("vendor/product identification", buffer, 0x08, 0x0a);
	dump_section("edid struct version/revision", buffer, 0x12, 0x02);
	dump_section("basic display parameters/features", buffer, 0x14, 0x05);
	dump_section("color characteristics", buffer, 0x19, 0x0a);
	dump_section("established timings", buffer, 0x23, 0x03);
	dump_section("standard timing identification", buffer, 0x26, 0x10);
	dump_section("detailed timing 0", buffer, 0x36, 0x12);
	dump_section("detailed timing 1", buffer, 0x48, 0x12);
	dump_section("detailed timing 2", buffer, 0x5a, 0x12);
	dump_section("detailed timing 3", buffer, 0x6c, 0x12);
	dump_section("extensions", buffer, 0x7e, 0x01);
	dump_section("checksum", buffer, 0x7f, 0x01);

	printf("\n");
	//OutputLog("\n");
}

static inline void
edid_manufacturer(const struct edid* const edid, char manufacturer[4])
{
	manufacturer[0] = '@' + ((edid->manufacturer & 0x007c) >> 2);
	manufacturer[1] = '@' + (((edid->manufacturer & 0x0003) >> 00) << 3)
		| (((edid->manufacturer & 0xe000) >> 13) << 0);
	manufacturer[2] = '@' + ((edid->manufacturer & 0x1f00) >> 8);
	manufacturer[3] = '\0';
}

static inline bool
edid_detailed_timing_is_monitor_descriptor(const struct edid* const edid,
	const uint8_t timing)
{
	const struct edid_monitor_descriptor* const mon =
		&edid->detailed_timings[timing].monitor;

	assert(timing < ARRAY_SIZE(edid->detailed_timings));

	return mon->flag0 == 0x0000 && mon->flag1 == 0x00 && mon->flag2 == 0x00;
}

static inline std::string
_edid_timing_string(const struct edid_detailed_timing_descriptor* const dtb)
{
	char timing[1024];
	const uint16_t hres = edid_detailed_timing_horizontal_active(dtb);
	const uint16_t vres = edid_detailed_timing_vertical_active(dtb);
	const uint32_t htotal = hres + edid_detailed_timing_horizontal_blanking(dtb);
	const uint32_t vtotal = vres + edid_detailed_timing_vertical_blanking(dtb);

	sprintf_s(timing, sizeof(timing)-1,
		"%ux%u%c at %.fHz (%s)",
		hres,
		vres,
		dtb->interlaced ? 'i' : 'p',
		(double)edid_detailed_timing_pixel_clock(dtb) / (vtotal * htotal),
		_aspect_ratio(hres, vres));

	return timing;
}

const inline uint32_t
edid_standard_timing_horizontal_active(const struct edid_standard_timing_descriptor* const desc)
{
	return ((desc->horizontal_active_pixels + 31) << 3);
}

const inline uint32_t
edid_standard_timing_vertical_active(const struct edid_standard_timing_descriptor* const desc)
{
	const uint32_t hres = edid_standard_timing_horizontal_active(desc);

	switch (desc->image_aspect_ratio) {
	case EDID_ASPECT_RATIO_16_10:
		return ((hres * 10) >> 4);
	case EDID_ASPECT_RATIO_4_3:
		return ((hres * 3) >> 2);
	case EDID_ASPECT_RATIO_5_4:
		return ((hres << 2) / 5);
	case EDID_ASPECT_RATIO_16_9:
		return ((hres * 9) >> 4);
	}

	return hres;
}

const inline uint32_t
edid_standard_timing_refresh_rate(const struct edid_standard_timing_descriptor* const desc)
{
	return (desc->refresh_rate + 60);
}

static inline std::string
_edid_mode_string(const struct edid_detailed_timing_descriptor* const dtb)
{
	char modestr[1024];
	const uint16_t xres = edid_detailed_timing_horizontal_active(dtb);
	const uint16_t yres = edid_detailed_timing_vertical_active(dtb);
	const uint32_t pixclk = edid_detailed_timing_pixel_clock(dtb);
	const uint16_t lower_margin = edid_detailed_timing_vertical_sync_offset(dtb);
	const uint16_t right_margin = edid_detailed_timing_horizontal_sync_offset(dtb);

	sprintf_s(modestr, sizeof(modestr)-1,
		"\"%ux%u\" %.3f %u %u %u %u %u %u %u %u %chsync %cvsync", 
		/* resolution */
		xres, yres,

		/* dot clock frequence (MHz) */
		HZ_2_MHZ((double)pixclk),

		/* horizontal timings */
		xres,
		xres + right_margin,
		xres + right_margin + edid_detailed_timing_horizontal_sync_pulse_width(dtb),
		xres + edid_detailed_timing_horizontal_blanking(dtb),

		/* vertical timings */
		yres,
		yres + lower_margin,
		yres + lower_margin + edid_detailed_timing_vertical_sync_pulse_width(dtb),
		yres + edid_detailed_timing_vertical_blanking(dtb),

		/* sync direction */
		dtb->signal_pulse_polarity ? '+' : '-',
		dtb->signal_serration_polarity ? '+' : '-');

	return modestr;
}

static void
disp_edid(const struct edid* const edid)
{
	const struct edid_monitor_range_limits* monitor_range_limits = NULL;
	edid_monitor_descriptor_string monitor_serial_number = { 0 };
	edid_monitor_descriptor_string monitor_model_name = { 0 };
	bool has_ascii_string = false;
	char manufacturer[4] = { 0 };

	//struct edid_color_characteristics_data characteristics;
	const uint8_t vlen = edid->maximum_vertical_image_size;
	const uint8_t hlen = edid->maximum_horizontal_image_size;
	uint8_t i;

	edid_manufacturer(edid, manufacturer);
	//characteristics = edid_color_characteristics(edid);


	for (i = 0; i < ARRAY_SIZE(edid->detailed_timings); i++) {
		const struct edid_monitor_descriptor* const mon =
			&edid->detailed_timings[i].monitor;

		if (!edid_detailed_timing_is_monitor_descriptor(edid, i))
			continue;

		switch (mon->tag) {
		case EDID_MONTIOR_DESCRIPTOR_MANUFACTURER_DEFINED:
			/* This is arbitrary data, just silently ignore it. */
			break;
		case EDID_MONITOR_DESCRIPTOR_ASCII_STRING:
			has_ascii_string = true;
			break;
		case EDID_MONITOR_DESCRIPTOR_MONITOR_NAME: {
			strncpy_s(monitor_model_name, (char*)mon->data,
				sizeof(monitor_model_name) - 1);
			char* p = strchr(monitor_model_name, '\n');
			if (p) *p = '\0';
		}
			break;
		case EDID_MONITOR_DESCRIPTOR_MONITOR_RANGE_LIMITS:
			monitor_range_limits = (struct edid_monitor_range_limits*) & mon->data;
			break;
		case EDID_MONITOR_DESCRIPTOR_MONITOR_SERIAL_NUMBER: {
			strncpy_s(monitor_serial_number, (char*)mon->data,
				sizeof(monitor_serial_number) - 1);
			char* p = strchr(monitor_serial_number, '\n');
			if (p) *p = '\0';
		}
			break;
		default:
			fprintf(stderr, "unknown monitor descriptor type 0x%02x\n",
				mon->tag);
			break;
		}
	}

	printf("Monitor\n");

	printf("  Model name............... %s\n",
		*monitor_model_name ? monitor_model_name : "n/a");

	printf("  Manufacturer............. %s\n",
		manufacturer);

	printf("  Product code............. %u\n",
		*(uint16_t*)edid->product);

	if (*(uint32_t*)edid->serial_number)
		printf("  Module serial number..... %u\n",
			*(uint32_t*)edid->serial_number);

#if defined(DISPLAY_UNKNOWN)
	printf("  Plug and Play ID......... %s\n", NULL);
#endif

	printf("  Serial number............ %s\n",
		*monitor_serial_number ? monitor_serial_number : "n/a");

	printf("  Manufacture date......... %u", edid->manufacture_year + 1990);
	if (edid->manufacture_week <= 52)
		printf(", ISO week %u", edid->manufacture_week);
	printf("\n");

	printf("  EDID revision............ %u.%u\n",
		edid->version, edid->revision);

	printf("  Input signal type........ %s\n",
		edid->video_input_definition.digital.digital ? "Digital" : "Analog");

	if (edid->video_input_definition.digital.digital) {
		printf("  VESA DFP 1.x supported... %s\n",
			edid->video_input_definition.digital.dfp_1x ? "Yes" : "No");
	}
	else {
		/* TODO print analog flags */
	}

	std::string sztype;
	switch (edid->feature_support.display_type) {
	case EDID_DISPLAY_TYPE_MONOCHROME: 
		sztype = "Monochrome or greyscale"; break;
	case EDID_DISPLAY_TYPE_RGB:
		sztype = "sRGB colour"; break;
	case EDID_DISPLAY_TYPE_NON_RGB: 
		sztype = "Non-sRGB colour"; break;
	case EDID_DISPLAY_TYPE_UNDEFINED:
		sztype = "Undefined"; break;
	}

	printf("  Display type............. %s\n", sztype.data());

	printf("  Screen size.............. %u mm x %u mm (%.1f in)\n",
		CM_2_MM(hlen), CM_2_MM(vlen),
		CM_2_IN(sqrt(hlen * hlen + vlen * vlen)));

	printf("  Power management......... %s%s%s%s\n",
		edid->feature_support.active_off ? "Active off, " : "",
		edid->feature_support.suspend ? "Suspend, " : "",
		edid->feature_support.standby ? "Standby, " : "",

		(edid->feature_support.active_off ||
			edid->feature_support.suspend ||
			edid->feature_support.standby) ? "\b\b  " : "n/a");

	printf("  Extension blocks......... %u\n",
		edid->extensions);

	printf("\n");

	if (has_ascii_string) {
		edid_monitor_descriptor_string string = { 0 };

		printf("General purpose ASCII string\n");

		for (i = 0; i < ARRAY_SIZE(edid->detailed_timings); i++) {
			const struct edid_monitor_descriptor* const mon =
				&edid->detailed_timings[i].monitor;

			if (!edid_detailed_timing_is_monitor_descriptor(edid, i))
				continue;

			if (mon->tag == EDID_MONITOR_DESCRIPTOR_ASCII_STRING) {
				strncpy_s(string, (char*)mon->data, sizeof(string) - 1);
				*strchr(string, '\n') = '\0';

				printf("  ASCII string............. %s\n", string);
			}
		}

		printf("\n");
	}

	//printf("Color characteristics\n");

	//printf("  Default color space...... %ssRGB\n",
	//	edid->feature_support.standard_default_color_space ? "" : "Non-");

	//printf("  Display gamma............ %.2f\n",
	//	edid_gamma(edid));

	//printf("  Red chromaticity......... Rx %0.3f - Ry %0.3f\n",
		//edid_decode_fixed_point(characteristics.red.x),
	//	edid_decode_fixed_point(characteristics.red.y));

	//printf("  Green chromaticity....... Gx %0.3f - Gy %0.3f\n",
	//	edid_decode_fixed_point(characteristics.green.x),
	//	edid_decode_fixed_point(characteristics.green.y));

	//printf("  Blue chromaticity........ Bx %0.3f - By %0.3f\n",
	//	edid_decode_fixed_point(characteristics.blue.x),
	//	edid_decode_fixed_point(characteristics.blue.y));

	//printf("  White point (default).... Wx %0.3f - Wy %0.3f\n",
	//	edid_decode_fixed_point(characteristics.white.x),
	//	edid_decode_fixed_point(characteristics.white.y));

	printf("\n");

	printf("Timing characteristics\n");

	if (monitor_range_limits) {
		printf("  Horizontal scan range.... %u - %u kHz\n",
			monitor_range_limits->minimum_horizontal_rate,
			monitor_range_limits->maximum_horizontal_rate);

		printf("  Vertical scan range...... %u - %u Hz\n",
			monitor_range_limits->minimum_vertical_rate,
			monitor_range_limits->maximum_vertical_rate);

		printf("  Video bandwidth.......... %u MHz\n",
			monitor_range_limits->maximum_supported_pixel_clock * 10);
	}

	printf("  GTF standard............. %sSupported\n",
		edid->feature_support.default_gtf ? "" : "Not ");

	printf("  Preferred timing......... %s\n",
		edid->feature_support.preferred_timing_mode ? "Yes" : "No");

	if (edid->feature_support.preferred_timing_mode) {
		std::string timingstr = _edid_timing_string(&edid->detailed_timings[0].timing);
		printf("  Native/preferred timing.. %s\n", timingstr.data());

		timingstr = _edid_mode_string(&edid->detailed_timings[0].timing);
		printf("    Modeline............... %s\n", timingstr.data());
	}
	else {
		printf("  Native/preferred timing.. n/a\n");
	}

	printf("\n");

	printf("Standard timings supported\n");
	if (edid->established_timings.timing_720x400_70)
		printf("   720 x  400p @ 70Hz - IBM VGA\n");
	if (edid->established_timings.timing_720x400_88)
		printf("   720 x  400p @ 88Hz - IBM XGA2\n");
	if (edid->established_timings.timing_640x480_60)
		printf("   640 x  480p @ 60Hz - IBM VGA\n");
	if (edid->established_timings.timing_640x480_67)
		printf("   640 x  480p @ 67Hz - Apple Mac II\n");
	if (edid->established_timings.timing_640x480_72)
		printf("   640 x  480p @ 72Hz - VESA\n");
	if (edid->established_timings.timing_640x480_75)
		printf("   640 x  480p @ 75Hz - VESA\n");
	if (edid->established_timings.timing_800x600_56)
		printf("   800 x  600p @ 56Hz - VESA\n");
	if (edid->established_timings.timing_800x600_60)
		printf("   800 x  600p @ 60Hz - VESA\n");

	if (edid->established_timings.timing_800x600_72)
		printf("   800 x  600p @ 72Hz - VESA\n");
	if (edid->established_timings.timing_800x600_75)
		printf("   800 x  600p @ 75Hz - VESA\n");
	if (edid->established_timings.timing_832x624_75)
		printf("   832 x  624p @ 75Hz - Apple Mac II\n");
	if (edid->established_timings.timing_1024x768_87)
		printf("  1024 x  768i @ 87Hz - VESA\n");
	if (edid->established_timings.timing_1024x768_60)
		printf("  1024 x  768p @ 60Hz - VESA\n");
	if (edid->established_timings.timing_1024x768_70)
		printf("  1024 x  768p @ 70Hz - VESA\n");
	if (edid->established_timings.timing_1024x768_75)
		printf("  1024 x  768p @ 75Hz - VESA\n");
	if (edid->established_timings.timing_1280x1024_75)
		printf("  1280 x 1024p @ 75Hz - VESA\n");

	for (i = 0; i < ARRAY_SIZE(edid->standard_timing_id); i++) {
		const struct edid_standard_timing_descriptor* const desc =
			&edid->standard_timing_id[i];

		if (!memcmp(desc, EDID_STANDARD_TIMING_DESCRIPTOR_INVALID, sizeof(*desc)))
			continue;

		printf("  %4u x %4u%c @ %uHz - VESA STD\n",
			edid_standard_timing_horizontal_active(desc),
			edid_standard_timing_vertical_active(desc),
			'p',
			edid_standard_timing_refresh_rate(desc));
	}



	printf("\n");
}

static inline void
disp_cea861_video_data(const struct cea861_video_data_block* const vdb)
{
	printf("CE video identifiers (VICs) - timing/formats supported\n");
	for (uint8_t i = 0; i < vdb->header.length; i++) {
		const struct cea861_timing* const timing =
			&cea861_timings[vdb->svd[i].video_identification_code];

		printf(" %s CEA Mode %02u: %4u x %4u%c @ %.fHz\n",
			vdb->svd[i].native ? "*" : " ",
			vdb->svd[i].video_identification_code,
			timing->hactive, timing->vactive,
			(timing->mode == INTERLACED) ? 'i' : 'p',
			timing->vfreq);
	}

	printf("\n");
}

static inline void
disp_cea861_audio_data(const struct cea861_audio_data_block* const adb)
{
	const uint8_t descriptors = adb->header.length / sizeof(*adb->sad);

	printf("CE audio data (formats supported)\n");
	for (uint8_t i = 0; i < descriptors; i++) {
		const struct cea861_short_audio_descriptor* const sad =
			(struct cea861_short_audio_descriptor*) & adb->sad[i];

		switch (sad->audio_format) {
		case CEA861_AUDIO_FORMAT_LPCM:
			printf("  LPCM    %u-channel, %s%s%s\b%s",
				sad->channels + 1,
				sad->flags.lpcm.bitrate_16_bit ? "16/" : "",
				sad->flags.lpcm.bitrate_20_bit ? "20/" : "",
				sad->flags.lpcm.bitrate_24_bit ? "24/" : "",

				((sad->flags.lpcm.bitrate_16_bit +
					sad->flags.lpcm.bitrate_20_bit +
					sad->flags.lpcm.bitrate_24_bit) > 1) ? " bit depths" : "-bit");
			break;
		case CEA861_AUDIO_FORMAT_AC_3:
			printf("  AC-3    %u-channel, %4uk max. bit rate",
				sad->channels + 1,
				(sad->flags.maximum_bit_rate << 3));
			break;
		default:
			fprintf(stderr, "unknown audio format 0x%02x\n",
				sad->audio_format);
			continue;
		}

		printf(" at %s%s%s%s%s%s%s\b kHz\n",
			sad->sample_rate_32_kHz ? "32/" : "",
			sad->sample_rate_44_1_kHz ? "44.1/" : "",
			sad->sample_rate_48_kHz ? "48/" : "",
			sad->sample_rate_88_2_kHz ? "88.2/" : "",
			sad->sample_rate_96_kHz ? "96/" : "",
			sad->sample_rate_176_4_kHz ? "176.4/" : "",
			sad->sample_rate_192_kHz ? "192/" : "");
	}

	printf("\n");
}

static inline void
disp_cea861_vendor_data(const struct cea861_vendor_specific_data_block* vsdb)
{
	const uint8_t oui[] = { vsdb->ieee_registration[2],
							vsdb->ieee_registration[1],
							vsdb->ieee_registration[0] };

	printf("CEA vendor specific data (VSDB)\n");
	printf("  IEEE registration number. 0x");
	for (uint8_t i = 0; i < ARRAY_SIZE(oui); i++)
		printf("%02X", oui[i]);
	printf("\n");

	if (!memcmp(oui, HDMI_OUI, sizeof(oui))) {
		const struct hdmi_vendor_specific_data_block* const hdmi =
			(struct hdmi_vendor_specific_data_block*) vsdb;

		printf("  CEC physical address..... %u.%u.%u.%u\n",
			hdmi->port_configuration_a,
			hdmi->port_configuration_b,
			hdmi->port_configuration_c,
			hdmi->port_configuration_d);

		if (hdmi->header.length >= HDMI_VSDB_EXTENSION_FLAGS_OFFSET) {
			printf("  Supports AI (ACP, ISRC).. %s\n",
				hdmi->audio_info_frame ? "Yes" : "No");
			printf("  Supports 48bpp........... %s\n",
				hdmi->colour_depth_48_bit ? "Yes" : "No");
			printf("  Supports 36bpp........... %s\n",
				hdmi->colour_depth_36_bit ? "Yes" : "No");
			printf("  Supports 30bpp........... %s\n",
				hdmi->colour_depth_30_bit ? "Yes" : "No");
			printf("  Supports YCbCr 4:4:4..... %s\n",
				hdmi->yuv_444_supported ? "Yes" : "No");
			printf("  Supports dual-link DVI... %s\n",
				hdmi->dvi_dual_link ? "Yes" : "No");
		}

		if (hdmi->header.length >= HDMI_VSDB_MAX_TMDS_OFFSET) {
			if (hdmi->max_tmds_clock)
				printf("  Maximum TMDS clock....... %uMHz\n",
					hdmi->max_tmds_clock * 5);
			else
				printf("  Maximum TMDS clock....... n/a\n");
		}

		if (hdmi->header.length >= HDMI_VSDB_LATENCY_FIELDS_OFFSET) {
			//if (hdmi->latency_fields) {
			//	printf("  Video latency %s........ %ums\n",
			//		hdmi->interlaced_latency_fields ? "(p)" : "...",
			//		(hdmi->video_latency - 1) << 1);
			//	printf("  Audio latency %s........ %ums\n",
			//		hdmi->interlaced_latency_fields ? "(p)" : "...",
			//		(hdmi->audio_latency - 1) << 1);
			//}

			//if (hdmi->interlaced_latency_fields) {
			//	printf("  Video latency (i)........ %ums\n",
			//		hdmi->interlaced_video_latency);
			//	printf("  Audio latency (i)........ %ums\n",
			//		hdmi->interlaced_audio_latency);
			//}

			if (hdmi->hdmi_video_present) {
				printf("  Supports 3D ............. %s\n",
					hdmi->content_definition.hdmi_present.present_3d ? "Yes" : "No");

				printf("  3D multi present ........ %u \n",
					hdmi->content_definition.hdmi_present.multi_present_3d);
				printf("  Hdmi vic length ......... %u \n",
					hdmi->content_definition.hdmi_present.hdmi_vic_len);
				printf("  Hdmi 3D length .......... %u \n",
					hdmi->content_definition.hdmi_present.hdmi_3d_len);
				printf("  Image size .............. %u \n",
					hdmi->content_definition.hdmi_present.image_size);

				printf("Hdmi vic list \n");

				const std::string format[5] = {
					"",
					"4K * 2K 29.97 30Hz",
					"4K * 2K 25Hz",
					"4K * 2K 23.98 24Hz",
					"4K * 2K 24Hz SMPTE"
				};

				for (uint8_t i = 0; i < hdmi->content_definition.hdmi_present.hdmi_vic_len; i++) {
					uint8_t index = hdmi->content_definition.hdmi_present.reserved[i];
					printf("  %s \n", format[index].data());
				}

			}
		}

		
	}

	printf("\n");
}

static inline void
disp_cea861_speaker_allocation_data(const struct cea861_speaker_allocation_data_block* const sadb)
{
	const struct cea861_speaker_allocation* const sa = &sadb->payload;
	const uint8_t* const channel_configuration = (uint8_t*)sa;

	printf("CEA speaker allocation data\n");
	printf("  Channel configuration.... %u.%u\n",
		((channel_configuration[0] & 0xe9) << 1) +
		((channel_configuration[0] & 0x14) << 0) +
		((channel_configuration[1] & 0x01) << 1) +
		((channel_configuration[1] & 0x06) << 0),
		(channel_configuration[0] & 0x02));
	printf("  Front left/right......... %s\n",
		sa->front_left_right ? "Yes" : "No");
	printf("  Front LFE................ %s\n",
		sa->front_lfe ? "Yes" : "No");
	printf("  Front center............. %s\n",
		sa->front_center ? "Yes" : "No");
	printf("  Rear left/right.......... %s\n",
		sa->rear_left_right ? "Yes" : "No");
	printf("  Rear center.............. %s\n",
		sa->rear_center ? "Yes" : "No");
	printf("  Front left/right center.. %s\n",
		sa->front_left_right_center ? "Yes" : "No");
	printf("  Rear left/right center... %s\n",
		sa->rear_left_right_center ? "Yes" : "No");
	printf("  Front left/right wide.... %s\n",
		sa->front_left_right_wide ? "Yes" : "No");
	printf("  Front left/right high.... %s\n",
		sa->front_left_right_high ? "Yes" : "No");
	printf("  Top center............... %s\n",
		sa->top_center ? "Yes" : "No");
	printf("  Front center high........ %s\n",
		sa->front_center_high ? "Yes" : "No");

	printf("\n");
}

static void
disp_cea861(const struct edid_extension* const ext)
{
	const struct edid_detailed_timing_descriptor* dtd = NULL;
	const struct cea861_timing_block* const ctb =
		(struct cea861_timing_block*) ext;
	const uint8_t offset = offsetof(struct cea861_timing_block, data);
	uint8_t index = 0, i;

	/*! \todo handle invalid revision */

	printf("CEA-861 Information\n");
	printf("  Revision number.......... %u\n",
		ctb->revision);

	if (ctb->revision >= 2) {
		printf("  IT underscan............. %supported\n",
			ctb->underscan_supported ? "S" : "Not s");
		printf("  Basic audio.............. %supported\n",
			ctb->basic_audio_supported ? "S" : "Not s");
		printf("  YCbCr 4:4:4.............. %supported\n",
			ctb->yuv_444_supported ? "S" : "Not s");
		printf("  YCbCr 4:2:2.............. %supported\n",
			ctb->yuv_422_supported ? "S" : "Not s");
		printf("  Native formats........... %u\n",
			ctb->native_dtds);
	}

	dtd = (struct edid_detailed_timing_descriptor*) ((uint8_t*)ctb + ctb->dtd_offset);
	for (i = 0; dtd->pixel_clock; i++, dtd++) {
		/*! \todo ensure that we are not overstepping bounds */

		std::string szdetail = _edid_timing_string(dtd);
		printf("  Detailed timing #%u....... %s\n", i + 1, szdetail.data());

		szdetail = _edid_mode_string(dtd);
		printf("    Modeline............... %s\n", szdetail.data());
	}

	printf("\n");

	if (ctb->revision >= 3) {
		do {
			const struct cea861_data_block_header* const header =
				(struct cea861_data_block_header*) & ctb->data[index];

			switch (header->tag) {
			case CEA861_DATA_BLOCK_TYPE_AUDIO:
			{
				OutputLog("Audio Length: %d \n", header->length);
				const struct cea861_audio_data_block* const db =
					(struct cea861_audio_data_block*) header;

				disp_cea861_audio_data(db);
			}
			break;
			case CEA861_DATA_BLOCK_TYPE_VIDEO:
			{
				OutputLog("Video Length: %d \n", header->length);
				const struct cea861_video_data_block* const db =
					(struct cea861_video_data_block*) header;

				disp_cea861_video_data(db);
			}
			break;
			case CEA861_DATA_BLOCK_TYPE_VENDOR_SPECIFIC:
			{
				OutputLog("VSDB Length: %d \n", header->length);
				const struct cea861_vendor_specific_data_block* const db =
					(struct cea861_vendor_specific_data_block*) header;

				disp_cea861_vendor_data(db);
			}
			break;
			case CEA861_DATA_BLOCK_TYPE_SPEAKER_ALLOCATION:
			{
				OutputLog("Speaker Length: %d \n", header->length);
				const struct cea861_speaker_allocation_data_block* const db =
					(struct cea861_speaker_allocation_data_block*) header;

				disp_cea861_speaker_allocation_data(db);
			}
			break;
			default:
				fprintf(stderr, "unknown CEA-861 data block type 0x%02x\n",
					header->tag);
				break;
			}

			index = index + header->length + sizeof(*header);
		} while (index < ctb->dtd_offset - offset);
	}

	printf("\n");
}


static void
dump_cea861(const uint8_t* const buffer)
{
	printf("cea info: \n");

	const struct edid_detailed_timing_descriptor* dtd = NULL;
	const struct cea861_timing_block* const ctb =
		(struct cea861_timing_block*) buffer;
	const uint8_t dof = offsetof(struct cea861_timing_block, data);

	dump_section("cea extension header", buffer, 0x00, 0x04);

	if (ctb->dtd_offset - dof)
		dump_section("data block collection", buffer, 0x04, ctb->dtd_offset - dof);

	dtd = (struct edid_detailed_timing_descriptor*) (buffer + ctb->dtd_offset);
	for (uint8_t i = 0; dtd->pixel_clock; i++, dtd++) {
		char header[1024] = {0};
		sprintf_s(header, sizeof(header) -1, "detailed timing descriptor %03u", i);
		dump_section(header, (uint8_t*)dtd, 0x00, sizeof(*dtd));
	}

	dump_section("padding", buffer, (uint8_t*)dtd - buffer,
		dof + sizeof(ctb->data) - ((uint8_t*)dtd - buffer));
	dump_section("checksum", buffer, 0x7f, 0x01);

	printf("\n");
}

static const struct edid_extension_handler {
	void (* const hex_dump)(const uint8_t* const);
	void (* const inf_disp)(const struct edid_extension* const);
} edid_extension_handlers[] = {
	{},
	{},
	{ dump_cea861, disp_cea861 },
};

void read(const std::string& file)
{
	std::fstream fs;
	fs.open(file.data(), std::fstream::in);
	assert(fs.is_open());

	uint8_t c;
	//fs >> std::noskipws;	// Do not skip whitespaces
	while (!fs.eof()) {
		fs >> c;
		//std::cout << std::hex << c << std::endl;
		OutputLog("%x \n", c);
	}

	fs.close();
}

CEdid::CEdid(const char* const filename)
{
	std::ifstream edid(filename, std::ifstream::binary);
	assert(edid.is_open());

	edid.seekg(0, std::ios::end);
	size_t size = (size_t)edid.tellg();
	edid.seekg(0, std::ios::beg);

	m_buf = new uint8_t[size];
	edid.read((char*)m_buf, size);
}

CEdid::~CEdid()
{
	delete[] m_buf;
}

void CEdid::parse_edid()
{
	const struct edid* const edid = (struct edid*)m_buf;
	const struct edid_extension * const extensions = (struct edid_extension*) (m_buf + sizeof(*edid));

	if (memcmp(edid->header, EDID_HEADER, sizeof(edid->header))) return;

	dump_edid((uint8_t*) edid);
	disp_edid(edid);

	for (uint8_t i = 0; i < edid->extensions; i++) {
		const struct edid_extension* const extension = &extensions[i];
		const struct edid_extension_handler* const handler =
			&edid_extension_handlers[extension->tag];

		if (!handler) {
			fprintf(stderr,
				"WARNING: block %u contains unknown extension (%#04x)\n",
				i, extensions[i].tag);
			continue;
		}

		if (handler->hex_dump)
			(*handler->hex_dump)((uint8_t*)extension);

		if (handler->inf_disp)
			(*handler->inf_disp)(extension);
	}
}
