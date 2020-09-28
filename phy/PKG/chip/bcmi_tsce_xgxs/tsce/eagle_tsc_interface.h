/***********************************************************************************
***********************************************************************************
*                                                                                 *
*  Revision    :  $Id: eagle_tsc_interface.h 1075 Broadcom SDK $   *
*                                                                                 *
*  Description :  Interface functions targeted to IP user                         *
*                                                                                 *
* This software is governed by the Broadcom Switch APIs license.
* This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
* 
* Copyright 2007-2020 Broadcom Inc. All rights reserved.                                                           *
*  No portions of this material may be reproduced in any form without             *
*  the written permission of:                                                     *
*      Broadcom Corporation                                                       *
*      5300 California Avenue                                                     *
*      Irvine, CA  92617                                                          *
*                                                                                 *
*  All information contained in this document is Broadcom Corporation             *
*  company private proprietary, and trade secret.                                 *
 */

/*! \mainpage
 *
 * \section intro_sec Introduction
 *
 * This documentation files are intended to guide a developer in using the
 * Broadcom SerDes core within a larger ecosystem by providing specific API
 * function calls which can be used to perform all necessary operations.
 *
 * The detailed description about the API directory structure, API overview,
 * release procedure, and check out process can be found in the Serdes API User Guide
 * (http://ingbu.broadcom.com/hsip/eagle_tsc/SerDes%20Documents/PMD_Methodology/Serdes%20External%20Micro%20User%20Guide.docx)
 * \n \n
 *
 * \section API_sec Important API Files
 *
 * eagle_tsc_interface.h - provides the primary functionality.
 *
 * eagle_tsc_dependencies.h - defines the functions to be provided by the IP user
 *
 * \n \n
 * Copyright 2014 Broadcom Corporation all rights reserved
 */

/** @file eagle_tsc_interface.h
 * Main interface functions provided to IP User
 */

#ifndef EAGLE_TSC_API_INTERFACE_H
#define EAGLE_TSC_API_INTERFACE_H

#include "eagle_tsc_usr_includes.h"

#include "eagle_tsc_ipconfig.h"
#include "eagle_tsc_enum.h"
#include "srds_api_err_code.h"


/*----------------------------------------*/
/*  Lane/Core structs (without bitfields) */
/*----------------------------------------*/

/** Lane Config Variable Structure in Microcode */
struct eagle_tsc_uc_lane_config_field_st {
	uint8_t lane_cfg_from_pcs;
	uint8_t an_enabled;
	uint8_t dfe_on;
	uint8_t force_brdfe_on;
	uint8_t media_type;
	uint8_t unreliable_los;
	uint8_t scrambling_dis;
	uint8_t cl72_auto_polarity_en;
	uint8_t cl72_restart_timeout_en;
	uint8_t reserved;
};

/** Core Config Variable Structure in Microcode */
struct eagle_tsc_uc_core_config_field_st {
	uint8_t core_cfg_from_pcs;
	uint8_t vco_rate;
	uint8_t reserved1;
	uint8_t reserved2;
};

/** Lane Config Struct */
struct  eagle_tsc_uc_lane_config_st {
	struct eagle_tsc_uc_lane_config_field_st field;
	uint16_t word;
};

/** Core Config Struct */
struct  eagle_tsc_uc_core_config_st {
	struct eagle_tsc_uc_core_config_field_st field;
	uint16_t word;
	int vco_rate_in_Mhz; /* if >0 then will get converted and replace field.vco_rate when update is called */
};

/** Eyescan Options Struct */
struct eagle_tsc_eyescan_options_st {
	uint32_t linerate_in_khz;
	uint16_t timeout_in_milliseconds;
	int8_t horz_max;
	int8_t horz_min;
	int8_t hstep;
	int8_t vert_max;
	int8_t vert_min;
	int8_t vstep;
	int8_t mode;
};


