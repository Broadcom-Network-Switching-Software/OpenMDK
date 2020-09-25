/*
 *         
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *         
 *     
 * DO NOT EDIT THIS FILE!
 */

#include <phymod/phymod.h>
#include <phymod/phymod.h>

#ifdef PHYMOD_DIAG

enum_mapping_t phymod_dispatch_type_t_mapping[] = {
#ifdef PHYMOD_EAGLE_SUPPORT
    {"phymodDispatchTypeEagle", phymodDispatchTypeEagle},
#endif /*PHYMOD_EAGLE_SUPPORT  */
#ifdef PHYMOD_FALCON_SUPPORT
    {"phymodDispatchTypeFalcon", phymodDispatchTypeFalcon},
#endif /*PHYMOD_FALCON_SUPPORT  */
#ifdef PHYMOD_QSGMIIE_SUPPORT
    {"phymodDispatchTypeQsgmiie", phymodDispatchTypeQsgmiie},
#endif /*PHYMOD_QSGMIIE_SUPPORT  */
#ifdef PHYMOD_TSCE_SUPPORT
    {"phymodDispatchTypeTsce", phymodDispatchTypeTsce},
#endif /*PHYMOD_TSCE_SUPPORT  */
#ifdef PHYMOD_TSCF_SUPPORT
    {"phymodDispatchTypeTscf", phymodDispatchTypeTscf},
#endif /*PHYMOD_TSCF_SUPPORT  */
#ifdef PHYMOD_PHY8806X_SUPPORT
    {"phymodDispatchTypePhy8806x", phymodDispatchTypePhy8806x},
#endif /*PHYMOD_PHY8806X_SUPPORT  */
#ifdef PHYMOD_FURIA_SUPPORT
    {"phymodDispatchTypeFuria", phymodDispatchTypeFuria},
#endif /*PHYMOD_FURIA_SUPPORT  */
#ifdef PHYMOD_VIPER_SUPPORT
    {"phymodDispatchTypeViper", phymodDispatchTypeViper},
#endif /*PHYMOD_VIPER_SUPPORT  */
#ifdef PHYMOD_SESTO_SUPPORT
    {"phymodDispatchTypeSesto", phymodDispatchTypeSesto},
#endif /*PHYMOD_SESTO_SUPPORT  */
#ifdef PHYMOD_QUADRA28_SUPPORT
    {"phymodDispatchTypeQuadra28", phymodDispatchTypeQuadra28},
#endif /*PHYMOD_QUADRA28_SUPPORT  */
#ifdef PHYMOD_QTCE_SUPPORT
    {"phymodDispatchTypeQtce", phymodDispatchTypeQtce},
#endif /*PHYMOD_QTCE_SUPPORT  */
#ifdef PHYMOD_HURACAN_SUPPORT
    {"phymodDispatchTypeHuracan", phymodDispatchTypeHuracan},
#endif /*PHYMOD_HURACAN_SUPPORT  */
#ifdef PHYMOD_MADURA_SUPPORT
    {"phymodDispatchTypeMadura", phymodDispatchTypeMadura},
#endif /*PHYMOD_MADURA_SUPPORT  */
#ifdef PHYMOD_FURIA_SUPPORT
    {"phymodDispatchTypeFuria_82212", phymodDispatchTypeFuria_82212},
#endif /*PHYMOD_FURIA_SUPPORT  */
#ifdef PHYMOD_DINO_SUPPORT
    {"phymodDispatchTypeDino", phymodDispatchTypeDino},
#endif /*PHYMOD_DINO_SUPPORT  */
    {NULL, 0}
};

enum_mapping_t phymod_port_loc_t_mapping[] = {
    {"phymodPortLocDC", phymodPortLocDC},
    {"phymodPortLocLine", phymodPortLocLine},
    {"phymodPortLocSys", phymodPortLocSys},
    {NULL, 0}
};

