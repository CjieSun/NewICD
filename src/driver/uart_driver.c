/**
 ******************************************************************************
 * @file    uart_driver.c
 * @author  IC Simulator Team
 * @brief   UART HAL Driver Source File (CMSIS Style)
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

/* Includes ------------------------------------------------------------------*/
#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "uart_driver.h"
#include "../common/register_map.h"
#include "../sim_interface/interrupt_manager.h"
#include "dma_driver.h"
#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
    #define usleep(us) Sleep((us)/1000)
#else
    #include <unistd.h>
#endif

/** @addtogroup IC_Simulator_Driver
  * @{
  */

/** @defgroup UART UART
  * @brief UART HAL module driver
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/** @defgroup UART_Private_Constants UART Private Constants
  * @{
  */
#define UART_TIMEOUT_VALUE                1000U          /* 1 second timeout */
#define UART_FIFO_SIZE                    16U            /* UART FIFO depth */

/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** @defgroup UART_Private_Variables UART Private Variables
  * @{
  */

/* Global UART handle for legacy support */
static UART_HandleTypeDef g_UartHandle = {0};

/* Legacy compatibility variables */
static volatile int uart_tx_complete = 0;
static volatile int uart_rx_available = 0;
static UART_TransferModeTypeDef g_uart_mode = UART_TRANSFER_MODE_POLLING;

/* DMA transfer state structures for legacy support */
typedef struct {
    uint8_t *buffer;        /*!< Data buffer pointer */
    uint32_t size;          /*!< Transfer size */
    bool completed;         /*!< Transfer completion flag */
    int8_t dma_channel;     /*!< DMA channel number (-1 if not allocated) */
} uart_dma_transfer_t;

static uart_dma_transfer_t g_uart_dma_tx = {0};
static uart_dma_transfer_t g_uart_dma_rx = {0};
static bool g_uart_dma_initialized = false;

/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/

/** @defgroup UART_Private_Functions UART Private Functions
  * @{
  */

static HAL_StatusTypeDef UART_SetConfig(UART_HandleTypeDef *huart);
static HAL_StatusTypeDef UART_CheckIdleState(UART_HandleTypeDef *huart);
static void UART_DMATransmitCplt(UART_HandleTypeDef *huart);
static void UART_DMAReceiveCplt(UART_HandleTypeDef *huart);
static void UART_DMAError(UART_HandleTypeDef *huart);
static uint32_t HAL_GetTick(void);

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup UART_Exported_Functions UART Exported Functions
  * @{
  */

/** @defgroup UART_Exported_Functions_Group1 Initialization and de-initialization functions
  * @brief    Initialization and Configuration functions
  * @{
  */

/**
  * @brief  Initialize the UART according to the specified parameters
  *         in the UART_InitTypeDef and initialize the associated handle.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart)
{
    /* Check the UART handle allocation */
    if (huart == NULL) {
        return HAL_ERROR;
    }

    /* Check the parameters */
    if (huart->Instance == NULL) {
        return HAL_ERROR;
    }

    if (huart->gState == HAL_UART_STATE_RESET) {
        /* Allocate lock resource and initialize it */
        huart->gState = HAL_UART_STATE_BUSY;

        /* Init the low level hardware */
        HAL_UART_MspInit(huart);
    }

    huart->gState = HAL_UART_STATE_BUSY;

    /* Disable the UART */
    huart->Instance->CR &= ~UART_CR_UARTEN;

    /* Set the UART Communication parameters */
    if (UART_SetConfig(huart) != HAL_OK) {
        return HAL_ERROR;
    }

    /* In simulation mode, skip complex configuration */
    printf("[%s:%s] UART HAL initialization for instance 0x%08X\n", 
           __FILE__, __func__, (uint32_t)huart->Instance);

    /* Initialize the UART state */
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->gState = HAL_UART_STATE_READY;
    huart->RxState = HAL_UART_STATE_READY;

    return HAL_OK;
}

/**
  * @brief  DeInitialize the UART peripheral.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart)
{
    /* Check the UART handle allocation */
    if (huart == NULL) {
        return HAL_ERROR;
    }

    /* Check the parameters */
    if (huart->Instance == NULL) {
        return HAL_ERROR;
    }

    huart->gState = HAL_UART_STATE_BUSY;

    /* Disable the UART */
    huart->Instance->CR &= ~UART_CR_UARTEN;

    /* DeInit the low level hardware */
    HAL_UART_MspDeInit(huart);

    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->gState = HAL_UART_STATE_RESET;
    huart->RxState = HAL_UART_STATE_RESET;

    printf("[%s:%s] UART HAL deinitialization completed\n", __FILE__, __func__);

    return HAL_OK;
}

