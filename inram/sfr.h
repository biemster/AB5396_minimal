#ifndef _SFR_H
#define _SFR_H

#ifndef __ASSEMBLER__
#define SFR_RO *(volatile unsigned long const *)
#define SFR_WO *(volatile unsigned long*)
#define SFR_RW *(volatile unsigned long*)
#define SWINT() asm(".long 0xb0030057")
#define EEBREAKINT() asm(".long 0xb0040057")
#else
#define SFR_RO
#define SFR_WO
#define SFR_RW
#define SWINT      .long 0xb0030057
#define EEBREAKINT .long 0xb0040057
#endif

#define RTC_WR          0x200
#define RTC_RD          0x100

#define RTCCNT_CMD      0x01
#define RTCALM_CMD      0x02
#define RTCRAM_CMD      0x03
#define RTCCON0_CMD     0x04
#define RTCCON1_CMD     0x05
#define RTCCON2_CMD     0x06
#define RTCCON3_CMD     0x07
#define RTCCON4_CMD     0x08
#define RTCCON5_CMD     0x09
#define RTCCON6_CMD     0x0a
#define RTCCON7_CMD     0x0b
#define RTCCON8_CMD     0x0c
#define RTCCON9_CMD     0x0d
#define RTCCON10_CMD    0x0e

//------------------------- SFR Group0 ---------------------------------------//
#define TICK0CON          SFR_RW (0x00000004)
#define TICK0CPND         SFR_RW (0x00000008)
#define TICK0CNT          SFR_RW (0x0000000c)
#define TICK0PR           SFR_RW (0x00000010)
#define TICK1CON          SFR_RW (0x00000014)
#define TICK1CPND         SFR_RW (0x00000018)
#define FUNCMCON0         SFR_RW (0x0000001c)
#define FUNCMCON1         SFR_RW (0x00000020)
#define FUNCMCON2         SFR_RW (0x00000024)
#define TICK1CNT          SFR_RW (0x00000028)
#define TICK1PR           SFR_RW (0x0000002c)
#define CPDATA            SFR_RW (0x00000030)
#define SPMODE            SFR_RW (0x00000034)
#define DEVICEID          SFR_RW (0x00000038)
#define VERSIONID         SFR_RW (0x0000003c)

#define UART0CON          SFR_RW (0x00000040)
#define UART0CPND         SFR_WO (0x00000044)
#define UART0BAUD         SFR_RW (0x00000048)
#define UART0DATA         SFR_RW (0x0000004c)
#define TMR0CON           SFR_RW (0x00000050)
#define TMR0CPND          SFR_RW (0x00000054)
#define TMR0CNT           SFR_RW (0x00000058)
#define TMR0PR            SFR_RW (0x0000005c)
#define CRSTPND           SFR_RW (0x00000060)
#define CLKCON0           SFR_RW (0x00000064)
#define WDTCON            SFR_RW (0x00000068)
#define RTCCON            SFR_RW (0x0000006c)
#define RTCDAT            SFR_RW (0x00000070)
#define CLKCON1           SFR_RW (0x00000074)
#define RTCCPND           SFR_WO (0x00000078)
#define U0KEYCON          SFR_RW (0x0000007c)

#define SD0CON            SFR_RW (0x00000080)
#define SD0CPND           SFR_WO (0x00000084)
#define SD0BAUD           SFR_RW (0x00000088)
#define SD0CMD            SFR_RW (0x0000008c)
#define SD0ARG3           SFR_RW (0x00000090)
#define SD0ARG2           SFR_RW (0x00000094)
#define SD0ARG1           SFR_RW (0x00000098)
#define SD0ARG0           SFR_RW (0x0000009c)
#define SD0DMAADR         SFR_RW (0x000000a0)
#define SD0DMACNT         SFR_RW (0x000000a4)
#define SPI0CON           SFR_RW (0x000000a8)
#define SPI0BUF           SFR_RW (0x000000ac)
#define SPI0BAUD          SFR_RW (0x000000b0)
#define SPI0CPND          SFR_RW (0x000000b4)
#define SPI0DMACNT        SFR_RW (0x000000b8)
#define SPI0DMAADR        SFR_RW (0x000000bc)

#define UART1CON          SFR_RW (0x000000c0)
#define UART1CPND         SFR_WO (0x000000c4)
#define UART1BAUD         SFR_RW (0x000000c8)
#define UART1DATA         SFR_RW (0x000000cc)
#define U1KEYCON          SFR_RW (0x000000d0)
#define TMR1CON           SFR_RW (0x000000d4)
#define TMR1CPND          SFR_RW (0x000000d8)
#define TMR1CNT           SFR_RW (0x000000dc)
#define TMR1PR            SFR_RW (0x000000e0)
#define TMR2CON           SFR_RW (0x000000e8)
#define TMR2CPND          SFR_RW (0x000000ec)
#define TMR2CNT           SFR_RW (0x000000f0)
#define TMR2PR            SFR_RW (0x000000f4)
#define CLKGAT2           SFR_RW (0x000000f8)

//------------------------- SFR Group1 ---------------------------------------//
#define AUBUFDATA       SFR_RW (0x00000104)
#define AUBUFCON        SFR_RW (0x00000108)
#define AUBUFSTARTADDR  SFR_RW (0x0000010c)
#define AUBUFSIZE       SFR_RW (0x00000110)
#define AUBUFFIFOCNT    SFR_RW (0x00000114)
#define AUBUF1DATA      SFR_RW (0x00000118)
#define AUBUF1CON       SFR_RW (0x0000011c)
#define AUBUF1STARTADDR SFR_RW (0x00000120)
#define AUBUF1SIZE      SFR_RW (0x00000124)
#define AUBUF1FIFOCNT   SFR_RW (0x00000128)
#define AUBUFPEAKLEFT   SFR_RW (0x0000012c)
#define AUBUFPEAKRIGHT  SFR_RW (0x00000130)
#define AUBUFPTR        SFR_RW (0x00000134)
#define AUBUFBACKUP     SFR_RW (0x00000138)

#define DACDIGCON0      SFR_RW (0x0000013c)
#define DACDIGCON2      SFR_RW (0x00000140)
#define DACDIGCON3      SFR_RW (0x00000144)
#define PHASECOMP       SFR_RW (0x00000148)
#define DACVOLCON       SFR_RW (0x0000014c)
#define SRC0VOLCON      SFR_RW (0x00000150)
#define SRC1VOLCON      SFR_RW (0x00000154)
#define MIXWEIGHT       SFR_RW (0x00000158)
#define DACRMDCCON      SFR_RW (0x0000015c)
#define DACLDCEXP       SFR_RW (0x00000160)
#define DACBQ0CON       SFR_RW (0x00000164)
#define DACBQPEND       SFR_RW (0x00000168)
#define DACBQCOEF       SFR_RW (0x0000016c)
#define DACRAMTADDR     SFR_RW (0x00000170)
#define DACRAMTDATA     SFR_RW (0x00000174)

