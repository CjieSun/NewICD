/**
 ******************************************************************************
 * @file    uart_driver.h
 * @author  IC Simulator Team
 * @brief   UART HAL Driver Header File (CMSIS Style)
 * @version V1.0.0
 * @date    26-July-2025
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 IC Simulator Project.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "../common/register_map.h"

/** @addtogroup IC_Simulator_Driver
  * @{
  */

/** @addtogroup UART
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup UART_Exported_Types UART Exported Types
  * @{
  */

/**
  * @brief  UART State Definition
  */
typedef enum {
    HAL_UART_STATE_RESET             = 0x00U,    /*!< Peripheral is not initialized   */
    HAL_UART_STATE_READY             = 0x20U,    /*!< Peripheral Initialized and ready for use */
    HAL_UART_STATE_BUSY              = 0x24U,    /*!< An internal process is ongoing  */
    HAL_UART_STATE_BUSY_TX           = 0x21U,    /*!< Data Transmission process is ongoing */
    HAL_UART_STATE_BUSY_RX           = 0x22U,    /*!< Data Reception process is ongoing */
    HAL_UART_STATE_BUSY_TX_RX        = 0x23U,    /*!< Data Transmission and Reception process is ongoing */
    HAL_UART_STATE_TIMEOUT           = 0xA0U,    /*!< Timeout state */
    HAL_UART_STATE_ERROR             = 0xE0U     /*!< Error state */
} HAL_UART_StateTypeDef;

/**
  * @brief  UART Error Definition
  */
typedef enum {
    HAL_UART_ERROR_NONE              = 0x00000000U,   /*!< No error            */
    HAL_UART_ERROR_PE                = 0x00000001U,   /*!< Parity error        */
    HAL_UART_ERROR_FE                = 0x00000002U,   /*!< Frame error         */
    HAL_UART_ERROR_OE                = 0x00000004U,   /*!< Overrun error       */
    HAL_UART_ERROR_DMA               = 0x00000008U,   /*!< DMA transfer error  */
    HAL_UART_ERROR_BUSY              = 0x00000010U,   /*!< Busy Error          */
    HAL_UART_ERROR_TIMEOUT           = 0x00000020U    /*!< Timeout Error       */
} HAL_UART_ErrorTypeDef;

/**
  * @brief  UART Word Length Definition
  */
typedef enum {
    UART_WORDLENGTH_5B               = 0x00000000U,   /*!< 5-bit long UART frame */
    UART_WORDLENGTH_6B               = 0x00000020U,   /*!< 6-bit long UART frame */
    UART_WORDLENGTH_7B               = 0x00000040U,   /*!< 7-bit long UART frame */
    UART_WORDLENGTH_8B               = 0x00000060U    /*!< 8-bit long UART frame */
} UART_WordLengthTypeDef;

/**
  * @brief  UART Stop Bits Definition
  */
typedef enum {
    UART_STOPBITS_1                  = 0x00000000U,   /*!< UART frame with 1 stop bit    */
    UART_STOPBITS_2                  = 0x00000008U    /*!< UART frame with 2 stop bits   */
} UART_StopBitsTypeDef;

/**
  * @brief  UART Parity Definition
  */
typedef enum {
    UART_PARITY_NONE                 = 0x00000000U,   /*!< No parity   */
    UART_PARITY_EVEN                 = 0x00000006U,   /*!< Even parity */
    UART_PARITY_ODD                  = 0x00000002U    /*!< Odd parity  */
} UART_ParityTypeDef;

/**
  * @brief  UART Mode Definition
  */
typedef enum {
    UART_MODE_RX                     = 0x00000200U,   /*!< RX mode        */
    UART_MODE_TX                     = 0x00000100U,   /*!< TX mode        */
    UART_MODE_TX_RX                  = 0x00000300U    /*!< RX and TX mode */
} UART_ModeTypeDef;

/**
  * @brief  UART Hardware Flow Control Definition
  */
typedef enum {
    UART_HWCONTROL_NONE              = 0x00000000U,   /*!< No hardware control       */
    UART_HWCONTROL_RTS               = 0x00004000U,   /*!< RTS hardware control      */
    UART_HWCONTROL_CTS               = 0x00008000U,   /*!< CTS hardware control      */
    UART_HWCONTROL_RTS_CTS           = 0x0000C000U    /*!< RTS and CTS hardware control */
} UART_HwFlowCtlTypeDef;

/**
  * @brief  UART Transfer Mode Definition
  */
typedef enum {
    UART_TRANSFER_MODE_POLLING       = 0x00U,         /*!< Polling mode     */
    UART_TRANSFER_MODE_INTERRUPT     = 0x01U,         /*!< Interrupt mode   */
    UART_TRANSFER_MODE_DMA           = 0x02U          /*!< DMA mode         */
} UART_TransferModeTypeDef;

