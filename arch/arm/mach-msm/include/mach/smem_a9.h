#ifndef __SMEM_A9_H
#define __SMEM_A9_H

#define DEVICE_IS_FUSED			1
#define CPU_TYPE_LEN				7
#define DEVICE_CPU_TYPE_8260A_1 		0x007910E1
#define DEVICE_CPU_TYPE_8260A_3 		0x007950E1

typedef enum {
	ACER_AMSS_BOOT_IN_NORMAL,
	ACER_AMSS_BOOT_IN_AMSS_FTM,
	ACER_AMSS_BOOT_IN_OS_FTM,
	ACER_AMSS_BOOT_IN_CHARGING_ONLY,
	ACER_AMSS_BOOT_IN_AMSS_DOWNLOAD,
	ACER_AMSS_BOOT_IN_OS_USB_FTM,
	ACER_AMSS_BOOT_IN_OS_DOWNLOAD,
	ACER_AMSS_BOOT_IN_SDDL_AMSS,
	ACER_AMSS_BOOT_IN_SDDL_OS,
	ACER_AMSS_BOOT_IN_SDDL_DIFF,
	ACER_AMSS_BOOT_IN_AMSS_CRASH_REBOOT,
	ACER_AMSS_BOOT_IN_OS_CRASH_REBOOT,
	ACER_AMSS_BOOT_IN_FTM,
	ACER_AMSS_BOOT_IN_AMSS_DOWNLOAD_DISPLAY,
	ACER_AMSS_BOOT_IN_SDDL_AMSS_DISPLAY,
	ACER_AMSS_BOOT_IN_POWER_CUT,
	ACER_AMSS_BOOT_IN_SW_RESET,
	ACER_AMSS_BOOT_IN_HW_RESET,
	ACER_AMSS_BOOT_IN_EFS_FORMAT,
	ACER_AMSS_BOOT_IN_ARM9_FATAL_ERROR,
	ACER_AMSS_BOOT_IN_WDOG,
	ACER_AMSS_BOOT_IN_BATT_REMOVE,
	ACER_AMSS_BOOT_IN_PMIC_RTC,
	ACER_AMSS_BOOT_IN_LOW_BATT,
	ACER_AMSS_BOOT_IN_AMSS_DOWNLOAD_ALL,
	ACER_AMSS_BOOT_IN_FORMAT_EFS,
	ACER_AMSS_BOOT_IN_UNKOWN,
	ACER_AMSS_BOOT_IN_MAX = 30,
	ACER_AMSS_BOOT_INVALID = 0x7FFFFFFF,
} acer_amss_boot_mode_type;

typedef enum {
	ACER_UART_LOG_ON,
	ACER_UART_LOG_OFF,
	ACER_UART_LOG_ON_MODEM_DO_INIT_AGAIN,
	ACER_UART_LOG_INVALID = 0x7FFFFFFF,
} acer_uart_log_switch_type;

typedef enum {
	ACER_OS_NORMAL_BOOT_MODE,
	ACER_OS_CLEAN_BOOT_MODE,
	ACER_OS_BOOT_MODE_INVALID = 0x7FFFFFFF,
} acer_os_boot_mode_type;

typedef struct {
	uint32_t magic_num;
	uint32_t apps_boot_reason;
} acer_bootmode_id_type;

typedef enum {
	ACER_HW_VERSION_EVB,
	ACER_HW_VERSION_EVT1,
	ACER_HW_VERSION_EVT2,
	ACER_HW_VERSION_DVT1,
	ACER_HW_VERSION_DVT1_1,
	ACER_HW_VERSION_DVT2,
	ACER_HW_VERSION_DVT2_1,
	ACER_HW_VERSION_DVT3,
	ACER_HW_VERSION_PVT,
	ACER_HW_VERSION_UNKNOW,
	ACER_HW_VERSION_INVALID = 0x7FFFFFFF,
} acer_hw_version_type;

typedef enum {
	RF_CONFIG1,				// LTE w/ NFC
	RF_CONFIG2,				// 3G w/ NFC
	RF_CONFIG3,				// LTE w/o NFC
	RF_CONFIG4,				// 3G w/o NFC
	RF_CONFIG5,
	RF_CONFIG6,
	RF_CONFIG7,
	RF_CONFIG8,
	RF_CONFIG_INVAILD = 0x7FFFFFFF,
} acer_rf_hw_version_type;

typedef enum {
	ACER_OS_NORMAL_MODE,
	ACER_OS_IDLE_MODE,
	ACER_OS_SUSPEND_MODE,
	ACER_OS_INVALID_MODE = 0x7FFFFFFF,
} acer_os_pwr_state_type;

typedef struct {
	acer_amss_boot_mode_type		acer_amss_boot_mode;
	acer_uart_log_switch_type		acer_uart_log_switch;
	acer_os_boot_mode_type			acer_os_boot_mode;
	acer_bootmode_id_type			acer_bootmode_info;
	unsigned char				acer_amss_sw_version[64];
	acer_hw_version_type			acer_hw_version;
	acer_rf_hw_version_type			acer_rf_hw_version;
	unsigned char				acer_factory_sn[32];
	acer_os_pwr_state_type			acer_os_pwr_state;
	unsigned char				acer_os_sw_version[64];
	unsigned char				acer_device_imei[9];
	unsigned char				acer_device_bt_mac[6];
	uint32_t				acer_ddr_vendor;
	uint32_t				acer_emmc_vendor;
	uint32_t				acer_secboot_fuses_is_verified;
	uint32_t				msm_hw_id;
	uint32_t				modem_err_counter;
	uint32_t				reserved[15];
} acer_smem_info_type;
#endif