enum_mapping_t phymod_core_version_t_mapping[] = {
    {"phymodCoreVersionFalconA0", phymodCoreVersionFalconA0},
    {"phymodCoreVersionEagleA0", phymodCoreVersionEagleA0},
    {"phymodCoreVersionQsgmiieA0", phymodCoreVersionQsgmiieA0},
    {"phymodCoreVersionTsce4A0", phymodCoreVersionTsce4A0},
    {"phymodCoreVersionTsce12A0", phymodCoreVersionTsce12A0},
    {"phymodCoreVersionTscfA0", phymodCoreVersionTscfA0},
    {"phymodCoreVersionTscfB0", phymodCoreVersionTscfB0},
    {"phymodCoreVersionPhy8806x", phymodCoreVersionPhy8806x},
    {"phymodCoreVersionFuriaA2", phymodCoreVersionFuriaA2},
    {"phymodCoreVersionViperXA0", phymodCoreVersionViperXA0},
    {"phymodCoreVersionViperGA0", phymodCoreVersionViperGA0},
    {"phymodCoreVersionSestoA0", phymodCoreVersionSestoA0},
    {"phymodCoreVersionQuadra28", phymodCoreVersionQuadra28},
    {"phymodCoreVersionHuracan", phymodCoreVersionHuracan},
    {"phymodCoreVersionMadura", phymodCoreVersionMadura},
    {"phymodCoreVersionSestoB0", phymodCoreVersionSestoB0},
    {"phymodCoreVersionDino", phymodCoreVersionDino},
    {NULL, 0}
};

enum_mapping_t phymod_reset_mode_t_mapping[] = {
    {"phymodResetModeHard", phymodResetModeHard},
    {"phymodResetModeSoft", phymodResetModeSoft},
    {NULL, 0}
};

enum_mapping_t phymod_reset_direction_t_mapping[] = {
    {"phymodResetDirectionIn", phymodResetDirectionIn},
    {"phymodResetDirectionOut", phymodResetDirectionOut},
    {"phymodResetDirectionInOut", phymodResetDirectionInOut},
    {NULL, 0}
};

enum_mapping_t phymod_firmware_media_type_t_mapping[] = {
    {"phymodFirmwareMediaTypePcbTraceBackPlane", phymodFirmwareMediaTypePcbTraceBackPlane},
    {"phymodFirmwareMediaTypeCopperCable", phymodFirmwareMediaTypeCopperCable},
    {"phymodFirmwareMediaTypeOptics", phymodFirmwareMediaTypeOptics},
    {NULL, 0}
};

enum_mapping_t phymod_sequencer_operation_t_mapping[] = {
    {"phymodSeqOpStop", phymodSeqOpStop},
    {"phymodSeqOpStart", phymodSeqOpStart},
    {"phymodSeqOpRestart", phymodSeqOpRestart},
    {NULL, 0}
};

enum_mapping_t phymod_core_event_t_mapping[] = {
    {"phymodCoreEventPllLock", phymodCoreEventPllLock},
    {NULL, 0}
};

enum_mapping_t phymod_drivermode_t_mapping[] = {
    {"phymodTxDriverModeDefault", phymodTxDriverModeDefault},
    {"phymodTxDriverModeNotSupported", phymodTxDriverModeNotSupported},
    {"phymodTxDriverModeHalfAmp", phymodTxDriverModeHalfAmp},
    {"phymodTxDriverModeHalfAmpHiImped", phymodTxDriverModeHalfAmpHiImped},
    {NULL, 0}
};

enum_mapping_t phymod_media_typed_t_mapping[] = {
    {"phymodMediaTypeChipToChip", phymodMediaTypeChipToChip},
    {"phymodMediaTypeShort", phymodMediaTypeShort},
    {"phymodMediaTypeMid", phymodMediaTypeMid},
    {"phymodMediaTypeLong", phymodMediaTypeLong},
    {NULL, 0}
};

