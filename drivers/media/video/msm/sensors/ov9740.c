#include "msm_sensor.h"
#include <mach/gpio.h>
#include "ov9740.h"
#define SENSOR_NAME "ov9740"
#define PLATFORM_DRIVER_NAME "msm_camera_ov9740"

DEFINE_MUTEX(ov9740_mut);
static struct msm_sensor_ctrl_t ov9740_s_ctrl;

static struct v4l2_subdev_info ov9740_subdev_info[] = {
	{
	.code = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt = 1,
	.order = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array ov9740_init_conf[] = {
	{
	&ov9740_init_settings[0],
	ARRAY_SIZE(ov9740_init_settings),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	}
};

static struct msm_camera_i2c_conf_array ov9740_mode[] = {
	{}
};

/* Brightness */
//Brightness: -3 ~ +3
static struct msm_camera_i2c_conf_array ov9740_brightness_conf[] = {
	{
	&ov9740_brightness_minus3[0],
	ARRAY_SIZE(ov9740_brightness_minus3),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_brightness_minus2[0],
	ARRAY_SIZE(ov9740_brightness_minus2),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_brightness_minus1[0],
	ARRAY_SIZE(ov9740_brightness_minus1),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{// default case
	&ov9740_brightness_default[0],
	ARRAY_SIZE(ov9740_brightness_default),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_brightness_plus1[0],
	ARRAY_SIZE(ov9740_brightness_plus1),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_brightness_plus2[0],
	ARRAY_SIZE(ov9740_brightness_plus2),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_brightness_plus3[0],
	ARRAY_SIZE(ov9740_brightness_plus3),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	}
};

/* Contrast */
//Contrast: -4 ~ +4
static struct msm_camera_i2c_conf_array ov9740_contrast_conf[] = {
	{
	&ov9740_contrast_minus4[0],
	ARRAY_SIZE(ov9740_contrast_minus4),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_contrast_minus3[0],
	ARRAY_SIZE(ov9740_contrast_minus3),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_contrast_minus2[0],
	ARRAY_SIZE(ov9740_contrast_minus2),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_contrast_minus1[0],
	ARRAY_SIZE(ov9740_contrast_minus1),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{// default case
	&ov9740_contrast_default[0],
	ARRAY_SIZE(ov9740_contrast_default),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_contrast_plus1[0],
	ARRAY_SIZE(ov9740_contrast_plus1),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_contrast_plus2[0],
	ARRAY_SIZE(ov9740_contrast_plus2),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_contrast_plus3[0],
	ARRAY_SIZE(ov9740_contrast_plus3),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_contrast_plus4[0],
	ARRAY_SIZE(ov9740_contrast_plus4),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	}
};
/* Saturation */
//Saturation: -3 ~ +3
static struct msm_camera_i2c_conf_array ov9740_saturation_conf[] = {
	{
	&ov9740_saturation_minus3[0],
	ARRAY_SIZE(ov9740_saturation_minus3),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_saturation_minus2[0],
	ARRAY_SIZE(ov9740_saturation_minus2),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_saturation_minus1[0],
	ARRAY_SIZE(ov9740_saturation_minus1),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{// default case
	&ov9740_saturation_default[0],
	ARRAY_SIZE(ov9740_saturation_default),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_saturation_plus1[0],
	ARRAY_SIZE(ov9740_saturation_plus1),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_saturation_plus2[0],
	ARRAY_SIZE(ov9740_saturation_plus2),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_saturation_plus3[0],
	ARRAY_SIZE(ov9740_saturation_plus3),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	}
};
/* Sharpness */
//Sharpness: -3 ~ +3
static struct msm_camera_i2c_conf_array ov9740_sharpness_conf[] = {
	{
	&ov9740_sharpness_minus3[0],
	ARRAY_SIZE(ov9740_sharpness_minus3),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_sharpness_minus2[0],
	ARRAY_SIZE(ov9740_sharpness_minus2),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_sharpness_minus1[0],
	ARRAY_SIZE(ov9740_sharpness_minus1),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{// default case
	&ov9740_sharpness_default[0],
	ARRAY_SIZE(ov9740_sharpness_default),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_sharpness_plus1[0],
	ARRAY_SIZE(ov9740_sharpness_plus1),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_sharpness_plus2[0],
	ARRAY_SIZE(ov9740_sharpness_plus2),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	},
	{
	&ov9740_sharpness_plus3[0],
	ARRAY_SIZE(ov9740_sharpness_plus3),
	0,
	MSM_CAMERA_I2C_BYTE_DATA
	}
};

static struct msm_sensor_output_info_t ov9740_dimensions[] = {
	{
	.x_output = 0x0500, //1280
	.y_output = 0x02d0, // 720
	.line_length_pclk = 0x0662,
	.frame_length_lines = 0x0307,
	.vt_pixel_clk = 72000000,
	.op_pixel_clk = 72000000,
	.binning_factor = 1,
	},
	{
	.x_output = 0x0500, // 640
	.y_output = 0x02d0, //480 0x168, // 360
	.line_length_pclk = 0x0662,
	.frame_length_lines = 0x0307,
	.vt_pixel_clk = 72000000,
	.op_pixel_clk = 72000000,
	.binning_factor = 1,
	},
};

static struct msm_camera_csid_vc_cfg ov9740_cid_cfg[] = {
	{0, 0x1E, CSI_DECODE_8BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ov9740_csi_params = {
	.csid_params = {
	        .lane_cnt = 1,
	        .lut_params = {
	                .num_cid = 2,
	                .vc_cfg = ov9740_cid_cfg,
	        },
	},
	.csiphy_params = {
	        .lane_cnt = 1,
	        .settle_cnt = 7,
	},
};

static struct msm_camera_csi2_params *ov9740_csi_params_array[] = {
	&ov9740_csi_params,
	&ov9740_csi_params,
};

static struct msm_sensor_output_reg_addr_t ov9740_reg_addr = {
	.x_output = 0x034c,
	.y_output = 0x034e,
	.line_length_pclk = 0x0342,
	.frame_length_lines = 0x0340,
};

static struct msm_sensor_id_info_t ov9740_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x9740,
};

static struct msm_sensor_exp_gain_info_t ov9740_exp_gain_info = {
	.coarse_int_time_addr = 0x3501,
	.global_gain_addr = 0x3508,
	.vert_offset = 6,
};

static int32_t ov9740_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	uint32_t fl_lines, offset;
	uint8_t int_time[3];
	fl_lines =
		(s_ctrl->curr_frame_length_lines * s_ctrl->fps_divider) / Q10;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line > (fl_lines - offset))
		fl_lines = line + offset;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
		MSM_CAMERA_I2C_WORD_DATA);
	int_time[0] = line >> 12;
	int_time[1] = line >> 4;
	int_time[2] = line << 4;
	msm_camera_i2c_write_seq(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr-1,
		&int_time[0], 3);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

//Brightness: -3 ~ +3
static int32_t ov9740_set_brightness(struct msm_sensor_ctrl_t *s_ctrl, uint32_t value)
{
	int32_t rc = 0;
	CDBG("ov9740_set_brightness: value= %d\n", value);
	//default  when brightness=0 :value = 3
	if(value >= 0 && value <= 6) {
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		rc = msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			&ov9740_brightness_conf[0],
			value);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	} else {
		pr_err("ov9740_set_brightness: value= %d not support\n", value);
	}
	return rc;
}

//Contrast: -4 ~ +4
static int32_t ov9740_set_contrast(struct msm_sensor_ctrl_t *s_ctrl, uint32_t value)
{
	int32_t rc = 0;
	CDBG("ov9740_set_contrast: value= %d\n", value);
	//default  when contrast=0 :value = 4
	if(value >= 0 && value <= 8) {
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		rc = msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			&ov9740_contrast_conf[0],
			value);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	} else {
		pr_err("ov9740_set_contrast: value= %d not support\n", value);
	}
	return rc;
}

//Saturation: -3 ~ +5
static int32_t ov9740_set_saturation(struct msm_sensor_ctrl_t *s_ctrl, uint32_t value)
{
	int32_t rc = 0;
	CDBG("ov9740_set_saturation: value= %d\n", value);
	//default  when saturation=0 :value = 3
	if(value >= 0 && value <= 8) {
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		rc = msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			&ov9740_saturation_conf[0],
			value);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	} else {
		pr_err("ov9740_set_saturation: value= %d not support\n", value);
	}
	return rc;
}

//Sharpness: -3 ~ +4
static int32_t ov9740_set_sharpness(struct msm_sensor_ctrl_t *s_ctrl, uint32_t value)
{
	int32_t rc = 0;
	CDBG("ov9740_set_sharpness: value= %d\n", value);
	//default  when sharpness=0 :value = 3
	if(value >= 0 && value <= 7) {
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		rc = msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			&ov9740_sharpness_conf[0],
			value);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	} else {
		pr_err("ov9740_set_sharpness: value= %d not support\n", value);
	}
	return rc;
}

static int32_t ov9740_get_line_count(struct msm_sensor_ctrl_t *s_ctrl, uint32_t *value)
{
	int32_t rc = 0;
	uint16_t reg_0202, reg_0203;

	// exposure_lines = 0x0202 bit[7:0], 0x0203 bit[7:0]
	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
				0x0202, &reg_0202,
				MSM_CAMERA_I2C_BYTE_DATA);
	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
				0x0203, &reg_0203,
				MSM_CAMERA_I2C_BYTE_DATA);

	*value = (uint32_t)reg_0202<<8 | (uint32_t)reg_0203;

	CDBG("%s: exposure_lines = %u\n", __func__, *value);

	return rc;
}

