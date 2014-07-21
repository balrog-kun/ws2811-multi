/*
 * Copyright 2012 Alan Burlison, alan@bleaklow.com.  All rights reserved.
 * Use is subject to license terms.
 *
 * The assembly rewritten and expanded by Andrew Zaborowski
 * <andrew.zaborowski@intel.com>.
 */

/*
 * WS2811 RGB LED driver.
 *
 * Note that all of the functions below probably read one byte beyond
 * the end of their input streams.
 */

#ifndef WS2811_h
#define WS2811_h

/* RGB value structure reordered for the WS2811's wiring. */
typedef struct __attribute__ ((__packed__)) {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} RGB_t;

#ifndef ARRAYLEN
#define ARRAYLEN(A) (sizeof(A) / sizeof(A[0]))
#endif

/*
 * Inline asm macro to output 24-bit GRB value in (G,R,B) order, MSBit first.
 * 0 bits are 250ns hi, 1000ns lo, 1 bits are 1000ns hi, 250ns lo.
 * r18 = byte to be output
 * r18:r19 = temp value
 * r16 = saved SREG
 * r17 = inner loop counter
 */
#define WS2811_OUT_1(PORT, PIN, RGB, LEN) \
asm volatile( \
/* initialise */ \
"    movw r18, %A[len]    ; multiply len by 3\n" \
"    add %A[len], r18\n" \
"    adc %B[len], r19\n" \
"    add %A[len], r18\n" \
"    adc %B[len], r19\n" \
"    ldi r17, 7           ; load inner loop counter\n" \
"    in r16, __SREG__     ; timing-critical, so no interrupts\n" \
"    cli\n" \
"    rjmp 6f             ; start with the end-of-loop check\n" \
/* loop over the first 7 bits */ \
"1:  sbi  %[port], %[pin] ; pin lo -> hi\n" \
"    sbrc r18, 7          ; test hi bit clear\n" \
"    rjmp 2f              ; true, skip pin hi -> lo\n" \
"    cbi  %[port], %[pin] ; false, pin hi -> lo\n" \
"2:  sbrc r18, 7          ; equalise delay of both code paths\n" \
"    rjmp 3f\n" \
"3:  nop                  ; pulse timing delay\n" \
"    nop\n" \
"    nop\n" \
"    nop\n" \
"    nop\n" \
"    nop\n" \
"    lsl r18              ; shift to next bit\n" \
"    dec r17              ; decrement loop counter, set flags\n" \
"    cbi %[port], %[pin]  ; pin hi -> lo\n" \
"    brne 1b              ; (inner) loop if required\n" \
"    ldi r17, 7           ; reload inner loop counter\n" \
/* 8th bit - output & fetch next values */ \
"    sbi %[port], %[pin]  ; pin lo -> hi\n" \
"    sbrc r18, 7          ; test hi bit clear\n" \
"    rjmp 4f              ; true, skip pin hi -> lo\n" \
"    cbi %[port], %[pin]  ; false, pin hi -> lo\n" \
"4:  sbrc r18, 7          ; equalise delay of both code paths\n" \
"    rjmp 5f\n" \
"5:  nop                  ; pulse timing delay\n" \
"    nop\n" \
"    nop\n" \
"    nop\n" \
"6:  ld r18, %a[rgb]+     ; load next byte\n" \
"    sbiw %A[len], 1      ; decrement outer loop counter, set flags\n" \
"    cbi %[port], %[pin]  ; pin hi -> lo\n" \
"    brge 1b              ; (outer) loop if required \n" \
"    out __SREG__, r16    ; reenable interrupts if required\n" \
: \
: [rgb] "e" (RGB), \
  [len] "w" (LEN), \
  [port] "I" (_SFR_IO_ADDR(PORT)), \
  [pin] "I" (PIN) \
: "r16", "r17", "r18", "r19", "cc", "memory" \
)

/*
 * Define a C function to wrap the inline WS2811 macro for a given port and pin.
 */
#define DEFINE_WS2811_OUT_1_FN(NAME, PORT, PIN) \
extern void NAME(const RGB_t *rgb, uint16_t len) __attribute__((noinline)); \
void NAME(const RGB_t *rgb, uint16_t len) { WS2811_OUT_1(PORT, PIN, rgb, len); }

#endif /* WS2811_h */