enum_mapping_t phymod_power_t_mapping[] = {
    {"phymodPowerOff", phymodPowerOff},
    {"phymodPowerOn", phymodPowerOn},
    {"phymodPowerOffOn", phymodPowerOffOn},
    {"phymodPowerNoChange", phymodPowerNoChange},
    {NULL, 0}
};

enum_mapping_t phymod_phy_hg2_codec_t_mapping[] = {
    {"phymodBcmHG2CodecOff", phymodBcmHG2CodecOff},
    {"phymodBcmHG2CodecOnWith8ByteIPG", phymodBcmHG2CodecOnWith8ByteIPG},
    {"phymodBcmHG2CodecOnWith9ByteIPG", phymodBcmHG2CodecOnWith9ByteIPG},
    {NULL, 0}
};

enum_mapping_t phymod_phy_tx_lane_control_t_mapping[] = {
    {"phymodTxTrafficDisable", phymodTxTrafficDisable},
    {"phymodTxTrafficEnable", phymodTxTrafficEnable},
    {"phymodTxReset", phymodTxReset},
    {"phymodTxSquelchOn", phymodTxSquelchOn},
    {"phymodTxSquelchOff", phymodTxSquelchOff},
    {"phymodTxElectricalIdleEnable", phymodTxElectricalIdleEnable},
    {"phymodTxElectricalIdleDisable", phymodTxElectricalIdleDisable},
    {NULL, 0}
};

enum_mapping_t phymod_phy_rx_lane_control_t_mapping[] = {
    {"phymodRxReset", phymodRxReset},
    {"phymodRxSquelchOn", phymodRxSquelchOn},
    {"phymodRxSquelchOff", phymodRxSquelchOff},
    {NULL, 0}
};

enum_mapping_t phymod_interface_t_mapping[] = {
    {"phymodInterfaceBypass", phymodInterfaceBypass},
    {"phymodInterfaceSR", phymodInterfaceSR},
    {"phymodInterfaceSR4", phymodInterfaceSR4},
    {"phymodInterfaceKX", phymodInterfaceKX},
    {"phymodInterfaceKX4", phymodInterfaceKX4},
    {"phymodInterfaceKR", phymodInterfaceKR},
    {"phymodInterfaceKR2", phymodInterfaceKR2},
    {"phymodInterfaceKR4", phymodInterfaceKR4},
    {"phymodInterfaceCX", phymodInterfaceCX},
    {"phymodInterfaceCX2", phymodInterfaceCX2},
    {"phymodInterfaceCX4", phymodInterfaceCX4},
    {"phymodInterfaceCR", phymodInterfaceCR},
    {"phymodInterfaceCR2", phymodInterfaceCR2},
    {"phymodInterfaceCR4", phymodInterfaceCR4},
    {"phymodInterfaceCR10", phymodInterfaceCR10},
    {"phymodInterfaceXFI", phymodInterfaceXFI},
    {"phymodInterfaceSFI", phymodInterfaceSFI},
    {"phymodInterfaceSFPDAC", phymodInterfaceSFPDAC},
    {"phymodInterfaceXGMII", phymodInterfaceXGMII},
    {"phymodInterface1000X", phymodInterface1000X},
    {"phymodInterfaceSGMII", phymodInterfaceSGMII},
    {"phymodInterfaceXAUI", phymodInterfaceXAUI},
    {"phymodInterfaceRXAUI", phymodInterfaceRXAUI},
    {"phymodInterfaceX2", phymodInterfaceX2},
    {"phymodInterfaceXLAUI", phymodInterfaceXLAUI},
    {"phymodInterfaceXLAUI2", phymodInterfaceXLAUI2},
    {"phymodInterfaceCAUI", phymodInterfaceCAUI},
    {"phymodInterfaceQSGMII", phymodInterfaceQSGMII},
    {"phymodInterfaceLR4", phymodInterfaceLR4},
    {"phymodInterfaceLR", phymodInterfaceLR},
    {"phymodInterfaceLR2", phymodInterfaceLR2},
    {"phymodInterfaceER", phymodInterfaceER},
    {"phymodInterfaceER2", phymodInterfaceER2},
    {"phymodInterfaceER4", phymodInterfaceER4},
    {"phymodInterfaceSR2", phymodInterfaceSR2},
    {"phymodInterfaceSR10", phymodInterfaceSR10},
    {"phymodInterfaceCAUI4", phymodInterfaceCAUI4},
    {"phymodInterfaceVSR", phymodInterfaceVSR},
    {"phymodInterfaceLR10", phymodInterfaceLR10},
    {"phymodInterfaceKR10", phymodInterfaceKR10},
    {"phymodInterfaceCAUI4_C2C", phymodInterfaceCAUI4_C2C},
    {"phymodInterfaceCAUI4_C2M", phymodInterfaceCAUI4_C2M},
    {"phymodInterfaceZR", phymodInterfaceZR},
    {"phymodInterfaceLRM", phymodInterfaceLRM},
    {"phymodInterfaceXLPPI", phymodInterfaceXLPPI},
    {"phymodInterfaceOTN", phymodInterfaceOTN},
    {NULL, 0}
};