/****************************************************/
/*  CORE Based APIs - Required to be used per Core  */
/****************************************************/
/* Returns API Version Number */
/** API Version Number.
 * @param pa phymod_access_t struct
 * @param *api_version API Version Number returned by the API
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_version( const phymod_access_t *pa, uint32_t *api_version);

/*------------------------------------------------------*/
/*  APIs to Read/Write Core Config variables in uC RAM  */
/*------------------------------------------------------*/
/** Write to core_config uC RAM variable.
 * @param pa phymod_access_t struct
 * @param struct_val Value to be written into core_config RAM variable.
 *                   (Note that struct_val.word must be = 0, only the fields are used)
 * @param pa phymod_access_t struct
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_set_uc_core_config( const phymod_access_t *pa, struct eagle_tsc_uc_core_config_st struct_val);

/** Read value of core_config uC RAM variable.
 * @param pa phymod_access_t struct
 * @param *struct_val Value to be written into core_config RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_get_uc_core_config( const phymod_access_t *pa, struct eagle_tsc_uc_core_config_st *struct_val);

/*----------------------------------------*/
/*  Display Core Config and Debug Status  */
/*----------------------------------------*/
/** Display Core configurations (RAM config variables and config register fields).
 * @param pa phymod_access_t struct
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_core_config( const phymod_access_t *pa );

/** Display current Core state. Read and displays core status variables and fields.
 * @param pa phymod_access_t struct
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_core_state( const phymod_access_t *pa );

/** Column definition header for eagle_tsc_display_core_state() API output.
 * To be called before eagle_tsc_display_core_state_line() API.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_core_state_hdr( const phymod_access_t *pa );

/** Display current Core state in single line. Read and displays core status variables and fields.
 * Call eagle_tsc_display_core_state_hdr() API before and eagle_tsc_display_core_state_legend() after
 * calling this API to get a formatted core state display with legend.
 * @param pa phymod_access_t struct
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_core_state_line( const phymod_access_t *pa );

/** Detailed description of each column in eagle_tsc_display_core_state_line() API output.
 * To be called after eagle_tsc_display_core_state_line() API to display the legends.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_core_state_legend( const phymod_access_t *pa );

/*----------------------------------------*/
/*  Temperature forcing and reading       */
/*----------------------------------------*/
/** Forces die temperature in degrees Ceisius (as integer).
 * @param pa phymod_access_t struct
 * @param die_temp  Die temperature in degrees Celsius.
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_force_die_temperature( const phymod_access_t *pa, int16_t die_temp);

/** Read die temperature in degrees Ceisius (as integer)
 * @param pa phymod_access_t struct
 * @param *die_temp  Die temperature in degrees Celsius
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_read_die_temperature( const phymod_access_t *pa, int16_t *die_temp);

/** Read die temperature in degrees Ceisius (as double)
 * @param pa phymod_access_t struct
 * @param *die_temp  Die temperature in degrees Celsius (as double)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_read_die_temperature_double(const phymod_access_t *pa, USR_DOUBLE *die_temp);


/*-----------------------------------------------*/
/*  Envelope functions requested by Switch team  */
/*-----------------------------------------------*/

/** Display current Core and Lane state.
 * @param pa phymod_access_t struct
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_state( const phymod_access_t *pa );

/** Display current Core and Lane config.
 * @param pa phymod_access_t struct
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_config( const phymod_access_t *pa );


/** Set the uC active mode.
 * uC active bit should be set by the user to ensure the ucode loaded into the micro is executed. See Programmers Guide for more details.
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable uC Active (1 = Enable; 0 = Disable)
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_uc_active_enable( const phymod_access_t *pa, uint8_t enable);

/*-----------------------------------*/
/*  Microcode Load/Verify Functions  */
/*-----------------------------------*/

