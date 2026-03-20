/**
 * @file    LcdTest.h
 * @brief   LCD test header file for UBC Solar DRD board
 *
 * This header declares the test function prototypes for LCD pages.
 * Each test function sets up mock data for a specific LCD page.
 *
 * @author  Gregory Bian
 * @date    Mar 12 2026
 */

#ifndef __LCD_TEST_H
#define __LCD_TEST_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initiates the LCD Display test
 */
void LcdTestInit();
/**
 * @brief Test function for DEBUG_PAGE
 * Sets up mock debug page data including speed, drive state, and power metrics
 */
void LcdTestDebugPage(void);
/**
 * @brief Test function for FAULTS_PAGE
 * Sets up mock battery and motor fault data
 */
void LcdTestFaultsPage(void);

/**
 * @brief Test function for WARNINGS_PAGE
 * Sets up mock warning data
 */
void LcdTestWarningsPage(void);

/**
 * @brief Test function for TEMPERATURE_PAGE
 * Sets up mock temperature data for all 8 temperature sensors
 */
void LcdTestTemperaturePage(void);



#endif /* __LCD_TEST_H */