enum_mapping_t phymod_ref_clk_t_mapping[] = {
    {"phymodRefClk156Mhz", phymodRefClk156Mhz},
    {"phymodRefClk125Mhz", phymodRefClk125Mhz},
    {"phymodRefClk106Mhz", phymodRefClk106Mhz},
    {"phymodRefClk161Mhz", phymodRefClk161Mhz},
    {"phymodRefClk174Mhz", phymodRefClk174Mhz},
    {"phymodRefClk312Mhz", phymodRefClk312Mhz},
    {"phymodRefClk322Mhz", phymodRefClk322Mhz},
    {"phymodRefClk349Mhz", phymodRefClk349Mhz},
    {"phymodRefClk644Mhz", phymodRefClk644Mhz},
    {"phymodRefClk698Mhz", phymodRefClk698Mhz},
    {"phymodRefClk155Mhz", phymodRefClk155Mhz},
    {"phymodRefClk156P6Mhz", phymodRefClk156P6Mhz},
    {"phymodRefClk157Mhz", phymodRefClk157Mhz},
    {"phymodRefClk158Mhz", phymodRefClk158Mhz},
    {"phymodRefClk159Mhz", phymodRefClk159Mhz},
    {"phymodRefClk168Mhz", phymodRefClk168Mhz},
    {"phymodRefClk172Mhz", phymodRefClk172Mhz},
    {"phymodRefClk173Mhz", phymodRefClk173Mhz},
    {"phymodRefClk169P409Mhz", phymodRefClk169P409Mhz},
    {"phymodRefClk348P125Mhz", phymodRefClk348P125Mhz},
    {"phymodRefClk162P948Mhz", phymodRefClk162P948Mhz},
    {"phymodRefClk336P094Mhz", phymodRefClk336P094Mhz},
    {"phymodRefClk168P12Mhz", phymodRefClk168P12Mhz},
    {"phymodRefClk346P74Mhz", phymodRefClk346P74Mhz},
    {"phymodRefClk167P41Mhz", phymodRefClk167P41Mhz},
    {"phymodRefClk345P28Mhz", phymodRefClk345P28Mhz},
    {"phymodRefClk162P26Mhz", phymodRefClk162P26Mhz},
    {"phymodRefClk334P66Mhz", phymodRefClk334P66Mhz},
    {NULL, 0}
};

enum_mapping_t phymod_triple_core_t_mapping[] = {
    {"phymodTripleCore444", phymodTripleCore444},
    {"phymodTripleCore343", phymodTripleCore343},
    {"phymodTripleCore442", phymodTripleCore442},
    {"phymodTripleCore244", phymodTripleCore244},
    {NULL, 0}
};