/** Load Microcode into Micro through Register (MDIO) Interface.
 * Once the microcode is loaded, de-assert reset to 8051 to start executing microcode "wrc_micro_mdio_dw8051_reset_n(0x1)".
 * \n Note: Micro should be loaded only after issuing a eagle_tsc_uc_reset(1) followed by asserting and de-asserting
 * core_s_reset. See relevant Programmers guide for more details.
 * @param pa phymod_access_t struct
 * @param *ucode_image pointer to the Microcode image organized in bytes
 * @param ucode_len Length of Microcode Image (number of bytes)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_ucode_mdio_load( const phymod_access_t *pa, uint8_t *ucode_image, uint16_t ucode_len);

/** To verify the Microcode image loaded in the Micro.
 * Read back the microcode from Micro and check against expected microcode image.
 * @param pa phymod_access_t struct
 * @param *ucode_image pointer to the expeted Microcode image organized in bytes
 * @param ucode_len Length of Microcode Image (number of bytes)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_ucode_load_verify( const phymod_access_t *pa, uint8_t *ucode_image, uint16_t ucode_len);

/** To verify the CRC of the microcode loaded in the Micro.
 * Instruct uC to read image and calculate CRC and check against expected CRC.
 * @param pa phymod_access_t struct
 * @param ucode_len Length of Microcode Image (number of bytes)
 * @param expected_crc_value Expected CRC value of the microcode
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_ucode_crc_verify( const phymod_access_t *pa, uint16_t ucode_len, uint16_t expected_crc_value);

/** To instruct the micro to start calculating the CRC of the microcode.
 * Instruct uC to read image and calculate CRC.
 * Control is returned after triggering start of CRC calculation (does NOT wait for completion of CRC calculation). \n \n
 * NOTE: No uC commands should be executed between starting the CRC calculaion [eagle_tsc_ucode_crc_verify()] and verifying the CRC value [eagle_tsc_start_ucode_crc_calc()].
 * @param pa phymod_access_t struct
 * @param ucode_len Length of Microcode Image (number of bytes)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_start_ucode_crc_calc( const phymod_access_t *pa, uint16_t ucode_len);

/** To check the expected CRC against the CRC calulated by the micro.
 * NOTE: No uC commands should be executed between starting the CRC calculation [eagle_tsc_ucode_crc_verify()] and verifying the CRC value [eagle_tsc_start_ucode_crc_calc()].
 * @param pa phymod_access_t struct
 * @param expected_crc_value Expected CRC value of the microcode
 * @param timeout_ms Time interval in milliseconds inside which the previous command (calculate CRC) should be completed
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_check_ucode_crc( const phymod_access_t *pa, uint16_t expected_crc_value, uint32_t timeout_ms);

/** To setup the pram bus for Micro load.
 * When using the pram interface to program the micro, the pram bus must first be setup.
 * @param pa phymod_access_t struct
 * @param ucode_len Length of Microcode Image (number of bytes) the value will be padded to 32bit boundary
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_init_pram_for_uc_load( const phymod_access_t *pa, uint16_t ucode_len);

/** To disable the pram bus after Micro load.
 * After using the pram interface to program the micro, the pram bus must then be diabled.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_finish_pram_load( const phymod_access_t *pa );

/** Enable or Disable the uC reset.
 * Note: Micro should be reset using the API everytime before reloading the microcode
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable uC reset (1 = Enable; 0 = Disable)
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_uc_reset( const phymod_access_t *pa, uint8_t enable);


/*------------------------------*/
/*  Shared TX Pattern Generator */
/*------------------------------*/
/** Configure Shared TX Pattern API.
 * An input string (hex or binary) and pattern length are taken in as inputs, based on which the Pattern Generator registers
 * are programmed to the required values to generate that pattern.
 * Note: eagle_tsc_tx_shared_patt_gen_en() API should be called to enable the Pattern generator for that particular lane.
 * @param pa phymod_access_t struct
 * @param patt_length Pattern length
 * @param pattern Input Pattern - Can be in hex (eg: "0xB055") or in binary (eg: "011011")
 * @return Error Code generated by invalid input pattern or pattern length (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_config_shared_tx_pattern( const phymod_access_t *pa, uint8_t patt_length, const char pattern[]);


/*-----------------------*/
/*  IDDQ / Clkgate APIs  */
/*-----------------------*/
/** Core configuration for IDDQ.
 * Note: User also needs to configure all lanes through eagle_tsc_lane_config_for_iddq()
 * before enabling IDDQ by asserting IDDQ pin.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_core_config_for_iddq( const phymod_access_t *pa );

/** Serdes Core Powerdown.
 * Along with eagle_tsc_core_pwrdn(), all lanes powerdowns should also be issued using
 * eagle_tsc_lane_pwrdn() to complete a Core Powerdown.
 * @param pa phymod_access_t struct
 * @param mode based on enum #srds_core_pwrdn_mode_enum select from ON, CORE, DEEP power down modes
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_core_pwrdn( const phymod_access_t *pa, enum srds_core_pwrdn_mode_enum mode);


/**************************************************/
/* MISC reg access                                */
/**************************************************/

/** Masked Register Write to the currently selected Serdes IP core/lane through MDIO.
 * If using MDIO interface to access the registers, use this API to implement eagle_tsc_pmd_mwr_reg().
 * @param pa phymod_access_t struct
 * @param addr Address of register to be written
 * @param mask 16-bit mask indicating the position of the field with bits of 1s
 * @param lsb  LSB of the field
 * @param val  16bit value to be written
 */
err_code_t eagle_tsc_pmd_mdio_mwr_reg( const phymod_access_t *pa, uint16_t addr, uint16_t mask, uint8_t lsb, uint16_t val);


/**************************************************/
/* LANE Based APIs - Required to be used per Lane */
/**************************************************/

/*------------------------------------------------------------*/
/*  APIs to Write Lane Config and User variables into uC RAM  */
/*------------------------------------------------------------*/
/** Write to lane_config uC RAM variable.
 * Note: This API should be used only during configuration under dp_reset.
 * @param pa phymod_access_t struct
 * @param struct_val Value to be written into lane_config RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_set_uc_lane_cfg( const phymod_access_t *pa, struct eagle_tsc_uc_lane_config_st struct_val);

/*-----------------------------------------------------------*/
/*  APIs to Read Lane Config and User variables from uC RAM  */
/*-----------------------------------------------------------*/
/** Read value of lane_config uC RAM variable.
 * @param pa phymod_access_t struct
 * @param *struct_val Value read from lane_config RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_get_uc_lane_cfg( const phymod_access_t *pa, struct eagle_tsc_uc_lane_config_st *struct_val);


/*-----------------*/
/*  Configure PLL  */
/*-----------------*/
/** Configure PLL.
 *
 * Use core_s_rstb to re-initialize all registers to default before calling this function.
 *
 * Configures PLL registers to obtain the required VCO frequency.
 * @param pa phymod_access_t struct
 * @param pll_cfg Required PLL configuration
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_configure_pll( const phymod_access_t *pa, enum eagle_tsc_pll_enum pll_cfg);


/*-------------------------*/
/*  Merlin TX Analog APIs  */
/*-------------------------*/


/*-------------------------------*/
/*  Falcon/Eagle TX Analog APIs  */
/*-------------------------------*/

