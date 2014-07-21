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
 * Inline asm macro to output two streams of 24-bit GRB values in
 * (G,R,B) order, MSBit first.
 * 0 bits are 250ns hi, 1000ns lo, 1 bits are 1000ns hi, 250ns lo.
 * r14 = stream 0 byte to be output
 * r15 = stream 1 byte to be output
 * r18 = port 0 original value with stream 0 pin low
 * r19 = port 1 original value with stream 1 pin low
 * r20 = port 0 original value with stream 0 pin high
 * r21 = port 1 original value with stream 1 pin high
 * r22 = port 0 original value with stream 0 pin set to the next output bit
 * r23 = port 1 original value with stream 1 pin set to the next output bit
 * r18:r19 = temp value
 * r16 = saved SREG
 * r17 = inner loop counter
 *
 * NOTE: a 3-pin version could be implemented in the same way, except
 * as far as I have tried, a single additional cycle was needed per bit.
 * Note that the (inner and outer) loop conditional jump has to be moved
 * to the middle 750nsec period because the jump requires 2 cycles, while
 * both of the two 250nsec periods are only 4-cycles long, 3 of which cycles
 * are used by "out" instructions.
 */
#define WS2811_OUT_2(PORT0, PIN0, RGB0, PORT1, PIN1, RGB1, LEN) \
asm volatile( \
/* initialise */ \
"    movw r18, %A[len]      ; multiply len by 3\n" \
"    add %A[len], r18\n" \
"    adc %B[len], r19\n" \
"    add %A[len], r18\n" \
"    adc %B[len], r19\n" \
"    in r18, %[port0]\n" \
"    in r19, %[port1]\n" \
"    movw r20, r18\n" \
"    sbr r20, 1 << %[pin0]\n" \
"    sbr r21, 1 << %[pin1]\n" \
"    movw r22, r18\n" \
"    ldi r17, 7             ; load inner loop counter\n" \
"    in r16, __SREG__       ; timing-critical, so no interrupts\n" \
"    cli\n" \
"    rjmp 2f               ; start with the end-of-loop check\n" \
/* loop over the first 7 bits */ \
"1:  out %[port0], r20      ; pin0 lo -> hi\n" \
"    out %[port1], r21      ; pin1 lo -> hi\n" \
"    nop\n" \
"    nop\n" \
"    out %[port0], r22      ; pin0 hi -> colour output bit\n" \
"    out %[port1], r23      ; pin1 hi -> colour output bit\n" \
"    nop\n" \
"    nop\n" \
"    nop\n" \
"    lsl r14                ; shift stream 0 byte to next bit, set Carry\n" \
"    bst r14, 7\n" \
"    bld r22, %[pin0]       ; load r14 bit 7 into pin0\n" \
"    lsl r15                ; shift stream 1 byte to next bit, set Carry\n" \
"    bst r15, 7\n" \
"    bld r23, %[pin1]       ; load r15 bit 7 into pin1\n" \
"    dec r17                ; decrement loop counter, set flags\n" \
"    out %[port0], r18      ; pin0 hi -> lo if not already low\n" \
"    out %[port1], r19      ; pin1 hi -> lo if not already low\n" \
"    brne 1b                ; (inner) loop if required\n" \
"    ldi r17, 7             ; reload inner loop counter\n" \
/* 8th bit - output & fetch next values */ \
"    out %[port0], r20      ; pin0 lo -> hi\n" \
"    out %[port1], r21      ; pin1 lo -> hi\n" \
"    nop\n" \
"    nop\n" \
"    out %[port0], r22      ; pin0 hi -> colour output bit\n" \
"    out %[port1], r23      ; pin1 hi -> colour output bit\n" \
"2:  ld r14, %a[rgb0]+      ; load next stream 0 byte\n" \
"    ld r15, %a[rgb1]+      ; load next stream 1 byte\n" \
"    bst r14, 7\n" \
"    bld r22, %[pin0]       ; load r14 bit 7 into pin0\n" \
"    bst r15, 7\n" \
"    bld r23, %[pin1]       ; load r15 bit 7 into pin1\n" \
"    sbiw %A[len], 1        ; decrement outer loop counter\n" \
"    out %[port0], r18      ; pin0 hi -> lo if not already low\n" \
"    out %[port1], r19      ; pin1 hi -> lo if not already low\n" \
"    brge 1b                ; (outer) loop if required\n" \
"    out __SREG__, r16      ; reenable interrupts if required\n" \
: \
: [rgb0] "e" (RGB0), \
  [rgb1] "e" (RGB1), \
  [len] "w" (LEN), \
  [port0] "I" (_SFR_IO_ADDR(PORT0)), \
  [port1] "I" (_SFR_IO_ADDR(PORT1)), \
  [pin0] "I" (PIN0), \
  [pin1] "I" (PIN1) \
: "r14", "r15", "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", \
  "cc", "memory" \
)

/*
 * Define C functions to wrap the inline WS2811 macro for given ports and pins.
 */
#define DEFINE_WS2811_OUT_1_FN(NAME, PORT, PIN) \
extern void NAME(const RGB_t *rgb, uint16_t len) __attribute__((noinline)); \
void NAME(const RGB_t *rgb, uint16_t len) { WS2811_OUT_1(PORT, PIN, rgb, len); }

#define DEFINE_WS2811_OUT_2_FN(NAME, PORT0, PIN0, PORT1, PIN1) \
extern void NAME(const RGB_t *rgb0, const RGB_t *rgb1, uint16_t len) \
    __attribute__((noinline)); \
void NAME(const RGB_t *rgb0, const RGB_t *rgb1, uint16_t len) { \
    WS2811_OUT_2(PORT0, PIN0, rgb0, PORT1, PIN1, rgb1, len); \
}

#endif /* WS2811_h */