enum_mapping_t phymod_otn_type_t_mapping[] = {
    {"phymodOTNOTU1", phymodOTNOTU1},
    {"phymodOTNOTU1e", phymodOTNOTU1e},
    {"phymodOTNOTU2", phymodOTNOTU2},
    {"phymodOTNOTU2e", phymodOTNOTU2e},
    {"phymodOTNOTU2f", phymodOTNOTU2f},
    {"phymodOTNOTU3", phymodOTNOTU3},
    {"phymodOTNOTU3e2", phymodOTNOTU3e2},
    {"phymodOTNOTU4", phymodOTNOTU4},
    {NULL, 0}
};

enum_mapping_t phymod_an_mode_type_t_mapping[] = {
    {"phymod_AN_MODE_NONE", phymod_AN_MODE_NONE},
    {"phymod_AN_MODE_CL73", phymod_AN_MODE_CL73},
    {"phymod_AN_MODE_CL37", phymod_AN_MODE_CL37},
    {"phymod_AN_MODE_CL73BAM", phymod_AN_MODE_CL73BAM},
    {"phymod_AN_MODE_CL37BAM", phymod_AN_MODE_CL37BAM},
    {"phymod_AN_MODE_HPAM", phymod_AN_MODE_HPAM},
    {"phymod_AN_MODE_SGMII", phymod_AN_MODE_SGMII},
    {"phymod_AN_MODE_CL37BAM_10P9375G_VCO", phymod_AN_MODE_CL37BAM_10P9375G_VCO},
    {"phymod_AN_MODE_CL37_SGMII", phymod_AN_MODE_CL37_SGMII},
    {NULL, 0}
};

enum_mapping_t phymod_cl37_sgmii_speed_t_mapping[] = {
    {"phymod_CL37_SGMII_10M", phymod_CL37_SGMII_10M},
    {"phymod_CL37_SGMII_100M", phymod_CL37_SGMII_100M},
    {"phymod_CL37_SGMII_1000M", phymod_CL37_SGMII_1000M},
    {NULL, 0}
};

enum_mapping_t phymod_firmware_load_force_t_mapping[] = {
    {"phymodFirmwareLoadSkip", phymodFirmwareLoadSkip},
    {"phymodFirmwareLoadForce", phymodFirmwareLoadForce},
    {"phymodFirmwareLoadAuto", phymodFirmwareLoadAuto},
    {NULL, 0}
};

enum_mapping_t phymod_firmware_load_method_t_mapping[] = {
    {"phymodFirmwareLoadMethodNone", phymodFirmwareLoadMethodNone},
    {"phymodFirmwareLoadMethodInternal", phymodFirmwareLoadMethodInternal},
    {"phymodFirmwareLoadMethodExternal", phymodFirmwareLoadMethodExternal},
    {"phymodFirmwareLoadMethodProgEEPROM", phymodFirmwareLoadMethodProgEEPROM},
    {NULL, 0}
};

enum_mapping_t phymod_datapath_t_mapping[] = {
    {"phymodDatapathNormal", phymodDatapathNormal},
    {"phymodDatapathUll", phymodDatapathUll},
    {NULL, 0}
};

enum_mapping_t phymod_tx_input_voltage_t_mapping[] = {
    {"phymodTxInputVoltageDefault", phymodTxInputVoltageDefault},
    {"phymodTxInputVoltage1p00", phymodTxInputVoltage1p00},
    {"phymodTxInputVoltage1p25", phymodTxInputVoltage1p25},
    {NULL, 0}
};

enum_mapping_t phymod_operation_mode_t_mapping[] = {
    {"phymodOperationModeRetimer", phymodOperationModeRetimer},
    {"phymodOperationModeRepeater", phymodOperationModeRepeater},
    {NULL, 0}
};