/**
  * @brief  Initialize the UART MSP.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @retval None
  */
__weak void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_MspInit could be implemented in the user file
     */
    printf("[%s:%s] UART MSP initialization (simulation mode)\n", __FILE__, __func__);
}

/**
  * @brief  DeInitialize the UART MSP.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @retval None
  */
__weak void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_MspDeInit could be implemented in the user file
     */
    printf("[%s:%s] UART MSP deinitialization (simulation mode)\n", __FILE__, __func__);
}

/**
  * @}
  */

/** @defgroup UART_Exported_Functions_Group2 IO operation functions
  * @brief UART Transmit and Receive functions
  * @{
  */

/**
  * @brief  Send an amount of data in blocking mode.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @param  pData Pointer to data buffer (u8 or u16 data elements).
  * @param  Size  Amount of data elements (u8 or u16) to be sent
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    const uint8_t *pdata8bits;
    uint32_t tickstart;

    /* Check that a Tx process is not already ongoing */
    if (huart->gState != HAL_UART_STATE_READY) {
        return HAL_BUSY;
    }

    if ((pData == NULL) || (Size == 0U)) {
        return HAL_ERROR;
    }

    /* Process Locked */
    huart->gState = HAL_UART_STATE_BUSY_TX;

    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->TxXferSize = Size;
    huart->TxXferCount = Size;

    /* Initialize the timeout */
    tickstart = HAL_GetTick();

    pdata8bits = pData;

    /* Process Unlocked */
    while (huart->TxXferCount > 0U) {
        /* Wait for TXE flag to be raised */
        if (__HAL_UART_GET_FLAG(huart, UART_FLAG_TXFE) == 0U) {
            /* Write data to Transmit Data register */
            huart->Instance->DR = (uint8_t)(*pdata8bits & 0xFFU);
            pdata8bits++;
            huart->TxXferCount--;
        }

        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if (((HAL_GetTick() - tickstart) > Timeout) || (Timeout == 0U)) {
                huart->gState = HAL_UART_STATE_READY;
                return HAL_TIMEOUT;
            }
        }

        /* Simulation: Add small delay to prevent tight loop */
        usleep(100);
    }

    /* At end of Tx process, restore huart->gState to Ready */
    huart->gState = HAL_UART_STATE_READY;

    return HAL_OK;
}

/**
  * @brief  Receive an amount of data in blocking mode.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @param  pData Pointer to data buffer (u8 or u16 data elements).
  * @param  Size  Amount of data elements (u8 or u16) to be received.
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    uint8_t *pdata8bits;
    uint32_t tickstart;

    /* Check that a Rx process is not already ongoing */
    if (huart->RxState != HAL_UART_STATE_READY) {
        return HAL_BUSY;
    }

    if ((pData == NULL) || (Size == 0U)) {
        return HAL_ERROR;
    }

    /* Process Locked */
    huart->RxState = HAL_UART_STATE_BUSY_RX;

    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->RxXferSize = Size;
    huart->RxXferCount = Size;

    /* Initialize the timeout */
    tickstart = HAL_GetTick();

    pdata8bits = pData;

    /* Process Unlocked */
    while (huart->RxXferCount > 0U) {
        /* Wait for RXNE flag to be raised */
        if (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXFE) == 0U) {
            /* Read data from Receive Data register */
            *pdata8bits = (uint8_t)(huart->Instance->DR & 0xFFU);
            pdata8bits++;
            huart->RxXferCount--;
        }

        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if (((HAL_GetTick() - tickstart) > Timeout) || (Timeout == 0U)) {
                huart->RxState = HAL_UART_STATE_READY;
                return HAL_TIMEOUT;
            }
        }

        /* Simulation: Add delay to allow interrupt-driven data arrival */
        usleep(1000);
    }

    /* At end of Rx process, restore huart->RxState to Ready */
    huart->RxState = HAL_UART_STATE_READY;

    return HAL_OK;
}