#define ADCRAMTADDR     SFR_RW (0x00000178)
#define ADCRAMTDATA     SFR_RW (0x0000017c)

#define AUCON0          SFR_RW (0x00000180)
#define AUCON1          SFR_RW (0x00000184)
#define AUCON2          SFR_RW (0x00000188)
#define AUCON3          SFR_RW (0x0000018c)
#define AUCON4          SFR_RW (0x00000190)
#define AUDMAADR        SFR_RW (0x00000194)

#define SBCECCON0       SFR_RW (0x00000198)
#define SBCECCON1       SFR_RW (0x0000019c)
#define SBCECDMAINADR   SFR_RW (0x000001a0)
#define SBCECDMAOUTADR  SFR_RW (0x000001a4)
#define SBCECDMACNT     SFR_RW (0x000001a8)

#define SDADCDMACON     SFR_RW (0x000001ac)
#define SDADCDMACLR     SFR_RW (0x000001b0)
#define SDADCDMAFLAG    SFR_RW (0x000001b4)
#define SDADCDMAADDR    SFR_RW (0x000001b8)
#define SDADCDMASIZE    SFR_RW (0x000001bc)
#define SDADCDIGCON     SFR_RW (0x000001c0)
#define SDADCGAINCON    SFR_RW (0x000001c4)
#define SDADCGETDCCON   SFR_RW (0x000001c8)

#define AUBUF0DMACON    SFR_RW (0x000001cc)
#define AUBUF0DMAADR    SFR_RW (0x000001d0)
#define AUBUF0DMAKICK   SFR_RW (0x000001d4)
#define AUBUF1DMACON    SFR_RW (0x000001d8)
#define AUBUF1DMAADR    SFR_RW (0x000001dc)
#define AUBUF1DMAKICK   SFR_RW (0x000001e0)

#define AUANGCON0       SFR_RW (0x000001e4)
#define AUANGCON1       SFR_RW (0x000001e8)
#define AUANGCON2       SFR_RW (0x000001ec)
#define AUANGCON3       SFR_RW (0x000001f0)
#define AUANGCON4       SFR_RW (0x000001f4)
#define AUANGCON5       SFR_RW (0x000001f8)
#define AUANGCON6       SFR_RW (0x000001fc)

//------------------------- SFR Group2 ---------------------------------------//
#define UFADDR          SFR_RW (0x00000200)
#define UPOWER          SFR_RW (0x00000204)
#define UINTRTX1        SFR_RW (0x00000208)
#define UINTRTX2        SFR_RW (0x0000020c)
#define UINTRRX1        SFR_RW (0x00000210)
#define UINTRRX2        SFR_RW (0x00000214)
#define UINTRUSB        SFR_RW (0x00000218)
#define UINTRTX1E       SFR_RW (0x0000021c)
#define UINTRTX2E       SFR_RW (0x00000220)
#define UINTRRX1E       SFR_RW (0x00000224)
#define UINTRRX2E       SFR_RW (0x00000228)
#define UINTRUSBE       SFR_RW (0x0000022c)
#define UFRAME1         SFR_RW (0x00000230)
#define UFRAME2         SFR_RW (0x00000234)
#define UINDEX          SFR_RW (0x00000238)
#define UDEVCTL         SFR_RW (0x0000023c)

#define UTXMAXP         SFR_RW (0x00000240)
#define UCSR0           SFR_RW (0x00000244)
#define UTXCSR1         SFR_RW (0x00000244)
#define UTXCSR2         SFR_RW (0x00000248)
#define URXMAXP         SFR_RW (0x0000024c)
#define URXCSR1         SFR_RW (0x00000250)
#define URXCSR2         SFR_RW (0x00000254)
#define UCOUNT0         SFR_RW (0x00000258)
#define URXCOUNT1       SFR_RW (0x00000258)
#define URXCOUNT2       SFR_RW (0x0000025c)
#define UTXTYPE         SFR_RW (0x00000260)
#define UTXINTERVAL     SFR_RW (0x00000264)
#define URXTYPE         SFR_RW (0x00000268)
#define URXINTERVAL     SFR_RW (0x0000026c)

#define UFIFO0          SFR_RW (0x00000280)
#define UFIFO1          SFR_RW (0x00000284)
#define UFIFO2          SFR_RW (0x00000288)
#define UFIFO3          SFR_RW (0x0000028c)
#define UFIFO4          SFR_RW (0x00000290)
#define UFIFO5          SFR_RW (0x00000294)
#define UFIFO6          SFR_RW (0x00000298)
#define UFIFO7          SFR_RW (0x0000029c)
#define UFIFO8          SFR_RW (0x000002a0)
#define UFIFO9          SFR_RW (0x000002a4)
#define UFIFO10         SFR_RW (0x000002a8)
#define UFIFO11         SFR_RW (0x000002ac)
#define UFIFO12         SFR_RW (0x000002b0)
#define UFIFO13         SFR_RW (0x000002b4)
#define UFIFO14         SFR_RW (0x000002b8)
#define UFIFO15         SFR_RW (0x000002bc)

//------------------------- SFR Group3 ---------------------------------------//
#define USBCON0         SFR_RW (0x00000300)
#define USBCON1         SFR_RW (0x00000304)
#define USBCON2         SFR_RW (0x00000308)
#define USBCON3         SFR_RW (0x0000030c)
#define USBCON4         SFR_RW (0x00000310)
#define USBEP0ADR       SFR_RW (0x00000314)
#define USBEP1RXADR     SFR_RW (0x00000318)
#define USBEP1TXADR     SFR_RW (0x0000031c)
#define USBEP2RXADR     SFR_RW (0x00000320)
#define USBEP2TXADR     SFR_RW (0x00000324)
#define USBEP3RXADR     SFR_RW (0x00000328)
#define USBEP3TXADR     SFR_RW (0x0000032c)
#define BTPHYTSCON      SFR_RW (0x00000330)
#define PLL2CON1        SFR_RW (0x00000334)
#define DBGCON1         SFR_RW (0x00000338)
#define DBGCON          SFR_RW (0x0000033c)

