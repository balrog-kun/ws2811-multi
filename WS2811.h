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
 * Inline asm macro to output three streams of 24-bit GRB values in
 * (G,R,B) order, MSBit first.  The three output pins must be on the same
 * atmega port.
 * 0 bits are 250ns hi, 1000ns lo, 1 bits are 1000ns hi, 250ns lo.
 * r18 = stream 0 byte to be output
 * r19 = stream 1 byte to be output
 * r20 = stream 2 byte to be output
 * r21 = original port value with all three pins low
 * r22 = original port value with all three pins high
 * r23 = original port value with the 3 pins set to their next output values
 * r24 = (+r25) outer loop counter
 * r16 = saved SREG
 * r17 = inner loop counter
 *
 * NOTE: a similar 4-pin version would also be possible but we'd need to
 * unroll the last two iterations to save enough cycles instead of only the
 * single last iteration.
 * Instead if you need 4 or more parallel outputs you're probably better off
 * with the approach where the output bits are pre-grouped into atmega port
 * values.
 */
#define WS2811_OUT_3_COMMON(PORT, PIN0, RGB0, PIN1, RGB1, PIN2, RGB2, LEN) \
asm volatile( \
/* initialise */ \
"    movw r24, %[len]      ; multiply len by 3\n" \
"    add r24, %A[len]\n" \
"    adc r25, %B[len]\n" \
"    add r24, %A[len]\n" \
"    adc r25, %B[len]\n" \
"    in r21, %[port]\n" \
"    mov r22, r21\n" \
"    sbr r22, (1 << %[pin0]) | (1 << %[pin1]) | (1 << %[pin2])\n" \
"    mov r23, r21\n" \
"    ldi r17, 7             ; load inner loop counter\n" \
"    in r16, __SREG__       ; timing-critical, so no interrupts\n" \
"    cli\n" \
"    rjmp 2f                ; start with the end-of-loop check\n" \
/* loop over the first 7 bits */ \
"1:  out %[port], r22       ; pins lo -> hi\n" \
"    nop\n" \
"    nop\n" \
"    nop\n" \
"    out %[port], r23       ; pins hi -> colour output bit\n" \
"    nop\n" \
"    nop\n" \
"    lsl r18                ; shift stream 0 byte to next bit, set Carry\n" \
"    bst r18, 7\n" \
"    bld r23, %[pin0]       ; load r18 bit 7 into pin0\n" \
"    lsl r19                ; shift stream 0 byte to next bit, set Carry\n" \
"    bst r19, 7\n" \
"    bld r23, %[pin1]       ; load r19 bit 7 into pin1\n" \
"    lsl r20                ; shift stream 1 byte to next bit, set Carry\n" \
"    bst r20, 7\n" \
"    bld r23, %[pin2]       ; load r20 bit 7 into pin2\n" \
"    out %[port], r21       ; pins hi -> lo if not already low\n" \
"    dec r17                ; decrement loop counter, set flags\n" \
"    brne 1b                ; (inner) loop if required\n" \
"    ldi r17, 7             ; reload inner loop counter\n" \
/* 8th bit - output & fetch next values */ \
"    out %[port], r22       ; pins lo -> hi\n" \
"2:  ld r18, %a[rgb0]+      ; load next stream 0 byte\n" \
"    bst r18, 7\n" \
"    out %[port], r23       ; pins hi -> colour output bit\n" \
"    bld r23, %[pin0]       ; load r18 bit 7 into pin0\n" \
"    ld r19, %a[rgb1]+      ; load next stream 1 byte\n" \
"    bst r19, 7\n" \
"    bld r23, %[pin1]       ; load r19 bit 7 into pin1\n" \
"    ld r20, %a[rgb2]+      ; load next stream 2 byte\n" \
"    bst r20, 7\n" \
"    bld r23, %[pin2]       ; load r20 bit 7 into pin2\n" \
"    sbiw r24, 1            ; decrement outer loop counter\n" \
"    out %[port], r21       ; pins hi -> lo if not already low\n" \
"    nop\n" \
"    brge 1b                ; (outer) loop if required\n" \
"    out __SREG__, r16      ; reenable interrupts if required\n" \
: \
: [rgb0] "e" (RGB0), \
  [rgb1] "e" (RGB1), \
  [rgb2] "e" (RGB2), \
  [len] "r" (LEN), /* For some reason we can't use "w" here to get r24 */ \
  [port] "I" (_SFR_IO_ADDR(PORT)), \
  [pin0] "I" (PIN0), \
  [pin1] "I" (PIN1), \
  [pin2] "I" (PIN2) \
: "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25", \
  "cc", "memory" \
)