/**
  * @brief  UART Configuration Structure definition
  */
typedef struct {
    uint32_t                    BaudRate;           /*!< This member configures the UART communication baud rate. */

    UART_WordLengthTypeDef      WordLength;         /*!< Specifies the number of data bits transmitted or received in a frame. */

    UART_StopBitsTypeDef        StopBits;           /*!< Specifies the number of stop bits transmitted. */

    UART_ParityTypeDef          Parity;             /*!< Specifies the parity mode. */

    UART_ModeTypeDef            Mode;               /*!< Specifies whether the Receive or Transmit mode is enabled or disabled. */

    UART_HwFlowCtlTypeDef       HwFlowCtl;          /*!< Specifies whether the hardware flow control mode is enabled or disabled. */

    UART_TransferModeTypeDef    TransferMode;       /*!< Specifies the transfer mode (Polling, Interrupt, DMA). */

} UART_InitTypeDef;

/**
  * @brief  UART Handle Structure definition
  */
typedef struct __UART_HandleTypeDef {
    UART_TypeDef                     *Instance;        /*!< UART registers base address        */

    UART_InitTypeDef                 Init;             /*!< UART communication parameters      */

    const uint8_t                    *pTxBuffPtr;      /*!< Pointer to UART Tx transfer Buffer */

    uint16_t                         TxXferSize;       /*!< UART Tx Transfer size              */

    __IO uint16_t                    TxXferCount;      /*!< UART Tx Transfer Counter           */

    uint8_t                          *pRxBuffPtr;      /*!< Pointer to UART Rx transfer Buffer */

    uint16_t                         RxXferSize;       /*!< UART Rx Transfer size              */

    __IO uint16_t                    RxXferCount;      /*!< UART Rx Transfer Counter           */

    uint16_t                         Mask;             /*!< UART Rx RDR register mask          */

    __IO HAL_UART_StateTypeDef       gState;           /*!< UART state information related to global Handle management 
                                                            and also related to Tx operations. */

    __IO HAL_UART_StateTypeDef       RxState;          /*!< UART state information related to Rx operations. */

    __IO uint32_t                    ErrorCode;        /*!< UART Error code                    */

    /* DMA related members */
    void                             *hdmatx;          /*!< UART Tx DMA Handle parameters      */

    void                             *hdmarx;          /*!< UART Rx DMA Handle parameters      */

    int8_t                           TxDmaChannel;     /*!< UART Tx DMA Channel number         */

    int8_t                           RxDmaChannel;     /*!< UART Rx DMA Channel number         */

    volatile bool                    TxCompleted;      /*!< UART Tx completion flag            */

    volatile bool                    RxCompleted;      /*!< UART Rx completion flag            */

} UART_HandleTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup UART_Exported_Constants UART Exported Constants
  * @{
  */

/** @defgroup UART_Interrupt_definition UART Interrupts Definition
  * @{
  */
#define UART_IT_PE                   UART_IMSC_PEIM
#define UART_IT_FE                   UART_IMSC_FEIM
#define UART_IT_OE                   UART_IMSC_OEIM
#define UART_IT_BE                   UART_IMSC_BEIM
#define UART_IT_RT                   UART_IMSC_RTIM
#define UART_IT_TX                   UART_IMSC_TXIM
#define UART_IT_RX                   UART_IMSC_RXIM
#define UART_IT_CTS                  UART_IMSC_CTSMIM
/**
  * @}
  */

/** @defgroup UART_FLAG_definition UART Flags Definition
  * @{
  */
#define UART_FLAG_CTS                UART_FR_CTS
#define UART_FLAG_DSR                UART_FR_DSR
#define UART_FLAG_DCD                UART_FR_DCD
#define UART_FLAG_BUSY               UART_FR_BUSY
#define UART_FLAG_RXFE               UART_FR_RXFE
#define UART_FLAG_TXFF               UART_FR_TXFF
#define UART_FLAG_RXFF               UART_FR_RXFF
#define UART_FLAG_TXFE               UART_FR_TXFE
#define UART_FLAG_RI                 UART_FR_RI
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/

/** @defgroup UART_Exported_Macros UART Exported Macros
  * @{
  */

/** @brief Reset UART handle gstate & RxState
  * @param  __HANDLE__ specifies the UART Handle.
  * @retval None
  */
#define __HAL_UART_RESET_HANDLE_STATE(__HANDLE__)  do{                                                   \
                                                        (__HANDLE__)->gState = HAL_UART_STATE_RESET;      \
                                                        (__HANDLE__)->RxState = HAL_UART_STATE_RESET;     \
                                                      } while(0U)