#define CRCDAT          SFR_RW (0x00000340)
#define CRCRES          SFR_RW (0x00000344)
#define LFSRRES         SFR_RW (0x00000348)
#define TESTDATA                LFSRRES
#define LFCRCCON        SFR_RW (0x0000034c)
#define BTDMAADR        SFR_RW (0x00000350)
#define PLL0CON1        SFR_RW (0x00000354)
#define PRCLKDIVCON0    SFR_RW (0x00000358)
#define PRCLKDIVCON1    SFR_RW (0x0000035c)
#define BTRFDMACON      SFR_RW (0x00000360)
#define BTADDMACNT      SFR_RW (0x00000364)
#define BTADDMAADR      SFR_RW (0x00000368)
#define BTDADMACNT      SFR_RW (0x0000036c)
#define BTDADMAADR      SFR_RW (0x00000370)
#define PWRCON0         SFR_RW (0x00000374)
#define LVDCON          SFR_RW (0x00000378)
#define PWRCON1         SFR_RW (0x0000037c)

#define CRC1DAT         SFR_RW (0x00000380)
#define CRC1RES         SFR_RW (0x00000384)
#define PLL1CON1        SFR_RW (0x00000388)
#define LPBUCKCON       SFR_RW (0x00000388)
#define PLL0DIV         SFR_RW (0x0000038c)
#define PLL1DIV         SFR_RW (0x00000390)
#define PLL2DIV         SFR_RW (0x00000394)
#define PLL0CON         SFR_RW (0x00000398)
#define PLL1CON         SFR_RW (0x0000039c)
#define XO26MCON        SFR_RW (0x000003a4)
#define CLKCON2         SFR_RW (0x000003a8)
#define CLKGAT0         SFR_RW (0x000003b0)
#define LPMCON          SFR_RW (0x000003b4)
#define MEMCON          SFR_RW (0x000003b8)
#define CLKDIVCON1      SFR_RW (0x000003bc)

#define HSUT0CON        SFR_RW (0x000003c0)
#define HSUT0CPND       SFR_RW (0x000003c4)
#define HSUT0BAUD       SFR_RW (0x000003c8)
#define HSUT0DATA       SFR_RW (0x000003cc)
#define HSUT0TXCNT      SFR_RW (0x000003d0)
#define HSUT0TXADR      SFR_RW (0x000003d4)
#define HSUT0RXCNT      SFR_RW (0x000003d8)
#define HSUT0RXADR      SFR_RW (0x000003dc)
#define HSUT0FIFOCNT    SFR_RW (0x000003e0)
#define HSUT0FIFO       SFR_RW (0x000003e4)
#define HSUT0FIFOADR    SFR_RW (0x000003e8)
#define HSUT0TMRCNT     SFR_RW (0x000003ec)
#define HSUT0FCCON      SFR_RW (0x000003f0)
#define BTCON4          SFR_RW (0x000003f4)
#define PWRCON2         SFR_RW (0x000003f8)
#define CLKGAT1         SFR_RW (0x000003fc)

//------------------------- SFR Group4 ---------------------------------------//
//0x00~0x3f reserve for CPU
#define EXCEPTPND       SFR_RO (0x00000400)
#define EXCEPTCPND      SFR_WO (0x00000404)
#define NMICON          SFR_RW (0x00000408)
#define BP0ADR          SFR_RW (0x0000040c)
#define BP1ADR          SFR_RW (0x00000410)
#define BP2ADR          SFR_RW (0x00000414)
#define BP3ADR          SFR_RW (0x00000418)
#define BP4ADR          SFR_RW (0x0000041c)
#define BP5ADR          SFR_RW (0x00000420)
#define BP6ADR          SFR_RW (0x00000424)
#define ICLOCK1         SFR_RW (0x00000428)
#define ICVAL1          SFR_RW (0x0000042c)
#define PICCONCLR       SFR_WO (0x00000430)
#define PICCONSET       SFR_WO (0x00000434)
#define PICENCLR        SFR_WO (0x00000438)
#define PICENSET        SFR_WO (0x0000043c)

#define PICCON          SFR_RW (0x00000440)
#define PICEN           SFR_RW (0x00000444)
#define PICPR           SFR_RW (0x00000448)
#define PICADR          SFR_RW (0x0000044c)
#define PICPND          SFR_RW (0x00000450)
#define CACHCON0        SFR_RW (0x00000454)
#define CACHCON1        SFR_RW (0x00000458)
#define ICTAG           SFR_RW (0x0000045c)
#define ICINDEX         SFR_RW (0x00000460)
#define ICADRMS         SFR_RW (0x00000464)
#define ICLOCK          SFR_RW (0x00000468)
#define ICVAL           SFR_RW (0x0000046c)
//#define DCADRMS         SFR_RW (0x00000470)
#define SWBK            SFR_RW (0x00000474)
#define EPICCON         SFR_RW (0x00000478)
#define EPC             SFR_RW (0x0000047c)

#define BP7ADR          SFR_RW (0x00000480)
#define BP8ADR          SFR_RW (0x00000484)
#define BP9ADR          SFR_RW (0x00000488)
#define BP10ADR         SFR_RW (0x0000048c)
#define BP11ADR         SFR_RW (0x00000490)
#define BP12ADR         SFR_RW (0x00000494)
#define BP13ADR         SFR_RW (0x00000498)
#define BP14ADR         SFR_RW (0x0000049c)
#define BP15ADR         SFR_RW (0x000004a0)
#define PICPR1          SFR_RW (0x000004a4)

#define PCERR           SFR_RW (0x000004b8)
#define PCST            SFR_RW (0x000004bc)

//------------------------- SFR Group5 ---------------------------------------//
#define PARCON0         SFR_RW (0x00000500)
#define PARCON1         SFR_RW (0x00000504)
#define PARCON2         SFR_RW (0x00000508)
#define PARCON3         SFR_RW (0x0000050c)
#define PARCON4         SFR_RW (0x00000510)
#define PARCON5         SFR_RW (0x00000514)
#define PARCON6         SFR_RW (0x00000518)

#define FUNCINCON       SFR_RW (0x0000051c)
#define FUNCOUTCON      SFR_RW (0x00000520)
#define FUNCOUTMCON     SFR_RW (0x00000524)
// #define FUNCMCON0       SFR_RW (0x00000528)
// #define FUNCMCON1       SFR_RW (0x0000052c)
// #define FUNCMCON2       SFR_RW (0x00000530)