/** Write TX AFE parameters.
 * @param pa phymod_access_t struct
 * @param param selects the parameter to write based on #srds_tx_afe_settings_enum
 * @param val is the signed input value to the parameter
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_write_tx_afe( const phymod_access_t *pa, enum srds_tx_afe_settings_enum param, int8_t val);

/** Read TX AFE parameters.
 * @param pa phymod_access_t struct
 * @param param selects the parameter to read based on #srds_tx_afe_settings_enum
 * @param *val is the returned signed value of the parameter
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_read_tx_afe( const phymod_access_t *pa, enum srds_tx_afe_settings_enum param, int8_t *val);

/** Validates Eagle/Falcon TXFIR tap settings.
 * Returns failcodes if TXFIR settings are invalid.
 * @param pa phymod_access_t struct
 * @param pre   TXFIR pre tap value (0..31)
 * @param main  TXFIR main tap value (40..112)
 * @param post1 TXFIR post tap value (0..63)
 * @param post2 TXFIR post2 tap value (-15..15)
 * @param post3 TXFIR post3 tap value (-15..15)
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_validate_txfir_cfg( const phymod_access_t *pa, int8_t pre, int8_t main, int8_t post1, int8_t post2, int8_t post3);


/*-------------------*/
/* Display Eye Scan  */
/*-------------------*/