enum_mapping_t phymod_autoneg_link_qualifier_t_mapping[] = {
    {"phymodAutonegLinkQualifierRegisterWrite", phymodAutonegLinkQualifierRegisterWrite},
    {"phymodAutonegLinkQualifierKRBlockLock", phymodAutonegLinkQualifierKRBlockLock},
    {"phymodAutonegLinkQualifierKR4BlockLock", phymodAutonegLinkQualifierKR4BlockLock},
    {"phymodAutonegLinkQualifierKR4PMDLock", phymodAutonegLinkQualifierKR4PMDLock},
    {"phymodAutonegLinkQualifierExternalPCS", phymodAutonegLinkQualifierExternalPCS},
    {"phymodAutonegLinkQualifierDefault", phymodAutonegLinkQualifierDefault},
    {NULL, 0}
};

enum_mapping_t phymod_loopback_mode_t_mapping[] = {
    {"phymodLoopbackGlobal", phymodLoopbackGlobal},
    {"phymodLoopbackGlobalPMD", phymodLoopbackGlobalPMD},
    {"phymodLoopbackGlobalPCS", phymodLoopbackGlobalPCS},
    {"phymodLoopbackRemotePMD", phymodLoopbackRemotePMD},
    {"phymodLoopbackRemotePCS", phymodLoopbackRemotePCS},
    {"phymodLoopbackSysGlobal", phymodLoopbackSysGlobal},
    {"phymodLoopbackSysGlobalPMD", phymodLoopbackSysGlobalPMD},
    {"phymodLoopbackSysGlobalPCS", phymodLoopbackSysGlobalPCS},
    {"phymodLoopbackSysRemotePMD", phymodLoopbackSysRemotePMD},
    {"phymodLoopbackSysRemotePCS", phymodLoopbackSysRemotePCS},
    {NULL, 0}
};

enum_mapping_t phymod_pcs_userspeed_mode_t_mapping[] = {
    {"phymodPcsUserSpeedModeST", phymodPcsUserSpeedModeST},
    {"phymodPcsUserSpeedModeHTO", phymodPcsUserSpeedModeHTO},
    {NULL, 0}
};

enum_mapping_t phymod_pcs_userspeed_param_t_mapping[] = {
    {"phymodPcsUserSpeedParamEntry", phymodPcsUserSpeedParamEntry},
    {"phymodPcsUserSpeedParamHCD", phymodPcsUserSpeedParamHCD},
    {"phymodPcsUserSpeedParamClear", phymodPcsUserSpeedParamClear},
    {"phymodPcsUserSpeedParamPllDiv", phymodPcsUserSpeedParamPllDiv},
    {"phymodPcsUserSpeedParamPmaOS", phymodPcsUserSpeedParamPmaOS},
    {"phymodPcsUserSpeedParamScramble", phymodPcsUserSpeedParamScramble},
    {"phymodPcsUserSpeedParamEncode", phymodPcsUserSpeedParamEncode},
    {"phymodPcsUserSpeedParamCl48CheckEnd", phymodPcsUserSpeedParamCl48CheckEnd},
    {"phymodPcsUserSpeedParamBlkSync", phymodPcsUserSpeedParamBlkSync},
    {"phymodPcsUserSpeedParamReorder", phymodPcsUserSpeedParamReorder},
    {"phymodPcsUserSpeedParamCl36Enable", phymodPcsUserSpeedParamCl36Enable},
    {"phymodPcsUserSpeedParamDescr1", phymodPcsUserSpeedParamDescr1},
    {"phymodPcsUserSpeedParamDecode1", phymodPcsUserSpeedParamDecode1},
    {"phymodPcsUserSpeedParamDeskew", phymodPcsUserSpeedParamDeskew},
    {"phymodPcsUserSpeedParamDescr2", phymodPcsUserSpeedParamDescr2},
    {"phymodPcsUserSpeedParamDescr2ByteDel", phymodPcsUserSpeedParamDescr2ByteDel},
    {"phymodPcsUserSpeedParamBrcm64B66", phymodPcsUserSpeedParamBrcm64B66},
    {"phymodPcsUserSpeedParamSgmii", phymodPcsUserSpeedParamSgmii},
    {"phymodPcsUserSpeedParamClkcnt0", phymodPcsUserSpeedParamClkcnt0},
    {"phymodPcsUserSpeedParamClkcnt1", phymodPcsUserSpeedParamClkcnt1},
    {"phymodPcsUserSpeedParamLpcnt0", phymodPcsUserSpeedParamLpcnt0},
    {"phymodPcsUserSpeedParamLpcnt1", phymodPcsUserSpeedParamLpcnt1},
    {"phymodPcsUserSpeedParamMacCGC", phymodPcsUserSpeedParamMacCGC},
    {"phymodPcsUserSpeedParamRepcnt", phymodPcsUserSpeedParamRepcnt},
    {"phymodPcsUserSpeedParamCrdtEn", phymodPcsUserSpeedParamCrdtEn},
    {"phymodPcsUserSpeedParamPcsClkcnt", phymodPcsUserSpeedParamPcsClkcnt},
    {"phymodPcsUserSpeedParamPcsCGC", phymodPcsUserSpeedParamPcsCGC},
    {"phymodPcsUserSpeedParamCl72En", phymodPcsUserSpeedParamCl72En},
    {"phymodPcsUserSpeedParamNumOfLanes", phymodPcsUserSpeedParamNumOfLanes},
    {NULL, 0}
};