#define GPDMACON        SFR_RW (0x00000540)
#define GPDMAADDRST0    SFR_RW (0x00000544)
#define GPDMAADDRST1    SFR_RW (0x00000548)
#define GPDMAKICK       SFR_RW (0x0000054c)

#define SADCDAT0        SFR_RO (0x00000580)
#define SADCDAT1        SFR_RO (0x00000584)
#define SADCDAT2        SFR_RO (0x00000588)
#define SADCDAT3        SFR_RO (0x0000058c)
#define SADCDAT4        SFR_RO (0x00000590)
#define SADCDAT5        SFR_RO (0x00000594)
#define SADCDAT6        SFR_RO (0x00000598)
#define SADCDAT7        SFR_RO (0x0000059c)
#define SADCDAT8        SFR_RO (0x000005a0)
#define SADCDAT9        SFR_RO (0x000005a4)
#define SADCDAT10       SFR_RO (0x000005a8)
#define SADCDAT11       SFR_RO (0x000005ac)
#define SADCDAT12       SFR_RO (0x000005b0)
#define SADCDAT13       SFR_RO (0x000005b4)
#define SADCDAT14       SFR_RO (0x000005b8)
#define SADCDAT15       SFR_RO (0x000005bc)

#define SADCCON         SFR_RW (0x000005c0)
#define SADCCH          SFR_RW (0x000005c4)
#define SADCST          SFR_WO (0x000005c8)
#define SADCBAUD        SFR_WO (0x000005cc)
#define MBISTCON        SFR_WO (0x000005d0)
#define MBISTEADR       SFR_WO (0x000005d4)
#define MBISTBADR       SFR_WO (0x000005d8)
#define MBISTCRC        SFR_WO (0x000005dc)
#define MBISTERR        SFR_WO (0x000005e0)
#define EFCON0          SFR_RW (0x000005f4)
#define EFCON1          SFR_WO (0x000005f8)
#define EFDAT           SFR_RW (0x000005fc)

//------------------------- SFR Group6 ---------------------------------------//
#define GPIOASET        LPSFR_WO (0x00000600)
#define GPIOACLR        LPSFR_WO (0x00000604)
#define GPIOA           LPSFR_RW (0x00000608)
#define GPIOADIR        LPSFR_RW (0x0000060c)
#define GPIOADE         LPSFR_RW (0x00000610)
#define GPIOAFEN        LPSFR_RW (0x00000614)
#define GPIOADRV        LPSFR_RW (0x00000618)
#define GPIOAPU         LPSFR_RW (0x0000061c)
#define GPIOAPD         LPSFR_RW (0x00000620)
#define GPIOAPU200K     LPSFR_RW (0x00000624)
#define GPIOAPD200K     LPSFR_RW (0x00000628)
#define GPIOAPU300      LPSFR_RW (0x0000062c)
#define GPIOAPD300      LPSFR_RW (0x00000630)

#define GPIOBSET        LPSFR_WO (0x00000640)
#define GPIOBCLR        LPSFR_WO (0x00000644)
#define GPIOB           LPSFR_RW (0x00000648)
#define GPIOBDIR        LPSFR_RW (0x0000064c)
#define GPIOBDE         LPSFR_RW (0x00000650)
#define GPIOBFEN        LPSFR_RW (0x00000654)
#define GPIOBDRV        LPSFR_RW (0x00000658)
#define GPIOBPU         LPSFR_RW (0x0000065c)
#define GPIOBPD         LPSFR_RW (0x00000660)
#define GPIOBPU200K     LPSFR_RW (0x00000664)
#define GPIOBPD200K     LPSFR_RW (0x00000668)
#define GPIOBPU300      LPSFR_RW (0x0000066c)
#define GPIOBPD300      LPSFR_RW (0x00000670)

#define GPIOESET        LPSFR_WO (0x00000680)
#define GPIOECLR        LPSFR_WO (0x00000684)
#define GPIOE           LPSFR_RW (0x00000688)
#define GPIOEDIR        LPSFR_RW (0x0000068c)
#define GPIOEDE         LPSFR_RW (0x00000690)
#define GPIOEFEN        LPSFR_RW (0x00000694)
#define GPIOEDRV        LPSFR_RW (0x00000698)
#define GPIOEPU         LPSFR_RW (0x0000069c)
#define GPIOEPD         LPSFR_RW (0x000006a0)
#define GPIOEPU200K     LPSFR_RW (0x000006a4)
#define GPIOEPD200K     LPSFR_RW (0x000006a8)
#define GPIOEPU300      LPSFR_RW (0x000006ac)
#define GPIOEPD300      LPSFR_RW (0x000006b0)

#define GPIOFSET        LPSFR_WO (0x000006c0)
#define GPIOFCLR        LPSFR_WO (0x000006c4)
#define GPIOF           LPSFR_RW (0x000006c8)
#define GPIOFDIR        LPSFR_RW (0x000006cc)
#define GPIOFDE         LPSFR_RW (0x000006d0)
#define GPIOFFEN        LPSFR_RW (0x000006d4)
#define GPIOFDRV        LPSFR_RW (0x000006d8)
#define GPIOFPU         LPSFR_RW (0x000006dc)
#define GPIOFPD         LPSFR_RW (0x000006e0)
#define GPIOFPU200K     LPSFR_RW (0x000006e4)
#define GPIOFPD200K     LPSFR_RW (0x000006e8)
#define GPIOFPU300      LPSFR_RW (0x000006ec)
#define GPIOFPD300      LPSFR_RW (0x000006f0)

//------------------------- SFR Group7 ---------------------------------------//
#define GPIOGSET        LPSFR_WO (0x00000700)
#define GPIOGCLR        LPSFR_WO (0x00000704)
#define GPIOG           LPSFR_RW (0x00000708)
#define GPIOGDIR        LPSFR_RW (0x0000070c)
#define GPIOGDE         LPSFR_RW (0x00000710)
#define GPIOGFEN        LPSFR_RW (0x00000714)
#define GPIOGDRV        LPSFR_RW (0x00000718)
#define GPIOGPU         LPSFR_RW (0x0000071c)
#define GPIOGPD         LPSFR_RW (0x00000720)
#define GPIOGPU200K     LPSFR_RW (0x00000724)
#define GPIOGPD200K     LPSFR_RW (0x00000728)
#define GPIOGPU300      LPSFR_RW (0x0000072c)
#define GPIOGPD300      LPSFR_RW (0x00000730)