/*
 * Inline asm macro to output four streams of 24-bit GRB values in
 * (G,R,B) order, MSBit first.  The four output pins must be on the same
 * atmega port.
 * 0 bits are 250ns hi, 1000ns lo, 1 bits are 1000ns hi, 250ns lo.
 * r17 = stream 0 byte to be output
 * r18 = stream 1 byte to be output
 * r19 = stream 2 byte to be output
 * r20 = stream 3 byte to be output
 * r21 = original port value with all three pins low
 * r22 = original port value with all three pins high
 * r23 = original port value with the 4 pins set to their next output values
 * r24 = (+r25) outer loop counter
 * r15 = saved SREG
 * r16 = inner loop counter
 */
#define WS2811_OUT_4_COMMON(PORT, PIN0, RGB0, PIN1, RGB1, PIN2, RGB2, PIN3, RGB3, LEN) \
asm volatile( \
/* initialise */ \
"    movw r24, %[len]      ; multiply len by 3\n" \
"    add r24, %A[len]\n" \
"    adc r25, %B[len]\n" \
"    add r24, %A[len]\n" \
"    adc r25, %B[len]\n" \
"    in r21, %[port]\n" \
"    mov r22, r21\n" \
"    sbr r22, (1 << %[pin0]) | (1 << %[pin1]) | (1 << %[pin2]) | (1 << %[pin3])\n" \
"    mov r23, r21\n" \
"    movw r26, %[rgb0]\n" \
"    ld r17, X+             ; load next stream 0 byte\n" \
"    movw %[rgb0], r26\n" \
"    movw r26, %[rgb1]\n" \
"    ld r18, X+             ; load next stream 1 byte\n" \
"    movw %[rgb1], r26\n" \
"    ld r19, %a[rgb2]+      ; load next stream 2 byte\n" \
"    ld r20, %a[rgb3]+      ; load next stream 3 byte\n" \
"    ldi r16, 6             ; load inner loop counter\n" \
"    in r15, __SREG__       ; timing-critical, so no interrupts\n" \
"    cli\n" \
"    rjmp 2f                ; start with the end-of-loop check\n" \
/* loop over the first 6 bits */ \
"1:  out %[port], r22       ; pins lo -> hi\n" \
"    nop\n" \
"    nop\n" \
"    lsl r17                ; shift stream 0 byte to next bit, set Carry\n" \
"    out %[port], r23       ; pins hi -> colour output bit\n" \
"    bst r17, 7\n" \
"    bld r23, %[pin0]       ; load r17 bit 7 into pin0\n" \
"    lsl r18                ; shift stream 1 byte to next bit, set Carry\n" \
"    bst r18, 7\n" \
"    bld r23, %[pin1]       ; load r18 bit 7 into pin1\n" \
"    lsl r19                ; shift stream 2 byte to next bit, set Carry\n" \
"    bst r19, 7\n" \
"    bld r23, %[pin2]       ; load r19 bit 7 into pin2\n" \
"    lsl r20                ; shift stream 3 byte to next bit, set Carry\n" \
"    bst r20, 7\n" \
"    bld r23, %[pin3]       ; load r20 bit 7 into pin3\n" \
"    out %[port], r21       ; pins hi -> lo if not already low\n" \
"    dec r16                ; decrement loop counter, set flags\n" \
"    brne 1b                ; (inner) loop if required\n" \
"    ldi r16, 6             ; reload inner loop counter\n" \
/* 7th bit - output & fetch next values */ \
"    out %[port], r22       ; pins lo -> hi\n" \
"    nop\n" \
"    nop\n" \
"    bst r17, 6\n" \
"    out %[port], r23       ; pins hi -> colour output bit\n" \
"    bld r23, %[pin0]       ; load r17 bit 6 into pin0\n" \
"    bst r18, 6\n" \
"    bld r23, %[pin1]       ; load r18 bit 6 into pin1\n" \
"    bst r19, 6\n" \
"    bld r23, %[pin2]       ; load r19 bit 6 into pin2\n" \
"    bst r20, 6\n" \
"    bld r23, %[pin3]       ; load r20 bit 6 into pin3\n" \
"    movw r26, %[rgb0]\n" \
"    ld r17, X+             ; load next stream 0 byte\n" \
"    movw %[rgb0], r26\n" \
"    out %[port], r21       ; pins hi -> lo if not already low\n" \
"    movw r26, %[rgb1]\n" \
"    ld r18, X+             ; load next stream 1 byte\n" \
/* 8th bit - output & fetch next values */ \
"    out %[port], r22       ; pins lo -> hi\n" \
"    movw %[rgb1], r26\n" \
"    ld r19, %a[rgb2]+      ; load next stream 2 byte\n" \
"    out %[port], r23       ; pins hi -> colour output bit\n" \
"    ld r20, %a[rgb3]+      ; load next stream 3 byte\n" \
"2:  bst r17, 7\n" \
"    bld r23, %[pin0]       ; load r17 bit 7 into pin0\n" \
"    bst r18, 7\n" \
"    bld r23, %[pin1]       ; load r18 bit 7 into pin1\n" \
"    bst r19, 7\n" \
"    bld r23, %[pin2]       ; load r19 bit 7 into pin2\n" \
"    bst r20, 7\n" \
"    sbiw r24, 1            ; decrement outer loop counter\n" \
"    out %[port], r21       ; pins hi -> lo if not already low\n" \
"    bld r23, %[pin3]       ; load r20 bit 7 into pin3\n" \
"    brge 1b                ; (outer) loop if required\n" \
"    out __SREG__, r15      ; reenable interrupts if required\n" \
: \
: [rgb0] "r" (RGB0), \
  [rgb1] "r" (RGB1), \
  [rgb2] "e" (RGB2), \
  [rgb3] "e" (RGB3), \
  [len] "r" (LEN), \
  [port] "I" (_SFR_IO_ADDR(PORT)), \
  [pin0] "I" (PIN0), \
  [pin1] "I" (PIN1), \
  [pin2] "I" (PIN2), \
  [pin3] "I" (PIN3) \
: "r15", "r16", "r18", "r19", "r20", "r21", "r22", "r23", \
  "r24", "r25", "r26", "r27", \
  "cc", "memory" \
)

