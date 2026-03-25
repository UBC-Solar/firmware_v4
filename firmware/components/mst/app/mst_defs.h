#pragma once


/* VERBOSE: verbose debugging statements */
#define VERBOSE

/**
 * Hardware unit tests to verify that the hardware works properly.
 * If ANY are defined, main loop is replaced with the specified test(s).
 * Assume that these unit test will never finish,
 * i.e. you can't run two hardware unit test with the same fw build.
 */
#define UNIT_TEST_MCU
// #define UNIT_TEST_IO
// #define UNIT_TEST_CAN
// #define UNIT_TEST_ISOSPI