#define WKUPCON         LPSFR_RW (0x00000740)
#define WKUPEDG         LPSFR_RW (0x00000744)
#define WKUPCPND        LPSFR_WO (0x00000748)
#define PORTINTEDG      LPSFR_RW (0x0000074c)
#define PORTINTEN       LPSFR_RW (0x00000750)
#define WKPINMAP        LPSFR_RW (0x00000754)
#define SENSCON         LPSFR_RW (0x00000758)
#define SENSCPND        LPSFR_WO (0x0000075c)
#define SENSCNT         LPSFR_RW (0x00000760)
#define SENSCON1        LPSFR_RW (0x00000764)
#define BTCON2          LPSFR_RW (0x00000768)

#define WKRSRC          LPSFR_RO (0x00000780)
#define WKFSRC          LPSFR_RO (0x00000784)
#define BTSNFCNT        LPSFR_RO (0x00000788)
#define BTSNCLKN        LPSFR_RO (0x0000078c)

//------------------------- SFR Group8 ---------------------------------------//
#define IIC0CON0        SFR_RW (0x00000800)
#define IIC0CON1        SFR_RW (0x00000804)
#define IIC0CMDA        SFR_RW (0x00000808)
#define IIC0DATA        SFR_RW (0x0000080c)
#define IIC0DMAADR      SFR_RW (0x00000810)
#define IIC0DMACNT      SFR_RW (0x00000814)
#define IIC0SSTS        SFR_RW (0x00000818)
#define IIC1CON0        SFR_RW (0x0000081c)
#define IIC1CON1        SFR_RW (0x00000820)
#define IIC1CMDA        SFR_RW (0x00000824)
#define IIC1DATA        SFR_RW (0x00000828)
#define IIC1DMAADR      SFR_RW (0x0000082c)
#define IIC1DMACNT      SFR_RW (0x00000830)
#define IIC1SSTS        SFR_RW (0x00000834)

#define AECFFTRST       SFR_RW (0x00000840)
#define AECFFTCON       SFR_RW (0x00000844)
#define AECFFTKICK      SFR_RW (0x00000848)
#define FFTDINADDR      SFR_RW (0x0000084c)
#define FFTBUFADDR      SFR_RW (0x00000850)
#define AECCON1         SFR_RW (0x00000854)
#define AECCON2         SFR_RW (0x00000858)
#define AECCON3         SFR_RW (0x0000085c)
#define AECCON4         SFR_RW (0x00000860)
#define AECCON5         SFR_RW (0x00000864)
#define AECFLAG         SFR_RW (0x00000868)
#define IRRXCON         SFR_RW (0x0000086c)
#define IRRXDAT         SFR_RW (0x00000870)
#define IRRXCPND        SFR_WO (0x00000874)
#define IRRXERR0        SFR_WO (0x00000878)
#define IRRXERR1        SFR_WO (0x0000087c)
#define IRRXPR0         SFR_WO (0x00000880)
#define IRRXPR1         SFR_WO (0x00000884)

#define PROTCON0        SFR_RW (0x00000888)
#define PROTCON1        SFR_RW (0x0000088c)
#define CVSDCON0        SFR_RW (0x00000890)
#define CVSDCPND        SFR_WO (0x00000894)
#define CVSDDMACNT      SFR_WO (0x00000898)
#define CVSDDMAERADR    SFR_WO (0x0000089c)
#define CVSDDMAEWADR    SFR_WO (0x000008a0)
#define CVSDDMADRADR    SFR_WO (0x000008a4)
#define CVSDDMADWADR    SFR_WO (0x000008a8)
#define CVSDPRED        SFR_RW (0x000008b0)
#define CVSDPREACC      SFR_RW (0x000008b4)
#define AECCON6         SFR_RW (0x000008bc)
#define AECCON7         SFR_RW (0x000008c0)


#define QDECCON         SFR_RW (0x000008c4)
#define QDECCPND        SFR_RW (0x000008c8)

#define SQRT64L         SFR_RW (0x000008cc)
#define SQRT64H         SFR_RW (0x000008d0)
#define PLCCON0         SFR_RW (0x000008d4)
#define PLCCON1         SFR_RW (0x000008d8)
#define PLCCON2         SFR_RW (0x000008dc)
#define MINPOWER        SFR_RW (0x000008e0)
#define PLCKICK         SFR_RW (0x000008e4)
#define PLCLOOPBASE     SFR_RW (0x000008e8)
#define PLCLOOPSIZE     SFR_RW (0x000008ec)
#define PLCSPTR         SFR_RW (0x000008f0)
#define PLCOPTR         SFR_RW (0x000008f4)
#define PLCFADDR        SFR_RW (0x000008f8)

//------------------------- SFR Group9 ---------------------------------------//
#define TMR3CON         SFR_RW (0x00000900)
#define TMR3CPND        SFR_WO (0x00000904)
#define TMR3CNT         SFR_RW (0x00000908)
#define TMR3PR          SFR_RW (0x0000090c)
#define TMR3CPT         SFR_RO (0x00000910)
#define TMR3DUTY0       SFR_WO (0x00000914)
#define TMR3DUTY1       SFR_WO (0x00000918)
#define TMR3DUTY2       SFR_WO (0x0000091c)
#define TMR4CON         SFR_RW (0x00000920)
#define TMR4CPND        SFR_WO (0x00000924)
#define TMR4CNT         SFR_RW (0x00000928)
#define TMR4PR          SFR_RW (0x0000092c)
#define TMR4CPT         SFR_RO (0x00000930)
#define TMR4DUTY0       SFR_WO (0x00000934)
#define TMR4DUTY1       SFR_WO (0x00000938)
#define TMR4DUTY2       SFR_WO (0x0000093c)

#define TMR5CON         SFR_RW (0x00000940)
#define TMR5CPND        SFR_WO (0x00000944)
#define TMR5CNT         SFR_RW (0x00000948)
#define TMR5PR          SFR_RW (0x0000094c)
#define TMR5CPT         SFR_RO (0x00000950)
#define TMR5DUTY0       SFR_WO (0x00000954)
#define TMR5DUTY1       SFR_WO (0x00000958)
#define TMR5DUTY2       SFR_WO (0x0000095c)
#define UART2CON        SFR_RW (0x00000960)
#define UART2CPND       SFR_WO (0x00000964)
#define UART2BAUD       SFR_RW (0x00000968)
#define UART2DATA       SFR_RW (0x0000096c)
// #define PORTINTEDG      SFR_RW (0x00000978)
// #define PORTINTEN       SFR_RW (0x0000097c)