/** Displays Passive Eye Scan from -0.5 UI to 0.5UI to BER 1e-7.
 *  Function uses uC to acquire data.
 *  It also retrieves the data and displays it in ASCII-art style, where number N corresponds to 1e-N
 *
 * This function retrieves the data from uC in horizontal stripe fashion
 *
 * @param pa phymod_access_t struct
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_eye_scan( const phymod_access_t *pa );

/** Displays Passive Eye Scan Data.
 * This function can be used to display data from meas_lowber_eye_scan() function.
 *
 *  eyescan_options.horz_max = 31;  \n
 *  eyescan_options.horz_min = -31; \n
 *  eyescan_options.vert_max = 31;  \n
 *  eyescan_options.vert_min = -31; \n
 *  eyescan_options.hstep = 1;      \n
 *  eyescan_options.vstep = 1;      \n
 *  eyescan_options.timeout_in_milliseconds =  between 4 and 255
 *     (larger numbers will greatly increase test time!)
 * @param pa phymod_access_t struct
 * @param eyescan_options is structure of options which control min, max, step, time, and linerate
 * @param *buffer is pointer to array which contains all samples.
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_lowber_eye( const phymod_access_t *pa, const struct eagle_tsc_eyescan_options_st eyescan_options, uint32_t *buffer);

/** Measure Passive Eye Scan based on eyescan_options provided and return data in buffer.
 * It relies on the accuracy of eagle_tsc_delay_us() function. \n\n
 * The recommend settings are: \n
 *  eyescan_options.horz_max = 31;  \n
 *  eyescan_options.horz_min = -31; \n
 *  eyescan_options.vert_max = 31;  \n
 *  eyescan_options.vert_min = -31; \n
 *  eyescan_options.hstep = 1;      \n
 *  eyescan_options.vstep = 1;      \n
 *  eyescan_options.timeout_in_milliseconds =  between 4 and 255
 *     (larger numbers will greatly increase test time!) \n
 * @param pa phymod_access_t struct
 * @param eyescan_options is structure of options which control min, max, step, time, and linerate
 * @param *buffer is pointer to array which is large enough to store all samples.
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_meas_lowber_eye( const phymod_access_t *pa, const struct eagle_tsc_eyescan_options_st eyescan_options, uint32_t *buffer);

/** Start uC controller eye scan Function.
 * Eye scan function provides a stripe of data at a time either vertical or horizontal.
 * This function only initiates the processor actions.  User must use eagle_tsc_read_eye_scan_stripe() function
 * to get the data from uC.
 * @param pa phymod_access_t struct
 * @param direction specifies either EYE_SCAN_VERTICAL or EYE_SCAN_HORIZ striping
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_meas_eye_scan_start( const phymod_access_t *pa, uint8_t direction);

/** Read a Stripe of eye scan data from uC.
 * @param pa phymod_access_t struct
 * @param *buffer must be of size 64
 * @param *status returns a status word                 \n
 *    bit 15 - indicates the ey scan is complete        \n
 *    bit 14 - indicates uC is slower than read access  \n
 *    bit 13 - indicates uC is faster than read access  \n
 *    bit 12-8 - reserved                               \n
 *    bit 7-0 - indicates amount of data in the uC buffer
 *
 * @param pa phymod_access_t struct
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_read_eye_scan_stripe( const phymod_access_t *pa, uint32_t *buffer, uint16_t *status);

/** Display Stripe of eye scan data to stdout and log.
 * @param pa phymod_access_t struct
 * @param y is the vertical step 31 to -31 (Eagle) 124 to -124(Falcon)
 * @param *buffer must be of size 64
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_eye_scan_stripe( const phymod_access_t *pa, int8_t y, uint32_t *buffer);

/** Display Eye scan header to stdout and log.
 * @param pa phymod_access_t struct
 * @param i indicates the number of headers to display for parallel eye scan
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_eye_scan_header( const phymod_access_t *pa, int8_t i);

/** Display Eye scan footer to stdout and log.
 * @param pa phymod_access_t struct
 * @param i indicates the number of footers to display for parallel eye scan
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_eye_scan_footer( const phymod_access_t *pa, int8_t i);

/** Check status of eye scan operation in uC.
 * @param pa phymod_access_t struct
 * @param *status returns a status word                 \n
 *    bit 15 - indicates the eye scan is complete       \n
 *    bit 14 - indicates uC is slower than read access  \n
 *    bit 13 - indicates uC is faster than read access  \n
 *    bit 12-8  reserved                                \n
 *    bit 7-0 - indicates amount of data in the uC buffer
 *
 * @param pa phymod_access_t struct
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_read_eye_scan_status( const phymod_access_t *pa, uint16_t *status);

/** Restores uC after running diagnostic eye scans.
 * @param pa phymod_access_t struct
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_meas_eye_scan_done( const phymod_access_t *pa );

/** Start a uC controlled BER scan function.
 * This will tell the uC to make a number of BER measurements at different offsets
 * and provide data back to API as a block of data. Several types of tests can be
 * made including Passive (which can be run on any data pattern and does not affect
 * datapath) or Intrusive (which can be run only when PRBS pattern is being used
 * and will cause errors to occur).  Intrusive test has a limited vertical range!
 *
 * @param pa phymod_access_t struct
 * @param ber_scan_mode configures the type of test (use #srds_diag_ber_mode_enum)         \n
 * \verbatim
   bit 7   : reserved
   bit 6   : 1 = BER FAST scan mode (reduce minimum sample time from 0.1sec to 0.02sec
   bit 5-4 : used for vertical intrusive test only (not recommended)
              00=move 1 slicer in direction bit0 (slicer selected for max range)
              11=move both, independent direction(not depend on bit0) legacy 40nm mode
              01=move only odd(depends on bit0)
              10=move only even(depends on bit0)
   bit 3   : 1 = set passive scan to narrow vertical range(150mV); 0 = full range(250mV)
   bit 2   : 1 = intrusive eye scan; 0 = passive
   bit 1   : 1 = scan horizontal direction; 0 = scan vertical
   bit 0   : 1 = scan negative portion of eye to center; 1 = scan positive  \endverbatim
 * @param pa phymod_access_t struct
 * @param timer_control sets the total test time in units of ~1.31 seconds
 * @param max_error_control sets the error threshold for test in units of 16.(4=64 error threshold)
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_start_ber_scan_test( const phymod_access_t *pa, uint8_t ber_scan_mode, uint8_t timer_control, uint8_t max_error_control);

/** Reads the BER scan data from uC after test has completed.
 * @param pa phymod_access_t struct
 * @param *errors is pointer to 32 element array of uint32 which will contain error data
 * @param *timer_values is pointer to 32 element array of uint32 which will contain time data
 * @param *cnt returns the number of samples
 * @param timeout for polling data from uC (typically 2000)
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_read_ber_scan_data( const phymod_access_t *pa, uint32_t *errors, uint32_t *timer_values, uint8_t *cnt, uint32_t timeout);

/** Extrapolate BER and display margin information
 * @param pa phymod_access_t struct
 * @param rate specifies the data rate in Hz
 * @param ber_scan_mode the type of test used to take the data(use #srds_diag_ber_mode_enum)
 * @param *total_errs is pointer to 32 element array of uint32 containing the error data
 * @param *total_time is pointer to 32 element array of uint32 containing the time data
 * @param max_offset is the maximum offset setting which is present in data (usually 31)
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_ber_scan_data(const phymod_access_t *pa, USR_DOUBLE rate, uint8_t ber_scan_mode, uint32_t *total_errs, uint32_t *total_time, uint8_t max_offset);

/** Example eye margin projection API.
 * This is an example function which uses the following API's to measure and display BER margin projections
 * eagle_tsc_start_ber_scan_test(), eagle_tsc_read_ber_scan_data(), eagle_tsc_display_ber_scan_data().
 *
 * @param pa phymod_access_t struct
 * @param rate specifies the data rate in Hz
 * @param ber_scan_mode the type of test used to take the data(use #srds_diag_ber_mode_enum)
 * @param timer_control sets the total test time in units of ~1.31 seconds
 * @param max_error_control sets the error threshold for test in units of 16.(4=64 errors)
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_eye_margin_proj( const phymod_access_t *pa, USR_DOUBLE rate, uint8_t ber_scan_mode, uint8_t timer_control, uint8_t max_error_control);

/*-----------------------------------------------*/
/*  Get dynamic eye margin estimation values     */
/*-----------------------------------------------*/
/** Get dynamic eye estimation values
 * Reads and converts the Eye margin estimation from the uC.
 * @param pa phymod_access_t struct
 * @param *left_eye_mUI a pointer to integer with return value of eye margin in units of mUI
 * @param *right_eye_mUI a pointer to integer with return value of eye margin in units of mUI
 * @param *upper_eye_mV a pointer to integer with return value of eye margin in units of mV
 * @param *lower_eye_mV a pointer to integer with return value of eye margin in units of mV
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_get_eye_margin_est (const phymod_access_t *pa, int *left_eye_mUI, int *right_eye_mUI, int *upper_eye_mV, int *lower_eye_mV);


/*-----------------------------------------------*/
/*  Display Serdes Lane Config and Debug Status  */
/*-----------------------------------------------*/
/** Display current lane configuration.
 * Reads and displays all important lane configuration RAM variables and register fields.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_lane_config( const phymod_access_t *pa );

/** Display current lane debug status.
 * Reads and displays all vital lane user status and debug status.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_lane_debug_status( const phymod_access_t *pa );


/*-----------------------------*/
/*  Display Serdes Lane State  */
/*-----------------------------*/
/** Display current lane state.
 * Reads and displays all important lane state values in a single line.
 * \n Note: Call functions eagle_tsc_display_lane_state_hdr() before and eagle_tsc_display_lane_state_legend() after
 * to get a formatted lane state display with legend
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_lane_state( const phymod_access_t *pa );

/** Column definition header for eagle_tsc display state.
 * To be called before eagle_tsc_display_lane_state() function.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_lane_state_hdr( const phymod_access_t *pa );

/** Detailed explanation of each column in eagle_tsc display state.
 * To be called after eagle_tsc_display_lane_state() function to display the legends.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_lane_state_legend( const phymod_access_t *pa );

/*---------------------------------------*/
/*  Required Serdes Diag/Debug routines  */
/*---------------------------------------*/
/** Parameterized diagnostic function which provides comprehensive diagnostic and debug information
 * This function is required to be implemented by upper level software to enable eagle_tsc support.
 * @param pa phymod_access_t struct
 * @param diag_level is a MASK style parameter to enable variable level of display
 *     enum #srds_diag_level_enum can be used to help set the diag_level
 * bit 0 : 1 = display extended lane state similar to existing falcon ext lane state.
 *         0 = standard display lane state
 * bit 1 = display core state
 * bit 2 = event log
 * bit 3 = display fast eye scan
 * bit 4 = dump reg_dump 1 (core level registers)
 * bit 5 = dump reg_dump 2 (lane level registers)
 * bit 6 = dump core uC vars
 * bit 7 = dump lane uC vars
 * bit 8 = display lane debug state
 * bit 9 = display data for ber projection vertical
 * bit 10 = display data for ber projection horzontal
 *
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_diag_data( const phymod_access_t *pa, uint16_t diag_level);

/** Parameterized diagnostic function which access to all eagle_tsc control and status bits
 * This function is required to be implemented by upper level software to enable eagle_tsc support.
 * @param pa phymod_access_t struct
 * @param type controls the type of access requested specified through enum #srds_diag_access_enum
 * type = 0 : Register Read (param becomes count)
 * type = 1 : Register Read-Modify-Write (param becomes mask)
 * type = 2 : CORE RAM Read byte  (data becomes count)
 * type = 3 : CORE RAM Read-Modify-Write byte (param becomes mask)
 * type = 4 : CORE RAM Read word  (data becomes count)
 * type = 5 : CORE RAM Read-Modify-Write word (param becomes mask)
 * type = 6 : LANE RAM Read byte    (data becomes count)
 * type = 7 : LANE RAM Read-Modify-Write byte (param becomes mask)
 * type = 8 : LANE RAM Read word  (data becomes count)
 * type = 9 : LANE RAM Read-Modify-Write word (param becomes mask)
 * type = 10 : Global RAM Read byte  (data becomes count)
 * type = 11 : Global RAM Read-Modify-Write byte (param becomes mask)
 * type = 12 : Global RAM Read word  (data becomes count)
 * type = 13 : Global RAM Read-Modify-Write word (param becomes mask)
 * type = 14 : uC Command (addr becomes command; param becomes supp_info)
 *              See microcode for available commands and further information.
 * type = 15 : Enable Breakboint
 * type = 16 : Next or Goto Breakpoint (addr becomes breakpoint #)
 * type = 17 : Read Breakpoint
 * type = 18 : Disable Breakpoint
 * type = 19 : Gather BER projection data (addr becomes ber_mode;data becomes max time;param becomes error threshold)
 *
 * Global RAM access will ONLY WORK on Cores with direct RAM access
 *
 * @param pa phymod_access_t struct
 * @param addr in most cases is the address of the register or RAM location
 * @param data in most cases is the data to be written
 * @param param is the multipurpose parameter and can be mask or other data
 */
