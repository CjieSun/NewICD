#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H

#include <stdint.h>

/* ================================================================================ */
/* ================              Core CMSIS Definitions             ============== */
/* ================================================================================ */

#ifdef __cplusplus
  #define   __I     volatile             /*!< Defines 'read only' permissions */
#else
  #define   __I     volatile const       /*!< Defines 'read only' permissions */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions */
#define     __IO    volatile             /*!< Defines 'read / write' permissions */

/* Following defines should be used for structure members */
#define     __IM     volatile const      /*! Defines 'read only' structure member permissions */
#define     __OM     volatile            /*! Defines 'write only' structure member permissions */
#define     __IOM    volatile            /*! Defines 'read / write' structure member permissions */

#ifndef __weak
  #define __weak   __attribute__((weak))
#endif

#ifndef UNUSED
  #define UNUSED(x) ((void)(x))
#endif

/**
 * @brief IC Simulator Device Register Map (CMSIS Style)
 * @version 1.0
 * @date 2025-07-26
 */

/* =============================================== */
/* ============== Memory Map ==================== */
/* =============================================== */
#define PERIPH_BASE           0x40000000UL
#define APB1_BASE             (PERIPH_BASE + 0x00000000UL)
#define APB2_BASE             (PERIPH_BASE + 0x00010000UL)

/* =============================================== */
/* ================ UART ======================== */
/* =============================================== */
#define UART0_BASE            (APB1_BASE + 0x2000UL)
#define UART1_BASE            (APB1_BASE + 0x3000UL)
#define UART2_BASE            (APB1_BASE + 0x4000UL)

/* UART Register Layout */
typedef struct {
    __IO uint32_t DR;        /*!< Data Register,                   Address offset: 0x00 */
    __IO uint32_t RSR_ECR;   /*!< Receive Status Register,         Address offset: 0x04 */
    uint32_t      RESERVED0[4]; /*!< Reserved,                     Address offset: 0x08-0x14 */
    __I  uint32_t FR;        /*!< Flag Register,                   Address offset: 0x18 */
    uint32_t      RESERVED1[1]; /*!< Reserved,                     Address offset: 0x1C */
    __IO uint32_t ILPR;      /*!< IrDA Low Power Register,         Address offset: 0x20 */
    __IO uint32_t IBRD;      /*!< Integer Baud Rate Register,      Address offset: 0x24 */
    __IO uint32_t FBRD;      /*!< Fractional Baud Rate Register,   Address offset: 0x28 */
    __IO uint32_t LCR_H;     /*!< Line Control Register,           Address offset: 0x2C */
    __IO uint32_t CR;        /*!< Control Register,                Address offset: 0x30 */
    __IO uint32_t IFLS;      /*!< Interrupt FIFO Level Select,    Address offset: 0x34 */
    __IO uint32_t IMSC;      /*!< Interrupt Mask Set/Clear,       Address offset: 0x38 */
    __I  uint32_t RIS;       /*!< Raw Interrupt Status,            Address offset: 0x3C */
    __I  uint32_t MIS;       /*!< Masked Interrupt Status,         Address offset: 0x40 */
    __O  uint32_t ICR;       /*!< Interrupt Clear Register,        Address offset: 0x44 */
    __IO uint32_t DMACR;     /*!< DMA Control Register,            Address offset: 0x48 */
} UART_TypeDef;

/* UART Flag Register (FR) */
#define UART_FR_CTS_Pos       (0U)
#define UART_FR_CTS_Msk       (0x1UL << UART_FR_CTS_Pos)
#define UART_FR_CTS           UART_FR_CTS_Msk
#define UART_FR_DSR_Pos       (1U)
#define UART_FR_DSR_Msk       (0x1UL << UART_FR_DSR_Pos)
#define UART_FR_DSR           UART_FR_DSR_Msk
#define UART_FR_DCD_Pos       (2U)
#define UART_FR_DCD_Msk       (0x1UL << UART_FR_DCD_Pos)
#define UART_FR_DCD           UART_FR_DCD_Msk
#define UART_FR_BUSY_Pos      (3U)
#define UART_FR_BUSY_Msk      (0x1UL << UART_FR_BUSY_Pos)
#define UART_FR_BUSY          UART_FR_BUSY_Msk
#define UART_FR_RXFE_Pos      (4U)
#define UART_FR_RXFE_Msk      (0x1UL << UART_FR_RXFE_Pos)
#define UART_FR_RXFE          UART_FR_RXFE_Msk
#define UART_FR_TXFF_Pos      (5U)
#define UART_FR_TXFF_Msk      (0x1UL << UART_FR_TXFF_Pos)
#define UART_FR_TXFF          UART_FR_TXFF_Msk
#define UART_FR_RXFF_Pos      (6U)
#define UART_FR_RXFF_Msk      (0x1UL << UART_FR_RXFF_Pos)
#define UART_FR_RXFF          UART_FR_RXFF_Msk
#define UART_FR_TXFE_Pos      (7U)
#define UART_FR_TXFE_Msk      (0x1UL << UART_FR_TXFE_Pos)
#define UART_FR_TXFE          UART_FR_TXFE_Msk
#define UART_FR_RI_Pos        (8U)
#define UART_FR_RI_Msk        (0x1UL << UART_FR_RI_Pos)
#define UART_FR_RI            UART_FR_RI_Msk