#define SPI1CON         SFR_RW (0x00000980)
#define SPI1BUF         SFR_RW (0x00000984)
#define SPI1BAUD        SFR_RW (0x00000988)
#define SPI1CPND        SFR_RW (0x0000098c)
#define SPI1DMACNT      SFR_RW (0x00000990)
#define SPI1DMAADR      SFR_RW (0x00000994)

#define FREQDETCON      SFR_RW (0x000009a0)
#define FREQDETCPND     SFR_RW (0x000009a4)
#define FREQDETCNT      SFR_RW (0x000009a8)
#define FREQDETTGT      SFR_RW (0x000009ac)

#define RTCRAMADR       SFR_RW (0x000009b0)
#define RTCRAMDAT       SFR_RW (0x000009b4)
#define RTCALM          SFR_RW (0x000009b8)
#define RTCCNT          SFR_RW (0x000009bc)

#define RTCCON0         SFR_RW (0x000009c0)
#define RTCCON1         SFR_RW (0x000009c4)
#define RTCCON2         SFR_RW (0x000009c8)
#define RTCCON3         SFR_RW (0x000009cc)
#define RTCCON4         SFR_RW (0x000009d0)
#define RTCCON5         SFR_RW (0x000009d4)
#define RTCCON6         SFR_RW (0x000009d8)
#define RTCCON7         SFR_RW (0x000009dc)
#define RTCCON8         SFR_RW (0x000009e0)
#define RTCCON9         SFR_RW (0x000009e4)
#define RTCCON10        SFR_RW (0x000009e8)
#define RTCCON11        SFR_RW (0x000009ec)
#define RTCCON12        SFR_RW (0x000009f0)
#define RTCCON13        SFR_RW (0x000009f4)
#define RTCCON14        SFR_RW (0x000009f8)
#define RTCCON15        SFR_RW (0x000009fc)

//------------------------- SFR Group10 --------------------------------------//

#define PBQCON          SFR_RW (0x00000a00)
#define PBQ0CON         SFR_RW (0x00000a04)
//#define PBQ1CON         SFR_RW (0x00000a08)
#define PBQPEND         SFR_RW (0x00000a0c)
#define PBQCOEF         SFR_RW (0x00000a10)
#define PBQGAIN         SFR_RW (0x00000a14)
#define PBQDMARADR      SFR_RW (0x00000a18)
#define PBQDMAWADR      SFR_RW (0x00000a1c)
#define PBQDMASIZE      SFR_RW (0x00000a20)
#define PIANOCON        SFR_RW (0x00000a24)
#define PIANOBUF        SFR_WO (0x00000a28)
#define TONEDLY         SFR_RW (0x00000a2c)
#define TONEDLY1        SFR_RW (0x00000a30)
#define AU0DMAOCON      SFR_RW (0x00000a34)
#define AU0DMAOADR      SFR_RW (0x00000a38)
#define AU0DMAOSIZE     SFR_RW (0x00000a3c)

#define DECON           SFR_RW (0x00000a40)
#define DEPEND          SFR_RW (0x00000a44)
#define DEPAR0          SFR_RW (0x00000a48)
#define DEPAR1          SFR_RW (0x00000a4c)
#define DEPAR2          SFR_RW (0x00000a50)
#define DEPAR3          SFR_RW (0x00000a54)
#define DEPAR4          SFR_RW (0x00000a58)
#define DEPAR5          SFR_RW (0x00000a5c)
#define DEPAR6          SFR_RW (0x00000a60)
#define DEPAR7          SFR_RW (0x00000a64)
#define DEPAR8          SFR_RW (0x00000a68)
#define DEPAR9          SFR_RW (0x00000a6c)
#define DEPAR10         SFR_RW (0x00000a70)
#define DEPAR11         SFR_RW (0x00000a74)
#define DESPICON        SFR_RW (0x00000a78)
#define DESPIBUF        SFR_RW (0x00000a7c)

#define DESPIBAUD       SFR_RW (0x00000a80)
#define DESPICPND       SFR_RW (0x00000a84)
#define DESPIDMACNT     SFR_RW (0x00000a88)
#define DESPIDMAADR     SFR_RW (0x00000a8c)
#define FACON           SFR_RW (0x00000a90)
#define FAPEND          SFR_RW (0x00000a94)
#define FAPAR0          SFR_RW (0x00000a98)
#define FAPAR1          SFR_RW (0x00000a9c)
#define FAPAR2          SFR_RW (0x00000aa0)
#define FAPAR3          SFR_RW (0x00000aa4)
#define FAPAR4          SFR_RW (0x00000aa8)
#define WPTCON          SFR_RW (0x00000aac)
#define WPTPND          SFR_RW (0x00000ab0)
#define WPTADR          SFR_RW (0x00000ab4)
#define WPTDAT          SFR_RW (0x00000ab8)
#define MEMCON1         SFR_RW (0x00000abc)

#define MEMCON2         SFR_RW (0x00000ac0)
#define SWSINPHASE      SFR_RW (0x00000ac4)
#define SWSINWAVE       SFR_RW (0x00000ac8)

#ifndef __ASSEMBLER__
enum funo_select_tbl {
    FO_T5PWM0              = 0,
    FO_T5PWM1,
    FO_T5PWM2,
    FO_T5PWM3,
    FO_UR0TX,
    FO_HURTX,
    FO_UR1TX,
    FO_I2C0SCL,
    FO_I2C0SDA,
    FO_CLKOUT,
    FO_SPI1D0,
    FO_SPI1D1,
    FO_SPI1CLK,
    FO_I2C1SCL,
    FO_I2C1SDA,
};

enum funo_io_tbl {
    FO_PA0              = 1,
    FO_PA1,
    FO_PA2,
    FO_PA3,
    FO_PA4,
    FO_PA5,
    FO_PA6,
    FO_PA7,
    FO_PB0              = 9,
    FO_PB1,
    FO_PB2,
    FO_PB3,
    FO_PB4,
    FO_PB5,
    FO_PB6,
    FO_PB7,
    FO_PE0              = 17,
    FO_PE1,
    FO_PE2,
    FO_PE3,
    FO_PE4,
    FO_PE5,
    FO_PE6,
    FO_PE7,
    FO_PF0              = 25,
    FO_PF1,
    FO_PF2,
    FO_PF3,
    FO_PF4,
    FO_PF5,
};

