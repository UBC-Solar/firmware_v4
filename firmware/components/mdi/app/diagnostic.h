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
        volatile bool mdi_voltage_over_threshold : 1;
    } bits;
    volatile uint8_t raw;
} MdiDiagnosticFlags;

void DiagnosticInit(void);
void DiagnosticSetIwdgCrash(bool has_crash);
void DiagnosticSetVoltageOverThreshold(bool is_over_threshold);
void DiagnosticSendTimeSinceBootup(void);
void DiagnosticSendFlags(void);

#endif /* DIAGNOSTIC_H_ */