/**
  * @brief  Return the UART handle state.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART.
  * @retval HAL state
  */
HAL_UART_StateTypeDef HAL_UART_GetState(UART_HandleTypeDef *huart)
{
    uint32_t temp1 = 0x00U, temp2 = 0x00U;
    temp1 = huart->gState;
    temp2 = huart->RxState;

    return (HAL_UART_StateTypeDef)(temp1 | temp2);
}

/**
  * @brief  Return the UART handle error code.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART.
  * @retval UART Error Code
  */
uint32_t HAL_UART_GetError(UART_HandleTypeDef *huart)
{
    return huart->ErrorCode;
}

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/

/** @defgroup UART_Private_Functions UART Private Functions
  * @{
  */

/**
  * @brief  Configure the UART peripheral.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_SetConfig(UART_HandleTypeDef *huart)
{
    uint32_t tmpreg;

    /* Check the parameters */
    if (huart->Instance == NULL) {
        return HAL_ERROR;
    }

    /*-------------------------- UART LCR_H Configuration -----------------------*/
    tmpreg = huart->Init.WordLength | huart->Init.Parity | huart->Init.StopBits;

    /* Enable FIFO */
    tmpreg |= UART_LCR_H_FEN;

    /* Write to UART LCR_H */
    huart->Instance->LCR_H = tmpreg;

    /*-------------------------- UART CR Configuration -----------------------*/
    tmpreg = huart->Init.Mode | huart->Init.HwFlowCtl;

    /* Enable UART */
    tmpreg |= UART_CR_UARTEN;

    /* Write to UART CR */
    huart->Instance->CR = tmpreg;

    return HAL_OK;
}

/**
  * @brief  Check the UART Idle State.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *               the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_CheckIdleState(UART_HandleTypeDef *huart)
{
    uint32_t tickstart;

    /* Initialize the timeout */
    tickstart = HAL_GetTick();

    /* Check if the Transmitter is enabled */
    if ((huart->Instance->CR & UART_CR_TXE) == UART_CR_TXE) {
        /* Wait until TEACK flag is set */
        while (__HAL_UART_GET_FLAG(huart, UART_FLAG_BUSY) != 0U) {
            /* Check for the Timeout */
            if ((HAL_GetTick() - tickstart) > UART_TIMEOUT_VALUE) {
                /* Set the UART state ready to be able to start again the process */
                huart->gState = HAL_UART_STATE_READY;
                huart->RxState = HAL_UART_STATE_READY;

                return HAL_TIMEOUT;
            }
        }
    }

    /* Initialize the UART state */
    huart->gState = HAL_UART_STATE_READY;
    huart->RxState = HAL_UART_STATE_READY;

    return HAL_OK;
}

/**
  * @}
  */

/* ============================================================================ */
/* ===================== LEGACY COMPATIBILITY FUNCTIONS ===================== */
/* ============================================================================ */

/** @defgroup UART_Legacy_Functions UART Legacy Functions
  * @brief Legacy functions for backward compatibility
  * @{
  */

/* UART register access macros for legacy support */
#define UART_TX_REG_PTR         ((volatile uint32_t*)UART_TX_REG)
#define UART_RX_REG_PTR         ((volatile uint32_t*)UART_RX_REG)
#define UART_STATUS_REG_PTR     ((volatile uint32_t*)UART_STATUS_REG)
#define UART_CTRL_REG_PTR       ((volatile uint32_t*)UART_CTRL_REG)
#define UART_DMA_CTRL_REG_PTR   ((volatile uint32_t*)UART_DMA_CTRL_REG)

/* HAL_GetTick simulation for timeout handling */
#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY           0xFFFFFFFFU
#endif

#ifndef UNUSED
#define UNUSED(X)               (void)X
#endif

/* Simulation of HAL_GetTick function */
static uint32_t HAL_GetTick(void)
{
    static uint32_t tick_counter = 0;
    tick_counter += 1;  /* Simplified tick simulation */
    return tick_counter;
}

/**
  * @brief  Legacy UART TX interrupt handler
  * @retval None
  */