/*
 * 7 parallel outputs should also be possible with the loop looking something
 * like the following, but may require all iterations to be unrolled or some
 * other trick to save enough cycles to load the bytes to be output into r16,
 * r17, r18, r19, r20, r21, r22.  This gets quite tricky.
"    ori r25, 0xff\n" \
"1:  out %[port], r25       ; pins lo -> hi\n" \
"    lsl r16                ; shift stream 0 byte to next bit, set Carry\n" \
"    rol r24                ; load Carry into pin0\n" \
"    lsl r22                ; shift stream 6 byte to next bit, set Carry\n" \
"    out %[port], r24       ; pins hi -> colour output bit\n" \
"    rol r24                ; load Carry into pin6\n" \
"    lsl r21                ; shift stream 5 byte to next bit, set Carry\n" \
"    rol r24                ; load Carry into pin5\n" \
"    lsl r20                ; shift stream 4 byte to next bit, set Carry\n" \
"    rol r24                ; load Carry into pin4\n" \
"    lsl r19                ; shift stream 3 byte to next bit, set Carry\n" \
"    rol r24                ; load Carry into pin3\n" \
"    lsl r18                ; shift stream 2 byte to next bit, set Carry\n" \
"    rol r24                ; load Carry into pin2\n" \
"    lsl r17                ; shift stream 1 byte to next bit, set Carry\n" \
"    rol r24                ; load Carry into pin1\n" \
"    out %[port], __zero_reg__ ; pins hi -> lo if not already low\n" \
"    dec r15                ; decrement loop counter, set flags\n" \
"    brne 1b                ; (inner) loop if required\n" \
"    ldi r15, 7             ; reload inner loop counter\n" \
 */

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

#define DEFINE_WS2811_OUT_3_COMMON_FN(NAME, PORT, PIN0, PIN1, PIN2) \
extern void NAME(const RGB_t *rgb0, const RGB_t *rgb1, const RGB_t *rgb2, \
        uint16_t len) __attribute__((noinline)); \
void NAME(const RGB_t *rgb0, const RGB_t *rgb1, const RGB_t *rgb2, \
        uint16_t len) { \
    WS2811_OUT_3_COMMON(PORT, PIN0, rgb0, PIN1, rgb1, PIN2, rgb2, len); \
}

#define DEFINE_WS2811_OUT_4_COMMON_FN(NAME, PORT, PIN0, PIN1, PIN2, PIN3) \
extern void NAME(const RGB_t *rgb0, const RGB_t *rgb1, const RGB_t *rgb2, \
        const RGB_t *rgb3, uint16_t len) __attribute__((noinline)); \
void NAME(const RGB_t *rgb0, const RGB_t *rgb1, const RGB_t *rgb2, \
        const RGB_t *rgb3, uint16_t len) { \
    WS2811_OUT_4_COMMON(PORT, PIN0, rgb0, PIN1, rgb1, PIN2, rgb2, PIN3, rgb3, \
            len); \
}

#endif /* WS2811_h */