static const struct i2c_device_id ov9740_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov9740_s_ctrl},
	{ }
};

static struct i2c_driver ov9740_i2c_driver = {
	.id_table = ov9740_i2c_id,
	.probe = msm_sensor_i2c_probe,
	.driver = {
	        .name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov9740_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init ov9740_sensor_module_init(void)
{
	int rc = 0;

	rc = i2c_add_driver(&ov9740_i2c_driver);
	return rc;
}

static struct v4l2_subdev_core_ops ov9740_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov9740_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov9740_subdev_ops = {
	.core = &ov9740_subdev_core_ops,
	.video = &ov9740_subdev_video_ops,
};

static struct msm_sensor_fn_t ov9740_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = ov9740_write_exp_gain,
	.sensor_write_snapshot_exp_gain = ov9740_write_exp_gain,
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	.sensor_set_brightness = ov9740_set_brightness,
	.sensor_set_contrast = ov9740_set_contrast,
	.sensor_set_saturation = ov9740_set_saturation,
	.sensor_set_sharpness = ov9740_set_sharpness,
	.sensor_get_line_count = ov9740_get_line_count,
};

static struct msm_sensor_reg_t ov9740_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov9740_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov9740_start_settings),
	.stop_stream_conf = ov9740_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov9740_stop_settings),
	.group_hold_on_conf = ov9740_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ov9740_groupon_settings),
	.group_hold_off_conf = ov9740_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(ov9740_groupoff_settings),
	.init_settings = &ov9740_init_conf[0],
	.init_size = ARRAY_SIZE(ov9740_init_conf),
	.mode_settings = &ov9740_mode[0],
	.output_settings = &ov9740_dimensions[0],
	.num_conf = 2,
};