void uart_tx_interrupt_handler(void)
{
    printf("[%s:%s] UART TX interrupt received.\n", __FILE__, __func__);
    uart_tx_complete = 1;
    
    /* Call HAL callback if handle is available */
    if (g_UartHandle.Instance != NULL) {
        HAL_UART_TxCpltCallback(&g_UartHandle);
    }
}

/**
  * @brief  Legacy UART RX interrupt handler
  * @retval None
  */
void uart_rx_interrupt_handler(void)
{
    printf("[%s:%s] UART RX interrupt received.\n", __FILE__, __func__);
    uart_rx_available = 1;
    
    /* Call HAL callback if handle is available */
    if (g_UartHandle.Instance != NULL) {
        HAL_UART_RxCpltCallback(&g_UartHandle);
    }
}

/**
  * @brief  DMA传输完成回调函数
  * @param  channel DMA通道号
  * @param  status 传输状态
  * @retval None
  */
static void uart_dma_tx_callback(uint8_t channel, dma_channel_status_t status) {
    printf("[%s:%s] UART DMA TX callback, channel=%d, status=%d\n", 
           __FILE__, __func__, channel, status);
    
    if (status == DMA_CH_DONE) {
        g_uart_dma_tx.completed = true;
        /* 在仿真模式中跳过寄存器访问 */
        printf("[%s:%s] Simulation mode: skipping UART DMA control register access\n", __FILE__, __func__);
        
        /* Call HAL callback */
        if (g_UartHandle.Instance != NULL) {
            HAL_UART_TxCpltCallback(&g_UartHandle);
        }
    } else if (status == DMA_CH_ERROR) {
        printf("[%s:%s] UART DMA TX error\n", __FILE__, __func__);
        g_uart_dma_tx.completed = true;
        
        /* Call HAL error callback */
        if (g_UartHandle.Instance != NULL) {
            g_UartHandle.ErrorCode |= HAL_UART_ERROR_DMA;
            HAL_UART_ErrorCallback(&g_UartHandle);
        }
    }
}

/**
  * @brief  DMA接收完成回调函数
  * @param  channel DMA通道号
  * @param  status 传输状态
  * @retval None
  */
static void uart_dma_rx_callback(uint8_t channel, dma_channel_status_t status) {
    printf("[%s:%s] UART DMA RX callback, channel=%d, status=%d\n", 
           __FILE__, __func__, channel, status);
    
    if (status == DMA_CH_DONE) {
        g_uart_dma_rx.completed = true;
        /* 禁用UART DMA接收 */
        *UART_DMA_CTRL_REG_PTR &= ~UART_DMA_RX_ENABLE;
        
        /* Call HAL callback */
        if (g_UartHandle.Instance != NULL) {
            HAL_UART_RxCpltCallback(&g_UartHandle);
        }
    } else if (status == DMA_CH_ERROR) {
        printf("[%s:%s] UART DMA RX error\n", __FILE__, __func__);
        g_uart_dma_rx.completed = true;
        
        /* Call HAL error callback */
        if (g_UartHandle.Instance != NULL) {
            g_UartHandle.ErrorCode |= HAL_UART_ERROR_DMA;
            HAL_UART_ErrorCallback(&g_UartHandle);
        }
    }
}

/**
  * @brief  Legacy UART initialization
  * @retval 0 on success, -1 on error
  */
