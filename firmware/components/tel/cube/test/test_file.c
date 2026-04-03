#include "unity.h"

#include "file.h"

void setUp(void) {
    // This is run before EACH TEST
}


void tearDown(void) {
    // This is run after EACH TEST
}


void test_main_return_1(void) {
    TEST_ASSERT_EQUAL(1U, main_return_1());
}