/** @brief  Flush the UART Data registers.
  * @param  __HANDLE__ specifies the UART Handle.
  * @retval None
  */
#define __HAL_UART_FLUSH_DRREGISTER(__HANDLE__)    do{                                                   \
                                                        while(((__HANDLE__)->Instance->FR & UART_FR_RXFE) == 0U) \
                                                        {                                                 \
                                                          (void)((__HANDLE__)->Instance->DR);           \
                                                        }                                                 \
                                                      } while(0U)

/** @brief  Check whether the specified UART flag is set or not.
  * @param  __HANDLE__ specifies the UART Handle.
  * @param  __FLAG__ specifies the flag to check.
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_UART_GET_FLAG(__HANDLE__, __FLAG__) (((__HANDLE__)->Instance->FR & (__FLAG__)) == (__FLAG__))

/** @brief  Clear the specified UART pending flag.
  * @param  __HANDLE__ specifies the UART Handle.
  * @param  __FLAG__ specifies the flag to check.
  * @retval None
  */
#define __HAL_UART_CLEAR_FLAG(__HANDLE__, __FLAG__) ((__HANDLE__)->Instance->ICR = (__FLAG__))

/** @brief  Enable the specified UART interrupt.
  * @param  __HANDLE__ specifies the UART Handle.
  * @param  __INTERRUPT__ specifies the UART interrupt source to enable.
  * @retval None
  */
#define __HAL_UART_ENABLE_IT(__HANDLE__, __INTERRUPT__)   ((__HANDLE__)->Instance->IMSC |= (__INTERRUPT__))

/** @brief  Disable the specified UART interrupt.
  * @param  __HANDLE__ specifies the UART Handle.
  * @param  __INTERRUPT__ specifies the UART interrupt source to disable.
  * @retval None
  */
#define __HAL_UART_DISABLE_IT(__HANDLE__, __INTERRUPT__)  ((__HANDLE__)->Instance->IMSC &= ~(__INTERRUPT__))

/** @brief  Check whether the specified UART interrupt has occurred or not.
  * @param  __HANDLE__ specifies the UART Handle.
  * @param  __INTERRUPT__ specifies the UART interrupt to check.
  * @retval The new state of __INTERRUPT__ (TRUE or FALSE).
  */
#define __HAL_UART_GET_IT(__HANDLE__, __INTERRUPT__) (((__HANDLE__)->Instance->MIS & (__INTERRUPT__)) == (__INTERRUPT__))

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @addtogroup UART_Exported_Functions
  * @{
  */

/** @addtogroup UART_Exported_Functions_Group1
  * @{
  */

/* Initialization and de-initialization functions ****************************/
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart);
void HAL_UART_MspInit(UART_HandleTypeDef *huart);
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart);

/**
  * @}
  */

/** @addtogroup UART_Exported_Functions_Group2
  * @{
  */

/* IO operation functions *****************************************************/
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_DMAPause(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_DMAResume(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef *huart);

/* Transfer Abort functions */
HAL_StatusTypeDef HAL_UART_Abort(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Abort_IT(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortTransmit_IT(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortReceive_IT(UART_HandleTypeDef *huart);

void HAL_UART_IRQHandler(UART_HandleTypeDef *huart);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart);

/**
  * @}
  */

/** @addtogroup UART_Exported_Functions_Group3
  * @{
  */

/* Peripheral State and Error functions **************************************/
HAL_UART_StateTypeDef HAL_UART_GetState(UART_HandleTypeDef *huart);
uint32_t              HAL_UART_GetError(UART_HandleTypeDef *huart);

/**
  * @}
  */

/** @addtogroup UART_Exported_Functions_Group4
  * @{
  */

/* Legacy functions for backward compatibility *******************************/
int uart_init(void);
void uart_cleanup(void);
int uart_send_byte(uint8_t data);
int uart_receive_byte(uint8_t *data);
int uart_send_string(const char *str);
int uart_dma_init(void);
void uart_dma_cleanup(void);
int uart_dma_send(const uint8_t *data, uint32_t size);
int uart_dma_receive(uint8_t *buffer, uint32_t size);
bool uart_dma_send_completed(void);
bool uart_dma_receive_completed(void);
int uart_dma_wait_send_complete(uint32_t timeout_ms);
int uart_dma_wait_receive_complete(uint32_t timeout_ms);
int uart_set_mode(UART_TransferModeTypeDef mode);
UART_TransferModeTypeDef uart_get_mode(void);
void uart_tx_interrupt_handler(void);
void uart_rx_interrupt_handler(void);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif // UART_DRIVER_H