int uart_init(void)
{
    printf("[%s:%s] UART driver initializing...\n", __FILE__, __func__);

    /* Initialize global handle for UART0 */
    g_UartHandle.Instance = UART0;
    g_UartHandle.Init.BaudRate = 115200;
    g_UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    g_UartHandle.Init.StopBits = UART_STOPBITS_1;
    g_UartHandle.Init.Parity = UART_PARITY_NONE;
    g_UartHandle.Init.Mode = UART_MODE_TX_RX;
    g_UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_UartHandle.Init.TransferMode = UART_TRANSFER_MODE_POLLING;

    /* Initialize HAL UART */
    if (HAL_UART_Init(&g_UartHandle) != HAL_OK) {
        printf("[%s:%s] HAL UART initialization failed\n", __FILE__, __func__);
        return -1;
    }

    /* 注册UART中断处理函数 */
    if (register_interrupt_handler(5, uart_tx_interrupt_handler) != 0) {
        printf("[%s:%s] Failed to register TX interrupt handler\n", __FILE__, __func__);
        return -1;
    }
    
    if (register_interrupt_handler(6, uart_rx_interrupt_handler) != 0) {
        printf("[%s:%s] Failed to register RX interrupt handler\n", __FILE__, __func__);
        return -1;
    }

    /* 在仿真模式中跳过控制寄存器访问 */
    printf("[%s:%s] Simulation mode: skipping UART control register access\n", __FILE__, __func__);
    
    /* 初始化DMA传输状态 */
    g_uart_dma_tx.dma_channel = -1;
    g_uart_dma_tx.completed = true;
    g_uart_dma_rx.dma_channel = -1;
    g_uart_dma_rx.completed = true;
    g_uart_mode = UART_TRANSFER_MODE_POLLING;
    
    printf("[%s:%s] UART driver initialized\n", __FILE__, __func__);
    return 0;
}

/**
  * @brief  Legacy UART cleanup
  * @retval None
  */
void uart_cleanup(void)
{
    /* 禁用UART */
    *UART_CTRL_REG_PTR = 0x00;
    
    /* 清理DMA */
    uart_dma_cleanup();
    
    /* DeInitialize HAL UART */
    HAL_UART_DeInit(&g_UartHandle);
    
    printf("[%s:%s] UART driver cleanup completed\n", __FILE__, __func__);
}

/**
  * @brief  Legacy UART DMA initialization
  * @retval 0 on success, -1 on error
  */
int uart_dma_init(void)
{
    printf("[%s:%s] UART DMA initializing (simulation mode)...\n", __FILE__, __func__);
    
    if (g_uart_dma_initialized) {
        printf("[%s:%s] UART DMA already initialized\n", __FILE__, __func__);
        return 0;
    }
    
    /* 在仿真模式中，我们模拟DMA通道分配 */
    g_uart_dma_tx.dma_channel = 1;  /* 模拟分配通道1用于TX */
    g_uart_dma_rx.dma_channel = 2;  /* 模拟分配通道2用于RX */
    
    /* 确保初始状态为完成 */
    g_uart_dma_tx.completed = true;
    g_uart_dma_rx.completed = true;
    
    g_uart_dma_initialized = true;
    printf("[%s:%s] UART DMA initialized (simulation), TX channel=%d, RX channel=%d\n", 
           __FILE__, __func__, g_uart_dma_tx.dma_channel, g_uart_dma_rx.dma_channel);
    
    return 0;
}

/**
  * @brief  Legacy UART DMA cleanup
  * @retval None
  */
void uart_dma_cleanup(void)
{
    printf("[%s:%s] UART DMA cleanup...\n", __FILE__, __func__);
    
    if (!g_uart_dma_initialized) {
        return;
    }
    
    /* 禁用DMA */
    *UART_DMA_CTRL_REG_PTR = 0;
    
    /* 释放DMA通道 */
    if (g_uart_dma_tx.dma_channel >= 0) {
        dma_free_channel(g_uart_dma_tx.dma_channel);
        g_uart_dma_tx.dma_channel = -1;
    }
    
    if (g_uart_dma_rx.dma_channel >= 0) {
        dma_free_channel(g_uart_dma_rx.dma_channel);
        g_uart_dma_rx.dma_channel = -1;
    }
    
    g_uart_dma_initialized = false;
    printf("[%s:%s] UART DMA cleanup completed\n", __FILE__, __func__);
}

/**
  * @brief  Legacy send single byte
  * @param  data Byte to send
  * @retval 0 on success, -1 on error
  */
int uart_send_byte(uint8_t data)
{
    /* Use HAL function for better reliability */
    if (g_UartHandle.Instance != NULL) {
        if (HAL_UART_Transmit(&g_UartHandle, &data, 1, 1000) == HAL_OK) {
            return 0;
        } else {
            return -1;
        }
    }
    
    /* Fallback to direct register access */
    /* 等待发送就绪 */
    while ((*UART_STATUS_REG_PTR & UART_TX_READY) == 0) {
        usleep(1000);  /* 等待1ms */
    }
    
    /* 写入发送寄存器 */
    *UART_TX_REG_PTR = data;
    
    /* 等待发送完成中断（可选） */
    uart_tx_complete = 0;
    while (!uart_tx_complete) {
        usleep(1000);  /* 简化的等待方式 */
        break;  /* 在真实环境中，这里应该等待中断 */
    }
    
    return 0;
}