enum funi_io_tbl {
    FI_PA0              = 0,
    FI_PA1,
    FI_PA2,
    FI_PA3,
    FI_PA4,
    FI_PA5,
    FI_PA6,
    FI_PA7,
    FI_PB0              = 8,
    FI_PB1,
    FI_PB2,
    FI_PB3,
    FI_PB4,
    FI_PB5,
    FI_PB6,
    FI_PB7,
    FI_PE0              = 16,
    FI_PE1,
    FI_PE2,
    FI_PE3,
    FI_PE4,
    FI_PE5,
    FI_PE6,
    FI_PE7,
    FI_PF0              = 24,
    FI_PF1,
    FI_PF2,
    FI_PF3,
    FI_PF4,
    FI_PF5,
    FI_PG0              = 30,
    FI_PG1,
    FI_PG2,
    FI_PG3,
    FI_PG4,
    FI_PG5,
};
#endif

//channel output function select
#define CH0_FUNO_SEL(ch0_funo_sel) FUNCOUTCON = (ch0_funo_sel << 0)
#define CH1_FUNO_SEL(ch1_funo_sel) FUNCOUTCON = (ch1_funo_sel << 8)
#define CH2_FUNO_SEL(ch2_funo_sel) FUNCOUTCON = (ch2_funo_sel <<16)
#define CH3_FUNO_SEL(ch3_funo_sel) FUNCOUTCON = (ch3_funo_sel <<24)

//channel 0 output mapping
#define CH0_FUNO_PA0MAP            FUNCOUTMCON = ( 1 << 0)
#define CH0_FUNO_PA1MAP            FUNCOUTMCON = ( 2 << 0)
#define CH0_FUNO_PA2MAP            FUNCOUTMCON = ( 3 << 0)
#define CH0_FUNO_PA3MAP            FUNCOUTMCON = ( 4 << 0)
#define CH0_FUNO_PA4MAP            FUNCOUTMCON = ( 5 << 0)
#define CH0_FUNO_PA5MAP            FUNCOUTMCON = ( 6 << 0)
#define CH0_FUNO_PA6MAP            FUNCOUTMCON = ( 7 << 0)
#define CH0_FUNO_PA7MAP            FUNCOUTMCON = ( 8 << 0)

#define CH0_FUNO_PB0MAP            FUNCOUTMCON = ( 9 << 0)
#define CH0_FUNO_PB1MAP            FUNCOUTMCON = (10 << 0)
#define CH0_FUNO_PB2MAP            FUNCOUTMCON = (11 << 0)
#define CH0_FUNO_PB3MAP            FUNCOUTMCON = (12 << 0)
#define CH0_FUNO_PB4MAP            FUNCOUTMCON = (13 << 0)
#define CH0_FUNO_PB5MAP            FUNCOUTMCON = (14 << 0)
#define CH0_FUNO_PB6MAP            FUNCOUTMCON = (15 << 0)
#define CH0_FUNO_PB7MAP            FUNCOUTMCON = (16 << 0)

#define CH0_FUNO_PE0MAP            FUNCOUTMCON = (17 << 0)
#define CH0_FUNO_PE1MAP            FUNCOUTMCON = (18 << 0)
#define CH0_FUNO_PE2MAP            FUNCOUTMCON = (19 << 0)
#define CH0_FUNO_PE3MAP            FUNCOUTMCON = (20 << 0)
#define CH0_FUNO_PE4MAP            FUNCOUTMCON = (21 << 0)
#define CH0_FUNO_PE5MAP            FUNCOUTMCON = (22 << 0)
#define CH0_FUNO_PE6MAP            FUNCOUTMCON = (23 << 0)
#define CH0_FUNO_PE7MAP            FUNCOUTMCON = (24 << 0)

#define CH0_FUNO_PF0MAP            FUNCOUTMCON = (25 << 0)
#define CH0_FUNO_PF1MAP            FUNCOUTMCON = (26 << 0)
#define CH0_FUNO_PF2MAP            FUNCOUTMCON = (27 << 0)
#define CH0_FUNO_PF3MAP            FUNCOUTMCON = (28 << 0)
#define CH0_FUNO_PF4MAP            FUNCOUTMCON = (29 << 0)
#define CH0_FUNO_PF5MAP            FUNCOUTMCON = (30 << 0)

//channel 1 output mapping
#define CH1_FUNO_PA0MAP            FUNCOUTMCON = ( 1 << 8)
#define CH1_FUNO_PA1MAP            FUNCOUTMCON = ( 2 << 8)
#define CH1_FUNO_PA2MAP            FUNCOUTMCON = ( 3 << 8)
#define CH1_FUNO_PA3MAP            FUNCOUTMCON = ( 4 << 8)
#define CH1_FUNO_PA4MAP            FUNCOUTMCON = ( 5 << 8)
#define CH1_FUNO_PA5MAP            FUNCOUTMCON = ( 6 << 8)
#define CH1_FUNO_PA6MAP            FUNCOUTMCON = ( 7 << 8)
#define CH1_FUNO_PA7MAP            FUNCOUTMCON = ( 8 << 8)

#define CH1_FUNO_PB0MAP            FUNCOUTMCON = ( 9 << 8)
#define CH1_FUNO_PB1MAP            FUNCOUTMCON = (10 << 8)
#define CH1_FUNO_PB2MAP            FUNCOUTMCON = (11 << 8)
#define CH1_FUNO_PB3MAP            FUNCOUTMCON = (12 << 8)
#define CH1_FUNO_PB4MAP            FUNCOUTMCON = (13 << 8)
#define CH1_FUNO_PB5MAP            FUNCOUTMCON = (14 << 8)
#define CH1_FUNO_PB6MAP            FUNCOUTMCON = (15 << 8)
#define CH1_FUNO_PB7MAP            FUNCOUTMCON = (16 << 8)

#define CH1_FUNO_PE0MAP            FUNCOUTMCON = (17 << 8)
#define CH1_FUNO_PE1MAP            FUNCOUTMCON = (18 << 8)
#define CH1_FUNO_PE2MAP            FUNCOUTMCON = (19 << 8)
#define CH1_FUNO_PE3MAP            FUNCOUTMCON = (20 << 8)
#define CH1_FUNO_PE4MAP            FUNCOUTMCON = (21 << 8)
#define CH1_FUNO_PE5MAP            FUNCOUTMCON = (22 << 8)
#define CH1_FUNO_PE6MAP            FUNCOUTMCON = (23 << 8)
#define CH1_FUNO_PE7MAP            FUNCOUTMCON = (24 << 8)