err_code_t eagle_tsc_diag_access( const phymod_access_t *pa, enum srds_diag_access_enum type, uint16_t addr, uint16_t data, uint16_t param);

/*-------------------------------------*/
/*   PMD_RX_LOCK and CL72/CL93 Status  */
/*-------------------------------------*/

/** PMD rx lock status of current lane.
 * @param pa phymod_access_t struct
 * @param *pmd_rx_lock PMD_RX_LOCK status of current lane returned by API
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_pmd_lock_status( const phymod_access_t *pa, uint8_t *pmd_rx_lock);

/** Display CL72 Status of current lane.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_display_cl72_status( const phymod_access_t *pa );



/*--------------------------------*/
/*  Serdes TX disable/RX Restart  */
/*--------------------------------*/
/** TX Disable.
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable TX disable (1 = TX Disable asserted; 0 = TX Disable removed)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_tx_disable( const phymod_access_t *pa, uint8_t enable);

/** Enable/disable Restart RX and hold.
 * (Reset DSC state machine into RESTART State and hold it till disabled)
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable Restart RX and hold (1 = RX restart and hold; 0 = Release hold in restart state)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_rx_restart( const phymod_access_t *pa, uint8_t enable);


/*-----------------------------*/
/*  Stop/Resume RX Adaptation  */
/*-----------------------------*/
/** Stop RX Adaptation on a Lane. Control is returned only after attempting to stop adaptation.
 * RX Adaptation needs to be stopped before modifying any of the VGA, PF or DFE taps.
 * @param pa phymod_access_t struct
 * @param enable Enable RX Adaptation stop (1 = Stop RX Adaptation on lane; 0 = Resume RX Adaptation on lane)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_stop_rx_adaptation( const phymod_access_t *pa, uint8_t enable);

/** Request to stop RX Adaptation on a Lane.
 * Control will be returned immediately before adaptaion is completely stopped.
 * RX Adaptation needs to be stopped before modifying any of the VGA, PF or DFE taps.
 * To resume RX adaptation, use the eagle_tsc_stop_rx_adaptation() API.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_request_stop_rx_adaptation( const phymod_access_t *pa );

/*------------------------------------*/
/*  Read/Write all RX AFE parameters  */
/*------------------------------------*/

