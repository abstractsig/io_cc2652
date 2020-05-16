/*
 *
 * provide io compatible versions of the RFCC26C2 api
 *
 */
#include "RFCC26X2.h"

// Increases max RX packet length from 37 to 255
// Sets one byte firmware parameter at offset 0xA5 to 0xFF
#define BLE_APP_OVERRIDES() (uint32_t)0x00FF8A53


#include <ti/rf_patches/rf_patch_cpe_bt5.c>

/*
 * i need stuff from  ti_radio_config.h and ti_radio_config.c

//
//  RF Setting:   Bluetooth 5 HPOSC, LE 1M PHY (1 Msym/s GFSK, 1 Mbps data rate) -- Packet Tx with AUX_ADV_IND PDU
//
//  PHY:          bt5le1m
//  Setting file: setting_bt5_le_1m.json
//

// TX Power table size definition
#define TX_POWER_TABLE_SIZE 16

// TX Power Table Object
extern RF_TxPowerTable_Entry txPowerTable[];

// TI-RTOS RF Mode Object
extern RF_Mode RF_prop;

// RF Core API commands
extern rfc_CMD_BLE5_RADIO_SETUP_t RF_cmdBle5RadioSetup;
extern rfc_CMD_FS_t RF_cmdFs;
extern rfc_CMD_BLE5_ADV_AUX_t RF_cmdBle5AdvAux;
extern rfc_CMD_BLE5_GENERIC_RX_t RF_cmdBle5GenericRx;

// RF Core API Overrides
extern uint32_t pOverridesCommon[];
extern uint32_t pOverrides1Mbps[];
extern uint32_t pOverrides2Mbps[];
extern uint32_t pOverridesCoded[];

 */


// TX Power table
// The RF_TxPowerTable_DEFAULT_PA_ENTRY and RF_TxPowerTable_HIGH_PA_ENTRY macros are defined in RF.h.
// The following arguments are required:
// RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
// RF_TxPowerTable_HIGH_PA_ENTRY(bias, ibboost, boost, coefficient, ldoTrim)
// See the Technical Reference Manual for further details about the "txPower" Command field.
// The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
//RF_TxPowerTable_Entry txPowerTable[TX_POWER_TABLE_SIZE] =
RF_TxPowerTable_Entry txPowerTable[] =
{
    {-20, RF_TxPowerTable_DEFAULT_PA_ENTRY(6, 3, 0, 2) },
    {-18, RF_TxPowerTable_DEFAULT_PA_ENTRY(8, 3, 0, 3) },
    {-15, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 3, 0, 3) },
    {-12, RF_TxPowerTable_DEFAULT_PA_ENTRY(12, 3, 0, 5) },
    {-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(15, 3, 0, 5) },
    {-9, RF_TxPowerTable_DEFAULT_PA_ENTRY(16, 3, 0, 5) },
    {-6, RF_TxPowerTable_DEFAULT_PA_ENTRY(20, 3, 0, 8) },
    {-5, RF_TxPowerTable_DEFAULT_PA_ENTRY(22, 3, 0, 9) },
    {-3, RF_TxPowerTable_DEFAULT_PA_ENTRY(19, 2, 0, 12) },
    {0, RF_TxPowerTable_DEFAULT_PA_ENTRY(19, 1, 0, 20) },
    {1, RF_TxPowerTable_DEFAULT_PA_ENTRY(22, 1, 0, 20) },
    {2, RF_TxPowerTable_DEFAULT_PA_ENTRY(25, 1, 0, 25) },
    {3, RF_TxPowerTable_DEFAULT_PA_ENTRY(29, 1, 0, 28) },
    {4, RF_TxPowerTable_DEFAULT_PA_ENTRY(35, 1, 0, 39) },
    {5, RF_TxPowerTable_DEFAULT_PA_ENTRY(23, 0, 0, 57) },
    RF_TxPowerTable_TERMINATION_ENTRY
};



RF_Handle
RF_open (RF_Object *pObj,RF_Mode *pRfMode,RF_RadioSetup *pRadioSetup,RF_Params *params) {
	return NULL;
}