static ssize_t sensor_id_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "OV9740\n");
}

static struct kobj_attribute sensor_id_attribute =
	__ATTR_RO(sensor_id);

static ssize_t vendor_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "OmniVision\n");
}

static struct kobj_attribute vendor_attribute =
	__ATTR_RO(vendor);

static struct attribute *ov9740_attrs[] = {
	&sensor_id_attribute.attr,
	&vendor_attribute.attr,
	NULL,  // need to NULL terminate the list of attributes
};

static struct attribute_group ov9740_attr_group = {
	.attrs = ov9740_attrs,
};

static struct sensor_info_t ov9740_sensor_info = {
	.kobj_name = "dev-info_front-camera",
	.attr_group = &ov9740_attr_group,
};

static struct msm_sensor_ctrl_t ov9740_s_ctrl = {
	.msm_sensor_reg = &ov9740_regs,
	.sensor_i2c_client = &ov9740_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.sensor_output_reg_addr = &ov9740_reg_addr,
	.sensor_id_info = &ov9740_id_info,
	.sensor_exp_gain_info = &ov9740_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &ov9740_csi_params_array[0],
	.msm_sensor_mutex = &ov9740_mut,
	.sensor_i2c_driver = &ov9740_i2c_driver,
	.sensor_v4l2_subdev_info = ov9740_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov9740_subdev_info),
	.sensor_v4l2_subdev_ops = &ov9740_subdev_ops,
	.func_tbl = &ov9740_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
	.sensor_info = &ov9740_sensor_info,
};

module_init(ov9740_sensor_module_init);
MODULE_DESCRIPTION("OV9740 sensor driver");
MODULE_LICENSE("GPL v2");
