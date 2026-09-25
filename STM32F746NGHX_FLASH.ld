/*Valid*/
/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM); /* end of "RAM" Ram type memory */

_Min_Heap_Size = 0x800 ; /* required amount of heap */
_Min_Stack_Size = 0x800 ; /* required amount of stack */

/* Memories definition */
MEMORY
{
  /* DTCM 64 kB + SRAM1 240 kB (gecacht über die Standard-Speicherkarte). SRAM2 (16 kB) ist
     getrennt: nicht cachebarer Bereich für DMA-Puffer (MPUDriver.h, Region 0). */
  RAM    (xrw) : ORIGIN = 0x20000000, LENGTH = 304K
  RAM_NC (xrw) : ORIGIN = 0x2004C000, LENGTH = 16K
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  /* Externes SDRAM (8 MB ab 0xC0000000). Die ersten 2 MB sind für die beiden
     LTDC-Framebuffer reserviert (LCDDriver: 2 x 480x272x4 Byte ab 0xC0000000). */
  SDRAM (xrw) : ORIGIN = 0xC0200000, LENGTH = 6M
}

/* Sections */
SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)

    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* SDS_110: große DSP-Puffer (Mic-Frames, Spektren, Mel-Filterbank) im SDRAM.
     NOLOAD: keine Initialisierung durch den Startup-Code – das SDRAM ist erst nach
     MX_FMC_Init() nutzbar; die Objekte werden von ihren Konstruktoren/init() gesetzt. */
  .sdram_data (NOLOAD) :
  {
    . = ALIGN(32);
    _ssdram_data = .;
    *(.sdram_data)
    *(.sdram_data*)
    . = ALIGN(32);
    _esdram_data = .;
  } >SDRAM

  /* SDS_110: DMA-Puffer ohne Cache-Pflege (SDS110_DMA_SECTION), SRAM2, per MPU nicht cachebar.
     NOLOAD: der Inhalt wird von DMA bzw. init() geschrieben. */
  .dma_nocache (NOLOAD) :
  {
    . = ALIGN(32);
    *(.dma_nocache)
    *(.dma_nocache*)
    . = ALIGN(32);
  } >RAM_NC

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
