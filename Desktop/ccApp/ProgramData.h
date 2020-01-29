/*
    cheali-charger - open source firmware for a variety of LiPo chargers
    Copyright (C) 2013  Paweł Stawicki. All right reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PROGRAMDATA_H_
#define PROGRAMDATA_H_

#define MAX_PROGRAMS 20

#include <inttypes.h>
#include <stddef.h>
namespace AnalogInputs {
typedef  uint16_t ValueType ;
}
#define ANALOG_CELCIUS(x) ((AnalogInputs::ValueType)((x)*100))
#define CHEALI_EEPROM_PACKED __attribute__((packed))
struct Settings {
	enum UARTType {Disabled, Normal,  ExtControl, Debug,  ExtDebug, ExtDebugAdc};
	enum FanOnType {FanDisabled, FanAlways, FanProgram, FanTemperature, FanProgramTemperature};

	enum UARTOutput {TempOutput, Separated
#ifdef ENABLE_TX_HW_SERIAL_PIN7_PIN38
		, HardwarePin7, HardwarePin38
#endif
	};
	enum MenuType  {MenuSimple, MenuAdvanced};
	enum MenuButtonsType  {MenuButtonsNormal, MenuButtonsReversed};

	static const uint16_t UARTSpeeds = 5;
	static const AnalogInputs::ValueType TempDifference = ANALOG_CELCIUS(5.12);
	uint16_t backlight;

	uint16_t fanOn;
	AnalogInputs::ValueType fanTempOn;
	AnalogInputs::ValueType dischargeTempOff;

	uint16_t audioBeep;
	AnalogInputs::ValueType minIc;
	AnalogInputs::ValueType maxIc;
	AnalogInputs::ValueType minId;
	AnalogInputs::ValueType maxId;

	AnalogInputs::ValueType maxPc;
	AnalogInputs::ValueType maxPd;

	AnalogInputs::ValueType inputVoltageLow;
	uint16_t adcNoise;
	uint16_t UART;
	uint16_t UARTspeed;
	uint16_t UARToutput;
	uint16_t menuType;
	uint16_t menuButtons;

	void apply();
	void setDefault();
	uint32_t getUARTspeed() const;
	static const uint32_t UARTSpeedValue[];

	static void load();
	static void save();
	static void check();
	static void restoreDefault();

};

extern Settings settings;

namespace ProgramData {

    enum BatteryClass {ClassNiXX, ClassPb, ClassLiXX, ClassNiZn, ClassUnknown, ClassLED, LAST_BATTERY_CLASS};
    enum BatteryType {NoneBatteryType, NiCd, NiMH, Pb, Life, Lilo, Lipo, Li430, Li435, NiZn, UnknownBatteryType, LED, LAST_BATTERY_TYPE};
    enum VoltageType {VNominal, VCharged, VDischarged, VStorage, VvalidEmpty, LAST_VOLTAGE_TYPE};

    struct Battery {
        uint16_t type;
        uint16_t capacity;
        uint16_t cells;

        uint16_t Ic;
        uint16_t Id;

        uint16_t Vc_per_cell;
        uint16_t Vd_per_cell;

        uint16_t minIc;
        uint16_t minId;

        uint16_t time;

        uint16_t enable_externT;
        AnalogInputs::ValueType externTCO;

        uint16_t enable_adaptiveDischarge;
        uint16_t DCRestTime;
        uint16_t capCutoff;

        union {
            struct { //LiXX
                uint16_t Vs_per_cell; // storage
                uint16_t balancerError;
            };
            struct { //NiXX
                uint16_t enable_deltaV;
                int16_t deltaV;
                uint16_t deltaVIgnoreTime;
                uint16_t deltaT;
                uint16_t DCcycles;
            };
        };


    } CHEALI_EEPROM_PACKED;
	inline void check() {};
};

#endif /* PROGRAMDATA_H_ */



