/**
 * @file    diagnostic.h
 * @brief   MDI diagnostic flags.
 */
#ifndef DIAGNOSTIC_H_
#define DIAGNOSTIC_H_

#include <stdbool.h>
#include <stdint.h>

typedef union {
    struct {
        volatile bool mdi_crash_iwdg : 1;
    } bits;
    volatile uint8_t raw;
} MdiDiagnosticFlags;

extern MdiDiagnosticFlags g_mdi_diagnostic_flags;

#endif /* DIAGNOSTIC_H_ */