/* UART Control Register (CR) */
#define UART_CR_UARTEN_Pos    (0U)
#define UART_CR_UARTEN_Msk    (0x1UL << UART_CR_UARTEN_Pos)
#define UART_CR_UARTEN        UART_CR_UARTEN_Msk
#define UART_CR_SIREN_Pos     (1U)
#define UART_CR_SIREN_Msk     (0x1UL << UART_CR_SIREN_Pos)
#define UART_CR_SIREN         UART_CR_SIREN_Msk
#define UART_CR_SIRLP_Pos     (2U)
#define UART_CR_SIRLP_Msk     (0x1UL << UART_CR_SIRLP_Pos)
#define UART_CR_SIRLP         UART_CR_SIRLP_Msk
#define UART_CR_LBE_Pos       (7U)
#define UART_CR_LBE_Msk       (0x1UL << UART_CR_LBE_Pos)
#define UART_CR_LBE           UART_CR_LBE_Msk
#define UART_CR_TXE_Pos       (8U)
#define UART_CR_TXE_Msk       (0x1UL << UART_CR_TXE_Pos)
#define UART_CR_TXE           UART_CR_TXE_Msk
#define UART_CR_RXE_Pos       (9U)
#define UART_CR_RXE_Msk       (0x1UL << UART_CR_RXE_Pos)
#define UART_CR_RXE           UART_CR_RXE_Msk
#define UART_CR_DTR_Pos       (10U)
#define UART_CR_DTR_Msk       (0x1UL << UART_CR_DTR_Pos)
#define UART_CR_DTR           UART_CR_DTR_Msk
#define UART_CR_RTS_Pos       (11U)
#define UART_CR_RTS_Msk       (0x1UL << UART_CR_RTS_Pos)
#define UART_CR_RTS           UART_CR_RTS_Msk
#define UART_CR_OUT1_Pos      (12U)
#define UART_CR_OUT1_Msk      (0x1UL << UART_CR_OUT1_Pos)
#define UART_CR_OUT1          UART_CR_OUT1_Msk
#define UART_CR_OUT2_Pos      (13U)
#define UART_CR_OUT2_Msk      (0x1UL << UART_CR_OUT2_Pos)
#define UART_CR_OUT2          UART_CR_OUT2_Msk
#define UART_CR_RTSEN_Pos     (14U)
#define UART_CR_RTSEN_Msk     (0x1UL << UART_CR_RTSEN_Pos)
#define UART_CR_RTSEN         UART_CR_RTSEN_Msk
#define UART_CR_CTSEN_Pos     (15U)
#define UART_CR_CTSEN_Msk     (0x1UL << UART_CR_CTSEN_Pos)
#define UART_CR_CTSEN         UART_CR_CTSEN_Msk

/* UART Line Control Register (LCR_H) */
#define UART_LCR_H_BRK_Pos    (0U)
#define UART_LCR_H_BRK_Msk    (0x1UL << UART_LCR_H_BRK_Pos)
#define UART_LCR_H_BRK        UART_LCR_H_BRK_Msk
#define UART_LCR_H_PEN_Pos    (1U)
#define UART_LCR_H_PEN_Msk    (0x1UL << UART_LCR_H_PEN_Pos)
#define UART_LCR_H_PEN        UART_LCR_H_PEN_Msk
#define UART_LCR_H_EPS_Pos    (2U)
#define UART_LCR_H_EPS_Msk    (0x1UL << UART_LCR_H_EPS_Pos)
#define UART_LCR_H_EPS        UART_LCR_H_EPS_Msk
#define UART_LCR_H_STP2_Pos   (3U)
#define UART_LCR_H_STP2_Msk   (0x1UL << UART_LCR_H_STP2_Pos)
#define UART_LCR_H_STP2       UART_LCR_H_STP2_Msk
#define UART_LCR_H_FEN_Pos    (4U)
#define UART_LCR_H_FEN_Msk    (0x1UL << UART_LCR_H_FEN_Pos)
#define UART_LCR_H_FEN        UART_LCR_H_FEN_Msk
#define UART_LCR_H_WLEN_Pos   (5U)
#define UART_LCR_H_WLEN_Msk   (0x3UL << UART_LCR_H_WLEN_Pos)
#define UART_LCR_H_WLEN       UART_LCR_H_WLEN_Msk
#define UART_LCR_H_SPS_Pos    (7U)
#define UART_LCR_H_SPS_Msk    (0x1UL << UART_LCR_H_SPS_Pos)
#define UART_LCR_H_SPS        UART_LCR_H_SPS_Msk

