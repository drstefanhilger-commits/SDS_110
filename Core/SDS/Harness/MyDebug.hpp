/*
 * MyDebug.hpp
 *
 *  Created on: Jul 26, 2026
 *      Author: 310004
 */

#pragma once

#include "SDS_Data.hpp"
#include "LCDDriver.hpp"
#include <stdio.h>
#include "DWT.hpp"

class MyDebug {
public:
	SDS_Data* pModel = &SDS_Data::instance();
	DWTTimer* pDWTTimer = &DWTTimer::instance();
	LCDDriver* pGfx = &LCDDriver::instance();

	static MyDebug& instance() {static MyDebug inst; return inst; };

//    void start() 	{ pDWTTimer->getStartTime(); };
//    void stop()  	{ pDWTTimer->getStopTime(); };
//    float diffMs()  { return 0; }; //pDWTTimer->getTimeDifferenceMs(); };
//
//    void deltaTimeUs(int x, int y, Color color=Color::White) {
//    	snprintf(buf, sizeof(buf), "DTime: %f [us]", pDWTTimer->getTimeDifferenceUs());
//    	pGfx->text8x12(x, y, buf, color);
//    };
//    void deltaTimeMs(int x, int y, Color color=Color::White) {
//    	snprintf(buf, sizeof(buf), "DTime: %f [ms]", pDWTTimer->getTimeDifferenceMs());
//    	pGfx->text8x12(x, y, buf, color);
//    };


private:
	MyDebug() {};
	char buf[64];

};
