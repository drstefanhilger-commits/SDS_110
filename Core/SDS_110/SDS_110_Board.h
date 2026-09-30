/*
 * SDS_110_Board.h – Board-Schalter, aus C (main.c) und C++ nutzbar.
 *
 * Branch „ohne LCD“ des Discovery-Projekts SDS_110: gleicher Code wie im Repo
 * SDS_110_STM32F746ZGT6 (kein LCDTask, Hop-Puffer im internen RAM, nur Simulation).
 */
#ifndef SDS_110_BOARD_H
#define SDS_110_BOARD_H

/* Zielboard: dieses CubeMX-Projekt ist das STM32F746G-Discovery.
 * LED1 (PI1, grün) = Herzschlag, PC-Verbindung USB-CDC an CN13 (USB FS). */
#ifndef SDS110_BOARD_DISCO
#define SDS110_BOARD_DISCO 1
#endif

/* SAI2 / ADAU7118 (Mikrofonpfad 116) verwenden.
 * 0 = nur Signal-Simulator: 116 wird nicht initialisiert, USB-Kommando Typ 3 = 0 (Mikrofone)
 *     wird ignoriert. MX_SAI2_Init (CubeMX) läuft weiter, der SAI bleibt aber ungenutzt.
 * 1 = Mikrofone über SAI2 Block A + I2C1 (Discovery, SDS_110_Wrapper.cpp). */
#ifndef SDS110_SAI_ENABLED
#define SDS110_SAI_ENABLED 0
#endif

/* Verbindung zum PC-Monitor: 0 = USB-CDC (OTG FS, CN13). Der UART-Pfad (1) gehört zum Board
 * STM32F746ZGT6 (USART1 + CP2102N) und ist auf dem Discovery nicht nutzbar (USART1-RX an PB7). */
#ifndef SDS110_LINK_UART
#define SDS110_LINK_UART 0
#endif
#if SDS110_LINK_UART
#error "Discovery: PC-Verbindung nur über USB-CDC (SDS110_LINK_UART 0)"
#endif
#ifndef SDS110_UART_BAUD
#define SDS110_UART_BAUD 921600U
#endif

#endif /* SDS_110_BOARD_H */