#define CH1_FUNO_PF0MAP            FUNCOUTMCON = (25 << 8)
#define CH1_FUNO_PF1MAP            FUNCOUTMCON = (26 << 8)
#define CH1_FUNO_PF2MAP            FUNCOUTMCON = (27 << 8)
#define CH1_FUNO_PF3MAP            FUNCOUTMCON = (28 << 8)
#define CH1_FUNO_PF4MAP            FUNCOUTMCON = (29 << 8)
#define CH1_FUNO_PF5MAP            FUNCOUTMCON = (30 << 8)

//channel 2 output mapping
#define CH2_FUNO_PA0MAP            FUNCOUTMCON = ( 1 <<16)
#define CH2_FUNO_PA1MAP            FUNCOUTMCON = ( 2 <<16)
#define CH2_FUNO_PA2MAP            FUNCOUTMCON = ( 3 <<16)
#define CH2_FUNO_PA3MAP            FUNCOUTMCON = ( 4 <<16)
#define CH2_FUNO_PA4MAP            FUNCOUTMCON = ( 5 <<16)
#define CH2_FUNO_PA5MAP            FUNCOUTMCON = ( 6 <<16)
#define CH2_FUNO_PA6MAP            FUNCOUTMCON = ( 7 <<16)
#define CH2_FUNO_PA7MAP            FUNCOUTMCON = ( 8 <<16)

#define CH2_FUNO_PB0MAP            FUNCOUTMCON = ( 9 <<16)
#define CH2_FUNO_PB1MAP            FUNCOUTMCON = (10 <<16)
#define CH2_FUNO_PB2MAP            FUNCOUTMCON = (11 <<16)
#define CH2_FUNO_PB3MAP            FUNCOUTMCON = (12 <<16)
#define CH2_FUNO_PB4MAP            FUNCOUTMCON = (13 <<16)
#define CH2_FUNO_PB5MAP            FUNCOUTMCON = (14 <<16)
#define CH2_FUNO_PB6MAP            FUNCOUTMCON = (15 <<16)
#define CH2_FUNO_PB7MAP            FUNCOUTMCON = (16 <<16)

#define CH2_FUNO_PE0MAP            FUNCOUTMCON = (17 <<16)
#define CH2_FUNO_PE1MAP            FUNCOUTMCON = (18 <<16)
#define CH2_FUNO_PE2MAP            FUNCOUTMCON = (19 <<16)
#define CH2_FUNO_PE3MAP            FUNCOUTMCON = (20 <<16)
#define CH2_FUNO_PE4MAP            FUNCOUTMCON = (21 <<16)
#define CH2_FUNO_PE5MAP            FUNCOUTMCON = (22 <<16)
#define CH2_FUNO_PE6MAP            FUNCOUTMCON = (23 <<16)
#define CH2_FUNO_PE7MAP            FUNCOUTMCON = (24 <<16)

#define CH2_FUNO_PF0MAP            FUNCOUTMCON = (25 <<16)
#define CH2_FUNO_PF1MAP            FUNCOUTMCON = (26 <<16)
#define CH2_FUNO_PF2MAP            FUNCOUTMCON = (27 <<16)
#define CH2_FUNO_PF3MAP            FUNCOUTMCON = (28 <<16)
#define CH2_FUNO_PF4MAP            FUNCOUTMCON = (29 <<16)
#define CH2_FUNO_PF5MAP            FUNCOUTMCON = (30 <<16)

//channel 3 output mapping
#define CH3_FUNO_PA0MAP            FUNCOUTMCON = ( 1 <<24)
#define CH3_FUNO_PA1MAP            FUNCOUTMCON = ( 2 <<24)
#define CH3_FUNO_PA2MAP            FUNCOUTMCON = ( 3 <<24)
#define CH3_FUNO_PA3MAP            FUNCOUTMCON = ( 4 <<24)
#define CH3_FUNO_PA4MAP            FUNCOUTMCON = ( 5 <<24)
#define CH3_FUNO_PA5MAP            FUNCOUTMCON = ( 6 <<24)
#define CH3_FUNO_PA6MAP            FUNCOUTMCON = ( 7 <<24)
#define CH3_FUNO_PA7MAP            FUNCOUTMCON = ( 8 <<24)

#define CH3_FUNO_PB0MAP            FUNCOUTMCON = ( 9 <<24)
#define CH3_FUNO_PB1MAP            FUNCOUTMCON = (10 <<24)
#define CH3_FUNO_PB2MAP            FUNCOUTMCON = (11 <<24)
#define CH3_FUNO_PB3MAP            FUNCOUTMCON = (12 <<24)
#define CH3_FUNO_PB4MAP            FUNCOUTMCON = (13 <<24)
#define CH3_FUNO_PB5MAP            FUNCOUTMCON = (14 <<24)
#define CH3_FUNO_PB6MAP            FUNCOUTMCON = (15 <<24)
#define CH3_FUNO_PB7MAP            FUNCOUTMCON = (16 <<24)

#define CH3_FUNO_PE0MAP            FUNCOUTMCON = (17 <<24)
#define CH3_FUNO_PE1MAP            FUNCOUTMCON = (18 <<24)
#define CH3_FUNO_PE2MAP            FUNCOUTMCON = (19 <<24)
#define CH3_FUNO_PE3MAP            FUNCOUTMCON = (20 <<24)
#define CH3_FUNO_PE4MAP            FUNCOUTMCON = (21 <<24)
#define CH3_FUNO_PE5MAP            FUNCOUTMCON = (22 <<24)
#define CH3_FUNO_PE6MAP            FUNCOUTMCON = (23 <<24)
#define CH3_FUNO_PE7MAP            FUNCOUTMCON = (24 <<24)

#define CH3_FUNO_PF0MAP            FUNCOUTMCON = (25 <<24)
#define CH3_FUNO_PF1MAP            FUNCOUTMCON = (26 <<24)
#define CH3_FUNO_PF2MAP            FUNCOUTMCON = (27 <<24)
#define CH3_FUNO_PF3MAP            FUNCOUTMCON = (28 <<24)
#define CH3_FUNO_PF4MAP            FUNCOUTMCON = (29 <<24)
#define CH3_FUNO_PF5MAP            FUNCOUTMCON = (30 <<24)

//channel input function select
#define CH0_FUNI_SEL(ch0_funi_sel) FUNCINCON  = (ch0_funi_sel << 0)
#define CH1_FUNI_SEL(ch1_funi_sel) FUNCINCON  = (ch1_funi_sel << 8)
#define CH2_FUNI_SEL(ch2_funi_sel) FUNCINCON  = (ch2_funi_sel <<16)
#define CH3_FUNI_SEL(ch3_funi_sel) FUNCINCON  = (ch3_funi_sel <<24)

#endif