/* UART DMA Control Register (DMACR) */
#define UART_DMACR_RXDMAE_Pos (0U)
#define UART_DMACR_RXDMAE_Msk (0x1UL << UART_DMACR_RXDMAE_Pos)
#define UART_DMACR_RXDMAE     UART_DMACR_RXDMAE_Msk
#define UART_DMACR_TXDMAE_Pos (1U)
#define UART_DMACR_TXDMAE_Msk (0x1UL << UART_DMACR_TXDMAE_Pos)
#define UART_DMACR_TXDMAE     UART_DMACR_TXDMAE_Msk
#define UART_DMACR_DMAONERR_Pos (2U)
#define UART_DMACR_DMAONERR_Msk (0x1UL << UART_DMACR_DMAONERR_Pos)
#define UART_DMACR_DMAONERR   UART_DMACR_DMAONERR_Msk

/* UART Interrupt Mask Set/Clear Register (IMSC) */
#define UART_IMSC_RIMIM_Pos   (0U)
#define UART_IMSC_RIMIM_Msk   (0x1UL << UART_IMSC_RIMIM_Pos)
#define UART_IMSC_RIMIM       UART_IMSC_RIMIM_Msk
#define UART_IMSC_CTSMIM_Pos  (1U)
#define UART_IMSC_CTSMIM_Msk  (0x1UL << UART_IMSC_CTSMIM_Pos)
#define UART_IMSC_CTSMIM      UART_IMSC_CTSMIM_Msk
#define UART_IMSC_DCDMIM_Pos  (2U)
#define UART_IMSC_DCDMIM_Msk  (0x1UL << UART_IMSC_DCDMIM_Pos)
#define UART_IMSC_DCDMIM      UART_IMSC_DCDMIM_Msk
#define UART_IMSC_DSRMIM_Pos  (3U)
#define UART_IMSC_DSRMIM_Msk  (0x1UL << UART_IMSC_DSRMIM_Pos)
#define UART_IMSC_DSRMIM      UART_IMSC_DSRMIM_Msk
#define UART_IMSC_RXIM_Pos    (4U)
#define UART_IMSC_RXIM_Msk    (0x1UL << UART_IMSC_RXIM_Pos)
#define UART_IMSC_RXIM        UART_IMSC_RXIM_Msk
#define UART_IMSC_TXIM_Pos    (5U)
#define UART_IMSC_TXIM_Msk    (0x1UL << UART_IMSC_TXIM_Pos)
#define UART_IMSC_TXIM        UART_IMSC_TXIM_Msk
#define UART_IMSC_RTIM_Pos    (6U)
#define UART_IMSC_RTIM_Msk    (0x1UL << UART_IMSC_RTIM_Pos)
#define UART_IMSC_RTIM        UART_IMSC_RTIM_Msk
#define UART_IMSC_FEIM_Pos    (7U)
#define UART_IMSC_FEIM_Msk    (0x1UL << UART_IMSC_FEIM_Pos)
#define UART_IMSC_FEIM        UART_IMSC_FEIM_Msk
#define UART_IMSC_PEIM_Pos    (8U)
#define UART_IMSC_PEIM_Msk    (0x1UL << UART_IMSC_PEIM_Pos)
#define UART_IMSC_PEIM        UART_IMSC_PEIM_Msk
#define UART_IMSC_BEIM_Pos    (9U)
#define UART_IMSC_BEIM_Msk    (0x1UL << UART_IMSC_BEIM_Pos)
#define UART_IMSC_BEIM        UART_IMSC_BEIM_Msk
#define UART_IMSC_OEIM_Pos    (10U)
#define UART_IMSC_OEIM_Msk    (0x1UL << UART_IMSC_OEIM_Pos)
#define UART_IMSC_OEIM        UART_IMSC_OEIM_Msk