/**
  * @brief  Legacy receive single byte
  * @param  data Pointer to store received byte
  * @retval 0 on success, -1 on error
  */
int uart_receive_byte(uint8_t *data)
{
    if (!data) {
        return -1;
    }
    
    /* Use HAL function for better reliability */
    if (g_UartHandle.Instance != NULL) {
        if (HAL_UART_Receive(&g_UartHandle, data, 1, 10000) == HAL_OK) {
            return 0;
        } else {
            return -1;
        }
    }
    
    /* Fallback to direct register access */
    /* 等待接收中断或直接检查状态 */
    int timeout = 10;  /* 10次检查后超时 */
    while (timeout-- > 0) {
        /* 先检查中断标志 */
        if (uart_rx_available) {
            /* 读取接收寄存器 */
            *data = (uint8_t)(*UART_RX_REG_PTR & 0xFF);
            uart_rx_available = 0;  /* 清除中断标志 */
            return 0;
        }
        
        /* 然后检查状态寄存器（只检查一次） */
        if ((*UART_STATUS_REG_PTR & UART_RX_READY) != 0) {
            /* 读取接收寄存器 */
            *data = (uint8_t)(*UART_RX_REG_PTR & 0xFF);
            return 0;
        }
        
        sleep(1);  /* 等待1秒 */
    }
    
    return -1;  /* 超时，没有数据可读 */
}

/**
  * @brief  Legacy send string
  * @param  str String to send
  * @retval 0 on success, -1 on error
  */
int uart_send_string(const char *str)
{
    if (!str) {
        return -1;
    }
    
    /* Use HAL function for better reliability */
    if (g_UartHandle.Instance != NULL) {
        uint16_t len = 0;
        /* Calculate string length */
        while (str[len] != '\0') len++;
        
        if (HAL_UART_Transmit(&g_UartHandle, (const uint8_t*)str, len, 5000) == HAL_OK) {
            return 0;
        } else {
            return -1;
        }
    }
    
    /* Fallback to byte-by-byte transmission */
    while (*str) {
        if (uart_send_byte(*str) != 0) {
            return -1;
        }
        str++;
    }
    
    return 0;
}

/**
  * @brief  Legacy DMA send function
  * @param  data Pointer to data buffer
  * @param  size Number of bytes to send
  * @retval 0 on success, -1 on error
  */
int uart_dma_send(const uint8_t *data, uint32_t size)
{
    if (!data || size == 0) {
        printf("[%s:%s] Invalid parameters\n", __FILE__, __func__);
        return -1;
    }
    
    if (!g_uart_dma_initialized) {
        printf("[%s:%s] UART DMA not initialized\n", __FILE__, __func__);
        return -1;
    }
    
    if (!g_uart_dma_tx.completed) {
        printf("[%s:%s] Previous DMA TX still in progress\n", __FILE__, __func__);
        return -1;
    }
    
    /* 设置传输参数 */
    g_uart_dma_tx.buffer = (uint8_t*)data;
    g_uart_dma_tx.size = size;
    g_uart_dma_tx.completed = false;
    
    printf("[%s:%s] Starting UART DMA send, channel=%d, size=%d\n", 
           __FILE__, __func__, g_uart_dma_tx.dma_channel, size);
    
    /* 在仿真环境中，我们简化DMA传输过程 */
    /* 直接设置完成状态，模拟快速传输 */
    printf("[%s:%s] Simulation mode: simulating instant DMA completion\n", __FILE__, __func__);
    sleep(1);  /* 模拟传输时间 */
    
    /* 模拟传输完成 */
    g_uart_dma_tx.completed = true;
    uart_dma_tx_callback(g_uart_dma_tx.dma_channel, DMA_CH_DONE);
    
    printf("[%s:%s] UART DMA send simulation completed\n", __FILE__, __func__);
    return 0;
}

/**
  * @brief  Legacy DMA receive function
  * @param  buffer Pointer to receive buffer
  * @param  size Number of bytes to receive
  * @retval 0 on success, -1 on error
  */
