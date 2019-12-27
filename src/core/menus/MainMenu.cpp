/*
    cheali-charger - open source firmware for a variety of LiPo chargers
    Copyright (C) 2016  Paweł Stawicki. All right reserved.

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
#include "ProgramMenus.h"
#include "OptionsMenu.h"
#include "MainMenu.h"
#include "Menu.h"
#include "ProgramData.h"
#include "LcdPrint.h"
#include "memory.h"
#ifdef ENABLE_EXTERNAL_CONTROL
#include "ExtControl.h"
#endif
using namespace options;

namespace MainMenu {

    void printItem(uint8_t i) {
        if(i < 1) {
            lcdPrint_P(string_options);
        } else if(i < MAX_PROGRAMS + 1){
            ProgramData::printProgramData(i - 1);
        }
#ifdef ENABLE_EXTERNAL_CONTROL
        else {
            lcdPrint_P(string_extControl);
        }
#endif
    }
    void run()
    {
        int8_t index = 0;
        while(true) {
#ifdef ENABLE_EXTERNAL_CONTROL
            Menu::initialize(MAX_PROGRAMS + 2);
#else
            Menu::initialize(MAX_PROGRAMS + 1);
#endif
            Menu::printMethod_ = printItem;
            Menu::setIndex(index);
            index = Menu::run();
#ifdef ENABLE_EXTERNAL_CONTROL

			if(index == EXTERNAL_COMMAND_REQUEST) {
				if(ExtControl::getCommand() == ExtControl::CMD_STOP) {
					ExtControl::setState(ExtControl::STATE_STOPPED);
					index = -1;
				}
				else if(ExtControl::getCommand() == ExtControl::CMD_SETUP)
					ProgramMenus::selectProgram(ExtControl::getCommandData().data1);
				else {
					ExtControl::setState(ExtControl::STATE_IDLE);
				}
			}
			else if(index == MENU_EXIT) {
				ExtControl::setState(ExtControl::STATE_NOT_CONTROLING);
			}
#endif
            if(index >= 0)  {
                if(index == 0) {
                    OptionsMenu::run();
                } else if(index < MAX_PROGRAMS + 1){
                    ProgramMenus::selectProgram(index - 1);
                }
#ifdef ENABLE_EXTERNAL_CONTROL
                else {
            		if (ExtControl::currentState == ExtControl::STATE_NOT_CONTROLING) {
            			ExtControl::setState(ExtControl::STATE_BEGIN);
            		}
                }
#endif
            } else {
                index = 0;
            }
        }
    }

}



