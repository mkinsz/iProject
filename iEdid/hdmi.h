#ifndef EDID_HDMI_H
#define EDID_HDMI_H

#include "cea861.h"

/*! \todo figure out a better way to determine the offsets */
#define HDMI_VSDB_EXTENSION_FLAGS_OFFSET        (0x06)
#define HDMI_VSDB_MAX_TMDS_OFFSET               (0x07)
#define HDMI_VSDB_LATENCY_FIELDS_OFFSET         (0x08)

static const uint8_t HDMI_OUI[] = { 0x00, 0x0C, 0x03 };

struct hdmi_vendor_specific_data_block {
	struct cea861_data_block_header header;

	uint8_t  ieee_registration_id[3];           /* LSB */

	uint8_t port_configuration_b : 4;
	uint8_t port_configuration_a : 4;
	uint8_t port_configuration_d : 4;
	uint8_t port_configuration_c : 4;

	/* extension fields */
	uint8_t dvi_dual_link : 1;
	uint8_t : 2;
	uint8_t yuv_444_supported : 1;
	uint8_t colour_depth_30_bit : 1;
	uint8_t colour_depth_36_bit : 1;
	uint8_t colour_depth_48_bit : 1;
	uint8_t audio_info_frame : 1;

	uint8_t  max_tmds_clock;                    /* = value * 5 */

	uint8_t : 5;
	uint8_t hdmi_video_present : 1;
	uint8_t interlaced_latency_fields : 1;
	uint8_t latency_fields : 1;

	union {
		struct {
			uint8_t video_latency;                     /* = (value - 1) * 2 */
			uint8_t audio_latency;                     /* = (value - 1) * 2 */
			uint8_t interlaced_video_latency;
			uint8_t interlaced_audio_latency;

			uint8_t : 3;
			uint8_t image_size : 2;
			uint8_t multi_present_3d : 2;
			uint8_t present_3d : 1;

			uint8_t hdmi_3d_len : 5;
			uint8_t hdmi_vic_len : 3;

			uint8_t reserved[];
		} all_field;

		struct {
			uint8_t video_latency;                     /* = (value - 1) * 2 */
			uint8_t audio_latency;                     /* = (value - 1) * 2 */

			uint8_t : 3;
			uint8_t image_size : 2;
			uint8_t multi_present_3d : 2;
			uint8_t present_3d : 1;

			uint8_t hdmi_3d_len : 5;
			uint8_t hdmi_vic_len : 3;

			uint8_t reserved[];
		} latency_field;

		struct {
			uint8_t interlaced_video_latency;
			uint8_t interlaced_audio_latency;

			uint8_t : 3;
			uint8_t image_size : 2;
			uint8_t multi_present_3d : 2;
			uint8_t present_3d : 1;

			uint8_t hdmi_3d_len : 5;
			uint8_t hdmi_vic_len : 3;

			uint8_t reserved[];
		} ilatency_field;

		struct {
			uint8_t : 3;
			uint8_t image_size : 2;
			uint8_t multi_present_3d : 2;
			uint8_t present_3d : 1;

			uint8_t hdmi_3d_len : 5;
			uint8_t hdmi_vic_len : 3;

			uint8_t reserved[];
		} hdmi_present;
	} content_definition;
};

#endif