/* =============================================== */
/* ================ DMA ========================= */
/* =============================================== */
#define DMA0_BASE             (APB1_BASE + 0x6000UL)
#define DMA1_BASE             (APB1_BASE + 0x7000UL)
#define DMA2_BASE             (APB1_BASE + 0x8000UL)

/* DMA Controller Register Layout */
typedef struct {
    __I  uint32_t IntStatus;      /*!< Interrupt Status Register,          Address offset: 0x000 */
    __I  uint32_t IntTCStatus;    /*!< Interrupt Terminal Count Status,    Address offset: 0x004 */
    __O  uint32_t IntTCClear;     /*!< Interrupt Terminal Count Clear,     Address offset: 0x008 */
    __I  uint32_t IntErrorStatus; /*!< Interrupt Error Status Register,    Address offset: 0x00C */
    __O  uint32_t IntErrClr;      /*!< Interrupt Error Clear Register,     Address offset: 0x010 */
    __I  uint32_t RawIntTCStatus; /*!< Raw Interrupt Terminal Count Status, Address offset: 0x014 */
    __I  uint32_t RawIntErrorStatus; /*!< Raw Error Interrupt Status,      Address offset: 0x018 */
    __I  uint32_t EnbldChns;      /*!< Enabled Channel Register,           Address offset: 0x01C */
    __IO uint32_t SoftBReq;       /*!< Software Burst Request Register,    Address offset: 0x020 */
    __IO uint32_t SoftSReq;       /*!< Software Single Request Register,   Address offset: 0x024 */
    __IO uint32_t SoftLBReq;      /*!< Software Last Burst Request,        Address offset: 0x028 */
    __IO uint32_t SoftLSReq;      /*!< Software Last Single Request,       Address offset: 0x02C */
    __IO uint32_t Configuration;  /*!< Configuration Register,             Address offset: 0x030 */
    __IO uint32_t Sync;           /*!< Synchronization Register,           Address offset: 0x034 */
    uint32_t      RESERVED0[50];  /*!< Reserved,                           Address offset: 0x038-0x0FC */
} DMA_TypeDef;

/* DMA Channel Register Layout */
typedef struct {
    __IO uint32_t SrcAddr;        /*!< Channel Source Address Register,     Address offset: 0x00 */
    __IO uint32_t DestAddr;       /*!< Channel Destination Address Register, Address offset: 0x04 */
    __IO uint32_t LLI;            /*!< Channel Linked List Item Register,   Address offset: 0x08 */
    __IO uint32_t Control;        /*!< Channel Control Register,            Address offset: 0x0C */
    __IO uint32_t Configuration;  /*!< Channel Configuration Register,      Address offset: 0x10 */
    uint32_t      RESERVED[3];    /*!< Reserved,                            Address offset: 0x14-0x1C */
} DMA_Channel_TypeDef;

/* DMA instances */
#define DMA0              ((DMA_TypeDef *) DMA0_BASE)
#define DMA1              ((DMA_TypeDef *) DMA1_BASE)
#define DMA2              ((DMA_TypeDef *) DMA2_BASE)

/* DMA Channel base addresses */
#define DMA0_Channel0_BASE (DMA0_BASE + 0x100UL)
#define DMA0_Channel1_BASE (DMA0_BASE + 0x120UL)
#define DMA0_Channel2_BASE (DMA0_BASE + 0x140UL)
#define DMA0_Channel3_BASE (DMA0_BASE + 0x160UL)
#define DMA0_Channel4_BASE (DMA0_BASE + 0x180UL)
#define DMA0_Channel5_BASE (DMA0_BASE + 0x1A0UL)
#define DMA0_Channel6_BASE (DMA0_BASE + 0x1C0UL)
#define DMA0_Channel7_BASE (DMA0_BASE + 0x1E0UL)

/* DMA Channel instances */
#define DMA0_Channel0     ((DMA_Channel_TypeDef *) DMA0_Channel0_BASE)
#define DMA0_Channel1     ((DMA_Channel_TypeDef *) DMA0_Channel1_BASE)
#define DMA0_Channel2     ((DMA_Channel_TypeDef *) DMA0_Channel2_BASE)
#define DMA0_Channel3     ((DMA_Channel_TypeDef *) DMA0_Channel3_BASE)
#define DMA0_Channel4     ((DMA_Channel_TypeDef *) DMA0_Channel4_BASE)
#define DMA0_Channel5     ((DMA_Channel_TypeDef *) DMA0_Channel5_BASE)
#define DMA0_Channel6     ((DMA_Channel_TypeDef *) DMA0_Channel6_BASE)
#define DMA0_Channel7     ((DMA_Channel_TypeDef *) DMA0_Channel7_BASE)