enum_mapping_t phymod_gpio_mode_t_mapping[] = {
    {"phymodGpioModeDisabled", phymodGpioModeDisabled},
    {"phymodGpioModeOutput", phymodGpioModeOutput},
    {"phymodGpioModeInput", phymodGpioModeInput},
    {NULL, 0}
};

enum_mapping_t phymod_osr_mode_t_mapping[] = {
    {"phymodOversampleMode1", phymodOversampleMode1},
    {"phymodOversampleMode2", phymodOversampleMode2},
    {"phymodOversampleMode3", phymodOversampleMode3},
    {"phymodOversampleMode3P3", phymodOversampleMode3P3},
    {"phymodOversampleMode4", phymodOversampleMode4},
    {"phymodOversampleMode5", phymodOversampleMode5},
    {"phymodOversampleMode7P5", phymodOversampleMode7P5},
    {"phymodOversampleMode8", phymodOversampleMode8},
    {"phymodOversampleMode8P25", phymodOversampleMode8P25},
    {"phymodOversampleMode10", phymodOversampleMode10},
    {"phymodOversampleMode16P5", phymodOversampleMode16P5},
    {"phymodOversampleMode20P625", phymodOversampleMode20P625},
    {NULL, 0}
};

enum_mapping_t phymod_timesync_timer_mode_t_mapping[] = {
    {"phymodTimesyncTimerModeNone", phymodTimesyncTimerModeNone},
    {"phymodTimesyncTimerModeDefault", phymodTimesyncTimerModeDefault},
    {"phymodTimesyncTimerMode32Bit", phymodTimesyncTimerMode32Bit},
    {"phymodTimesyncTimerMode48Bit", phymodTimesyncTimerMode48Bit},
    {"phymodTimesyncTimerMode64Bit", phymodTimesyncTimerMode64Bit},
    {"phymodTimesyncTimerMode80Bit", phymodTimesyncTimerMode80Bit},
    {NULL, 0}
};

enum_mapping_t phymod_timesync_global_mode_t_mapping[] = {
    {"phymodTimesyncGLobalModeFree", phymodTimesyncGLobalModeFree},
    {"phymodTimesyncGLobalModeSyncIn", phymodTimesyncGLobalModeSyncIn},
    {"phymodTimesyncGLobalModeCpu", phymodTimesyncGLobalModeCpu},
    {NULL, 0}
};

enum_mapping_t phymod_timesync_framesync_mode_t_mapping[] = {
    {"phymodTimesyncFramsyncModeNone", phymodTimesyncFramsyncModeNone},
    {"phymodTimesyncFramsyncModeSyncIn0", phymodTimesyncFramsyncModeSyncIn0},
    {"phymodTimesyncFramsyncModeSyncIn1", phymodTimesyncFramsyncModeSyncIn1},
    {"phymodTimesyncFramsyncModeSyncOut", phymodTimesyncFramsyncModeSyncOut},
    {"phymodTimesyncFramsyncModeCpu", phymodTimesyncFramsyncModeCpu},
    {NULL, 0}
};