int uart_dma_receive(uint8_t *buffer, uint32_t size)
{
    if (!buffer || size == 0) {
        printf("[%s:%s] Invalid parameters\n", __FILE__, __func__);
        return -1;
    }
    
    if (!g_uart_dma_initialized) {
        printf("[%s:%s] UART DMA not initialized\n", __FILE__, __func__);
        return -1;
    }
    
    if (!g_uart_dma_rx.completed) {
        printf("[%s:%s] Previous DMA RX still in progress\n", __FILE__, __func__);
        return -1;
    }
    
    /* 设置传输参数 */
    g_uart_dma_rx.buffer = buffer;
    g_uart_dma_rx.size = size;
    g_uart_dma_rx.completed = false;
    
    /* 启用UART DMA接收 */
    *UART_DMA_CTRL_REG_PTR |= UART_DMA_RX_ENABLE;
    
    /* 启动DMA传输 (外设到内存) */
    if (dma_transfer_async(g_uart_dma_rx.dma_channel,
                          UART_RX_REG,
                          (uintptr_t)buffer,
                          size,
                          DMA_TRANSFER_PER_TO_MEM,
                          uart_dma_rx_callback) != 0) {
        printf("[%s:%s] Failed to start DMA RX transfer\n", __FILE__, __func__);
        *UART_DMA_CTRL_REG_PTR &= ~UART_DMA_RX_ENABLE;
        return -1;
    }
    
    printf("[%s:%s] Started UART DMA receive, size=%d\n", __FILE__, __func__, size);
    return 0;
}

/**
  * @brief  Check if DMA send is completed
  * @retval true if completed, false otherwise
  */
bool uart_dma_send_completed(void)
{
    return g_uart_dma_tx.completed;
}

/**
  * @brief  Check if DMA receive is completed
  * @retval true if completed, false otherwise
  */
bool uart_dma_receive_completed(void)
{
    return g_uart_dma_rx.completed;
}

/**
  * @brief  Wait for DMA send completion
  * @param  timeout_ms Timeout in milliseconds
  * @retval 0 on success, -1 on timeout
  */
int uart_dma_wait_send_complete(uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    printf("[%s:%s] Waiting for DMA send completion, timeout=%d ms\n", __FILE__, __func__, timeout_ms);
    
    while (!g_uart_dma_tx.completed && elapsed < timeout_ms) {
        usleep(1000);  /* 等待1ms */
        elapsed++;
        
        /* 在模拟环境中，每10ms打印一次状态 */
        if (elapsed % 1000 == 0) {
            printf("[%s:%s] Waiting for DMA completion... elapsed=%d ms\n", __FILE__, __func__, elapsed);
        }
    }
    
    if (!g_uart_dma_tx.completed) {
        printf("[%s:%s] DMA send timeout after %d ms\n", __FILE__, __func__, elapsed);
        return -1;
    }
    
    printf("[%s:%s] DMA send completed successfully\n", __FILE__, __func__);
    return 0;
}

/**
  * @brief  Wait for DMA receive completion
  * @param  timeout_ms Timeout in milliseconds
  * @retval 0 on success, -1 on timeout
  */
int uart_dma_wait_receive_complete(uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    printf("[%s:%s] Waiting for DMA receive completion, timeout=%d ms\n", __FILE__, __func__, timeout_ms);
    
    while (!g_uart_dma_rx.completed && elapsed < timeout_ms) {
        usleep(1000);  /* 等待1ms */
        elapsed++;
        
        /* 在模拟环境中，每10ms打印一次状态 */
        if (elapsed % 10 == 0) {
            printf("[%s:%s] Waiting for DMA RX completion... elapsed=%d ms\n", __FILE__, __func__, elapsed);
        }
        
        /* 模拟环境下的简化处理：如果等待时间超过100ms，假设传输完成 */
        if (elapsed >= 100) {
            printf("[%s:%s] Simulation mode: forcing DMA RX completion after 100ms\n", __FILE__, __func__);
            g_uart_dma_rx.completed = true;
            /* 模拟回调函数调用 */
            uart_dma_rx_callback(g_uart_dma_rx.dma_channel, DMA_CH_DONE);
            break;
        }
    }
    
    if (!g_uart_dma_rx.completed) {
        printf("[%s:%s] DMA receive timeout after %d ms\n", __FILE__, __func__, elapsed);
        return -1;
    }
    
    printf("[%s:%s] DMA receive completed successfully\n", __FILE__, __func__);
    return 0;
}

