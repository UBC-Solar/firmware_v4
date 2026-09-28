/**
 * @file    gps_app.c
 * @brief   GPS application source file for UBC Solar TEL board
 *
 * This file contains the functions for initializing and polling the GPS for the TEL board.
 *
 * @author  Gregory Bian
 * @date    Sep 22 2026
 */

#include "gps_app.h"
#include "gps_driver.h"

void GpsAppInit(void)
{
    GpsDriverInit();
}

void GpsAppTask(void)
{
}