enum_mapping_t phymod_timesync_syncout_mode_t_mapping[] = {
    {"phymodTimesyncSyncoutModeDisable", phymodTimesyncSyncoutModeDisable},
    {"phymodTimesyncSyncoutModeOneTime", phymodTimesyncSyncoutModeOneTime},
    {"phymodTimesyncSyncoutModePulseTrain", phymodTimesyncSyncoutModePulseTrain},
    {"phymodTimesyncSyncoutModePulseTrainWithSync", phymodTimesyncSyncoutModePulseTrainWithSync},
    {NULL, 0}
};

enum_mapping_t phymod_timesync_event_msg_action_t_mapping[] = {
    {"phymodTimesyncEventMsgActionNone", phymodTimesyncEventMsgActionNone},
    {"phymodTimesyncEventMsgActionEgrModeUpdateCorrectionField", phymodTimesyncEventMsgActionEgrModeUpdateCorrectionField},
    {"phymodTimesyncEventMsgActionEgrModeReplaceCorrectionFieldOrigin", phymodTimesyncEventMsgActionEgrModeReplaceCorrectionFieldOrigin},
    {"phymodTimesyncEventMsgActionEgrModeCaptureTimestamp", phymodTimesyncEventMsgActionEgrModeCaptureTimestamp},
    {"phymodTimesyncEventMsgActionIngModeUpdateCorrectionField", phymodTimesyncEventMsgActionIngModeUpdateCorrectionField},
    {"phymodTimesyncEventMsgActionIngModeInsertTimestamp", phymodTimesyncEventMsgActionIngModeInsertTimestamp},
    {"phymodTimesyncEventMsgActionIngModeInsertDelaytime", phymodTimesyncEventMsgActionIngModeInsertDelaytime},
    {NULL, 0}
};

enum_mapping_t phymod_edc_config_method_t_mapping[] = {
    {"phymodEdcConfigMethodNone", phymodEdcConfigMethodNone},
    {"phymodEdcConfigMethodHardware", phymodEdcConfigMethodHardware},
    {"phymodEdcConfigMethodSoftware", phymodEdcConfigMethodSoftware},
    {NULL, 0}
};

enum_mapping_t phymod_core_mode_t_mapping[] = {
    {"phymodCoreModeDefault", phymodCoreModeDefault},
    {"phymodCoreModeSingle", phymodCoreModeSingle},
    {"phymodCoreModeDual", phymodCoreModeDual},
    {"phymodCoreModeIndepLane", phymodCoreModeIndepLane},
    {"phymodCoreModeSplit012", phymodCoreModeSplit012},
    {"phymodCoreModeSplit023", phymodCoreModeSplit023},
    {"phymodCoreModeTriple244", phymodCoreModeTriple244},
    {"phymodCoreModeTriple343", phymodCoreModeTriple343},
    {"phymodCoreModeTriple442", phymodCoreModeTriple442},
    {NULL, 0}
};

enum_mapping_t phymod_failover_mode_t_mapping[] = {
    {"phymodFailovermodeNone", phymodFailovermodeNone},
    {"phymodFailovermodeEnable", phymodFailovermodeEnable},
    {NULL, 0}
};

enum_mapping_t phymod_eye_margin_mode_t_mapping[] = {
    {"phymod_eye_marign_HZ_L", phymod_eye_marign_HZ_L},
    {"phymod_eye_marign_HZ_R", phymod_eye_marign_HZ_R},
    {"phymod_eye_marign_VT_U", phymod_eye_marign_VT_U},
    {"phymod_eye_marign_VT_D", phymod_eye_marign_VT_D},
    {NULL, 0}
};

#endif /*PHYMOD_DIAG*/