/** Write to RX AFE settings.
 * Note: RX Adaptation needs to be stopped before modifying any of the VGA, PF or DFE taps.
 * @param pa phymod_access_t struct
 * @param param Enum (#srds_rx_afe_settings_enum) to select the required RX AFE setting to be modified
 * @param val Value to be written to the selected AFE setting
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_write_rx_afe( const phymod_access_t *pa, enum srds_rx_afe_settings_enum param, int8_t val);

/** Read from RX AFE settings.
 * @param pa phymod_access_t struct
 * @param param Enum (#srds_rx_afe_settings_enum) to select the required RX AFE setting to be read
 * @param *val Value to be written to the selected AFE setting
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_read_rx_afe( const phymod_access_t *pa, enum srds_rx_afe_settings_enum param, int8_t *val);

/*----------------------------*/
/*  Enable Pattern Generator  */
/*----------------------------*/
/** Enable/Disable Shared TX pattern generator.
 * Note: The patt_length input to the function should be the value sent to the eagle_tsc_config_shared_tx_pattern() function
 * @param pa phymod_access_t struct
 * @param enable Enable shared fixed pattern generator (1 = Enable; 0 = Disable)
 * @param patt_length length of the pattern used in eagle_tsc_config_shared_tx_pattern()
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_tx_shared_patt_gen_en( const phymod_access_t *pa, uint8_t enable, uint8_t patt_length);


/*----------------------------*/
/*  Configure PRBS Functions  */
/*----------------------------*/
/**  Configure PRBS Generator.
 * Once the PRBS generator is configured, to enable PRBS use the eagle_tsc_tx_prbs_en() API.
 * @param pa phymod_access_t struct
 * @param prbs_poly_mode PRBS generator mode select (selects required PRBS polynomial)
 * @param prbs_inv PRBS invert enable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_config_tx_prbs( const phymod_access_t *pa, enum srds_prbs_polynomial_enum prbs_poly_mode, uint8_t prbs_inv);

/**  Get PRBS Generator Configuration.
 * @param pa phymod_access_t struct
 * @param *prbs_poly_mode PRBS generator mode select (selects required PRBS polynomial)
 * @param *prbs_inv PRBS invert enable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_get_tx_prbs_config( const phymod_access_t *pa, enum srds_prbs_polynomial_enum *prbs_poly_mode, uint8_t *prbs_inv);

/** PRBS Generator Enable/Disable.
 * @param pa phymod_access_t struct
 * @param enable Enable PRBS Generator (1 = Enable; 0 = Disable)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_tx_prbs_en( const phymod_access_t *pa, uint8_t enable);

/** Get PRBS Generator Enable/Disable.
 * @param pa phymod_access_t struct
 * @param *enable returns the value of Enable PRBS Generator (1 = Enable; 0 = Disable)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_get_tx_prbs_en( const phymod_access_t *pa, uint8_t *enable);

/** PRBS Generator Single Bit Error Injection.
 * @param pa phymod_access_t struct
 * @param enable (1 = error is injected; 0 = no error is injected)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_tx_prbs_err_inject( const phymod_access_t *pa, uint8_t enable);

/**  Configure PRBS Checker.
 * Once the PRBS checker is configured, use the eagle_tsc_rx_prbs_en() API to enable the checker.
 * @param pa phymod_access_t struct
 * @param prbs_poly_mode PRBS checker mode select (selects required PRBS polynomial)
 * @param prbs_checker_mode Checker Mode to select PRBS LOCK state machine
 * @param prbs_inv PRBS invert enable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_config_rx_prbs( const phymod_access_t *pa, enum srds_prbs_polynomial_enum prbs_poly_mode, enum srds_prbs_checker_mode_enum prbs_checker_mode, uint8_t prbs_inv);

/**  Get PRBS Checker congifuration.
 * @param pa phymod_access_t struct
 * @param *prbs_poly_mode PRBS checker mode select (selects required PRBS polynomial)
 * @param *prbs_checker_mode Checker Mode to select PRBS LOCK state machine
 * @param *prbs_inv PRBS invert enable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_get_rx_prbs_config( const phymod_access_t *pa, enum srds_prbs_polynomial_enum *prbs_poly_mode, enum srds_prbs_checker_mode_enum *prbs_checker_mode, uint8_t *prbs_inv);

/** PRBS Checker Enable/Disable.
 * @param pa phymod_access_t struct
 * @param enable Enable PRBS Checker (1 = Enable; 0 = Disable)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_rx_prbs_en( const phymod_access_t *pa, uint8_t enable);

/** Get PRBS Checker Enable/Disable.
 * @param pa phymod_access_t struct
 * @param *enable returns with the value of Enable PRBS Checker (1 = Enable; 0 = Disable)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_get_rx_prbs_en( const phymod_access_t *pa, uint8_t *enable);

/** PRBS Checker LOCK status (live status).
 * @param pa phymod_access_t struct
 * @param *chk_lock Live lock status read by API
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_prbs_chk_lock_state( const phymod_access_t *pa, uint8_t *chk_lock);

/** PRBS Error Count and Lock Lost status.
 * Error count and lock lost read back as a single 32bit value. Bit 31 is lock lost and [30:0] is error count.
 * @param pa phymod_access_t struct
 * @param *prbs_err_cnt 32bit value returned by API ([30:0] = Error Count; [31] = Lock lost)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_prbs_err_count_ll( const phymod_access_t *pa, uint32_t *prbs_err_cnt);

/** PRBS Error Count and Lock Lost status.
 * Error count and lock lost read back on separate variables
 * @param pa phymod_access_t struct
 * @param *prbs_err_cnt 32bit Error count value
 * @param *lock_lost Lock Lost status (1 = if lock was ever lost)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_prbs_err_count_state( const phymod_access_t *pa, uint32_t *prbs_err_cnt, uint8_t *lock_lost);


/*--------------------------------------------------------------*/
/*  IDDQ / Powerdown / Deep Powerdown / Isolate Pins  */
/*--------------------------------------------------------------*/
/** Lane configurations for IDDQ.
 * Note: User needs to configure all lanes through eagle_tsc_lane_config_for_iddq() and also call eagle_tsc_core_config_for_iddq()
 * before enabling IDDQ by asserting IDDQ pin.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_lane_config_for_iddq( const phymod_access_t *pa );

/** Serdes Lane Powerdown.
 * Powers down option available - TX only, RX only, complete LANE, complete lane DEEP, PWR_ON
 * Note: To wake up from lane DEEP powerdown, first call PWR_ON and then de-assert lane_dp_reset [wr_ln_dp_s_rstb(0x1)].
 * @param pa phymod_access_t struct
 * @param mode Enable/Disable lane powerdown based on #srds_core_pwrdn_mode_enum
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_lane_pwrdn( const phymod_access_t *pa, enum srds_core_pwrdn_mode_enum mode);

/** TX_PI Fixed Frequency Mode.
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable TX_PI (1 = Enable; 0 = Disable)
 * @param freq_override_val Fixed Frequency Override value (freq_override_val = desired_ppm*8192/781.25; Range: -8192 to + 8192);
 * @return Error Code generated by invalid TX_PI settings (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_tx_pi_freq_override( const phymod_access_t *pa, uint8_t enable, int16_t freq_override_val);


/*--------------------------------------------*/
/*  Loopback and Ultra-Low Latency Functions  */
/*--------------------------------------------*/

/** Locks TX_PI to Loop timing.
 * @param pa phymod_access_t struct
 * @param enable Enable TX_PI lock to loop timing (1 = Enable Lock; 0 = Disable Lock)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_loop_timing( const phymod_access_t *pa, uint8_t enable);

/** Enable/Disable Digital Loopback.
 * @param pa phymod_access_t struct
 * @param enable Enable Digital Loopback (1 = Enable dig_lpbk; 0 = Disable dig_lpbk)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_dig_lpbk( const phymod_access_t *pa, uint8_t enable);


/** Enable/Disable Remote Loopback.
 * @param pa phymod_access_t struct
 * @param enable Enable Remote Loopback (1 = Enable rmt_lpbk; 0 = Disable rmt_lpbk)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_rmt_lpbk( const phymod_access_t *pa, uint8_t enable);

/** Configure PLL.
 *  
 * Use core_s_rstb to re-initialize all registers to default before calling this function.
 *      
 * Retrieve PLL dev value from enum value
 * @param pll_cfg enum value
 * @param new_pll_div returned pll divider
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors)
 */
err_code_t eagle_tsc_get_pll_div_from_enum (enum eagle_tsc_pll_enum pll_cfg, uint32_t *new_pll_div);

#endif