/* =============================================== */
/* ============ Peripheral Instances ============ */
/* =============================================== */
#define UART0             ((UART_TypeDef *) UART0_BASE)
#define UART1             ((UART_TypeDef *) UART1_BASE)
#define UART2             ((UART_TypeDef *) UART2_BASE)

/* =============================================== */
/* ============ CMSIS Core Definitions ========== */
/* =============================================== */
#ifndef __IO
#define __IO              volatile
#endif
#ifndef __O
#define __O               volatile
#endif
#ifndef __I
#define __I               volatile const
#endif

/* Legacy support for backward compatibility */
#define UART_BASE           UART0_BASE
#define UART_TX_REG         (UART_BASE + 0x00)
#define UART_RX_REG         (UART_BASE + 0x00)  /* Same as DR register */
#define UART_STATUS_REG     (UART_BASE + 0x18)  /* Same as FR register */
#define UART_CTRL_REG       (UART_BASE + 0x30)  /* Same as CR register */
#define UART_DMA_CTRL_REG   (UART_BASE + 0x48)  /* Same as DMACR register */

/* Legacy status bits */
#define UART_TX_READY       (~UART_FR_TXFF)     /* TX not full */
#define UART_RX_READY       (~UART_FR_RXFE)     /* RX not empty */

/* Legacy DMA control bits */
#define UART_DMA_TX_ENABLE  UART_DMACR_TXDMAE
#define UART_DMA_RX_ENABLE  UART_DMACR_RXDMAE

/* DMA legacy definitions */
#define DMA_BASE_ADDR           DMA0_BASE
#define DMA_GLOBAL_CTRL_REG     (DMA_BASE_ADDR + 0x30)  /* Configuration register */
#define DMA_GLOBAL_STATUS_REG   (DMA_BASE_ADDR + 0x00)  /* IntStatus register */
#define DMA_INT_STATUS_REG      (DMA_BASE_ADDR + 0x00)  /* IntStatus register */
#define DMA_INT_CLEAR_REG       (DMA_BASE_ADDR + 0x008) /* IntTCClear register */
#define DMA_MAX_CHANNELS        8
#define DMA_CH_OFFSET           0x20
#define DMA_CH_BASE_ADDR        (DMA_BASE_ADDR + 0x100)  /* Channel registers base */
#define DMA_CH_CTRL_REG(ch)     (DMA_CH_BASE_ADDR + (ch) * DMA_CH_OFFSET + 0x0C)
#define DMA_CH_STATUS_REG(ch)   (DMA_CH_BASE_ADDR + (ch) * DMA_CH_OFFSET + 0x10)
#define DMA_CH_SRC_REG(ch)      (DMA_CH_BASE_ADDR + (ch) * DMA_CH_OFFSET + 0x00)
#define DMA_CH_DST_REG(ch)      (DMA_CH_BASE_ADDR + (ch) * DMA_CH_OFFSET + 0x04)
#define DMA_CH_SIZE_REG(ch)     (DMA_CH_BASE_ADDR + (ch) * DMA_CH_OFFSET + 0x0C)
#define DMA_CH_CONFIG_REG(ch)   (DMA_CH_BASE_ADDR + (ch) * DMA_CH_OFFSET + 0x10)
#define DMA_CH_CURRENT_SRC_REG(ch) (DMA_CH_BASE_ADDR + (ch) * DMA_CH_OFFSET + 0x00)
#define DMA_CH_CURRENT_DST_REG(ch) (DMA_CH_BASE_ADDR + (ch) * DMA_CH_OFFSET + 0x04)

/* Legacy DMA channel structure */
typedef struct {
    uint32_t ctrl;
    uint32_t status;
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t size;
    uint32_t config;
    uint32_t current_src;
    uint32_t current_dst;
} dma_channel_regs_t;

/* ================================================================================ */
/* ================                HAL Status Types                 ============== */
/* ================================================================================ */

/**
 * @brief  HAL Status structures definition
 */
typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

/**
 * @brief  HAL Lock structures definition
 */
typedef enum {
    HAL_UNLOCKED = 0x00U,
    HAL_LOCKED   = 0x01U
} HAL_LockTypeDef;

/* Maximum delay for HAL operations */
#define HAL_MAX_DELAY      0xFFFFFFFFU

#endif // REGISTER_MAP_H