/**
  * @brief  Set UART transfer mode
  * @param  mode Transfer mode
  * @retval 0 on success, -1 on error
  */
int uart_set_mode(UART_TransferModeTypeDef mode)
{
    g_uart_mode = mode;
    
    /* Update HAL handle if available */
    if (g_UartHandle.Instance != NULL) {
        g_UartHandle.Init.TransferMode = mode;
    }
    
    printf("[%s:%s] UART mode set to %d\n", __FILE__, __func__, mode);
    return 0;
}

/**
  * @brief  Get current UART transfer mode
  * @retval Current transfer mode
  */
UART_TransferModeTypeDef uart_get_mode(void)
{
    return g_uart_mode;
}

/**
  * @}
  */

/* ============================================================================ */
/* ======================= HAL CALLBACK FUNCTIONS =========================== */
/* ============================================================================ */

/** @defgroup UART_HAL_Callbacks UART HAL Callback Functions
  * @brief HAL callback functions that can be overridden by user
  * @{
  */

/**
  * @brief  Tx Transfer completed callback.
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_TxCpltCallback could be implemented in the user file
     */
    printf("[%s:%s] UART TX completion callback\n", __FILE__, __func__);
}

/**
  * @brief  Tx Half Transfer completed callback.
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_TxHalfCpltCallback could be implemented in the user file
     */
    printf("[%s:%s] UART TX half completion callback\n", __FILE__, __func__);
}

/**
  * @brief  Rx Transfer completed callback.
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_RxCpltCallback could be implemented in the user file
     */
    printf("[%s:%s] UART RX completion callback\n", __FILE__, __func__);
}

/**
  * @brief  Rx Half Transfer completed callback.
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_RxHalfCpltCallback could be implemented in the user file
     */
    printf("[%s:%s] UART RX half completion callback\n", __FILE__, __func__);
}

/**
  * @brief  UART error callback.
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_ErrorCallback could be implemented in the user file
     */
    printf("[%s:%s] UART error callback, ErrorCode=0x%08X\n", __FILE__, __func__, huart->ErrorCode);
}

/**
  * @brief  UART Abort Complete callback.
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_AbortCpltCallback could be implemented in the user file
     */
    printf("[%s:%s] UART abort completion callback\n", __FILE__, __func__);
}

/**
  * @brief  UART Abort Transmit Complete callback.
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_AbortTransmitCpltCallback could be implemented in the user file
     */
    printf("[%s:%s] UART abort transmit completion callback\n", __FILE__, __func__);
}

/**
  * @brief  UART Abort Receive Complete callback.
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_AbortReceiveCpltCallback could be implemented in the user file
     */
    printf("[%s:%s] UART abort receive completion callback\n", __FILE__, __func__);
}

/**
  * @brief  DMA transmit complete callback.
  * @param  huart UART handle.
  * @retval None
  */
static void UART_DMATransmitCplt(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    printf("[%s:%s] UART DMA transmit completion callback\n", __FILE__, __func__);
    /* Set transmission state to ready */
    huart->gState = HAL_UART_STATE_READY;
}

/**
  * @brief  DMA receive complete callback.
  * @param  huart UART handle.
  * @retval None
  */
static void UART_DMAReceiveCplt(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    printf("[%s:%s] UART DMA receive completion callback\n", __FILE__, __func__);
    /* Set reception state to ready */
    huart->RxState = HAL_UART_STATE_READY;
}

/**
  * @brief  DMA error callback.
  * @param  huart UART handle.
  * @retval None
  */
static void UART_DMAError(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    printf("[%s:%s] UART DMA error callback\n", __FILE__, __func__);
    /* Set error state */
    huart->ErrorCode |= HAL_UART_ERROR_DMA;
    huart->gState = HAL_UART_STATE_READY;
    huart->RxState = HAL_UART_STATE_READY;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT IC Simulator Project *****END OF FILE****/
