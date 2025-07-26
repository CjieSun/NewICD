/**
 ******************************************************************************
 * @file    dma_driver.h
 * @author  IC Simulator Team
 * @brief   DMA HAL Driver Header File (CMSIS Style)
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

#ifndef DMA_DRIVER_H
#define DMA_DRIVER_H

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

/** @addtogroup DMA
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup DMA_Exported_Types DMA Exported Types
  * @{
  */

/**
  * @brief  DMA State Definition
  */
typedef enum {
    HAL_DMA_STATE_RESET             = 0x00U,    /*!< DMA not yet initialized or disabled */
    HAL_DMA_STATE_READY             = 0x01U,    /*!< DMA initialized and ready for use   */
    HAL_DMA_STATE_BUSY              = 0x02U,    /*!< DMA process is ongoing              */
    HAL_DMA_STATE_TIMEOUT           = 0x03U,    /*!< DMA timeout state                   */
    HAL_DMA_STATE_ERROR             = 0x04U,    /*!< DMA error state                     */
    HAL_DMA_STATE_ABORT             = 0x05U     /*!< DMA Abort state                     */
} HAL_DMA_StateTypeDef;

/**
  * @brief  DMA Error Definition
  */
typedef enum {
    HAL_DMA_ERROR_NONE              = 0x00000000U,   /*!< No error             */
    HAL_DMA_ERROR_TE                = 0x00000001U,   /*!< Transfer error       */
    HAL_DMA_ERROR_FE                = 0x00000002U,   /*!< FIFO error           */
    HAL_DMA_ERROR_DME               = 0x00000004U,   /*!< Direct Mode error    */
    HAL_DMA_ERROR_TIMEOUT           = 0x00000020U,   /*!< Timeout error        */
    HAL_DMA_ERROR_PARAM             = 0x00000040U,   /*!< Parameter error      */
    HAL_DMA_ERROR_NO_XFER           = 0x00000080U,   /*!< Abort requested with no Xfer ongoing */
    HAL_DMA_ERROR_NOT_SUPPORTED     = 0x00000100U    /*!< Not supported mode */
} HAL_DMA_ErrorTypeDef;

/**
  * @brief  DMA Transfer Direction Definition
  */
typedef enum {
    DMA_MEMORY_TO_MEMORY            = 0x00000000U,   /*!< Memory to memory direction     */
    DMA_MEMORY_TO_PERIPH            = 0x00000001U,   /*!< Memory to peripheral direction */
    DMA_PERIPH_TO_MEMORY            = 0x00000002U,   /*!< Peripheral to memory direction */
    DMA_PERIPH_TO_PERIPH            = 0x00000003U    /*!< Peripheral to peripheral direction */
} DMA_DirectionTypeDef;

typedef enum {
    DMA_CH_IDLE = 0,
    DMA_CH_BUSY = 1,
    DMA_CH_DONE = 2,
    DMA_CH_ERROR = 3
} dma_channel_status_t;

typedef void (*dma_callback_t)(uint8_t channel, dma_channel_status_t status);

/**
  * @brief  DMA Transfer Mode Definition
  */
typedef enum {
    DMA_NORMAL                      = 0x00000000U,   /*!< Normal mode                  */
    DMA_CIRCULAR                    = 0x00000001U    /*!< Circular mode                */
} DMA_ModeTypeDef;

/**
  * @brief  DMA Priority Level Definition
  */
typedef enum {
    DMA_PRIORITY_LOW                = 0x00000000U,   /*!< Priority level : Low       */
    DMA_PRIORITY_MEDIUM             = 0x00000001U,   /*!< Priority level : Medium    */
    DMA_PRIORITY_HIGH               = 0x00000002U,   /*!< Priority level : High      */
    DMA_PRIORITY_VERY_HIGH          = 0x00000003U    /*!< Priority level : Very_High */
} DMA_PriorityTypeDef;

/**
  * @brief  DMA Memory Data Size Definition
  */
typedef enum {
    DMA_MDATAALIGN_BYTE             = 0x00000000U,   /*!< Memory data alignment : Byte     */
    DMA_MDATAALIGN_HALFWORD         = 0x00000001U,   /*!< Memory data alignment : HalfWord */
    DMA_MDATAALIGN_WORD             = 0x00000002U    /*!< Memory data alignment : Word     */
} DMA_MemDataAlignmentTypeDef;

/**
  * @brief  DMA Peripheral Data Size Definition
  */
typedef enum {
    DMA_PDATAALIGN_BYTE             = 0x00000000U,   /*!< Peripheral data alignment : Byte     */
    DMA_PDATAALIGN_HALFWORD         = 0x00000001U,   /*!< Peripheral data alignment : HalfWord */
    DMA_PDATAALIGN_WORD             = 0x00000002U    /*!< Peripheral data alignment : Word     */
} DMA_PerDataAlignmentTypeDef;

/**
  * @brief  DMA Memory Increment Definition
  */
typedef enum {
    DMA_MINC_DISABLE                = 0x00000000U,   /*!< Memory increment mode disable */
    DMA_MINC_ENABLE                 = 0x00000001U    /*!< Memory increment mode enable  */
} DMA_MemIncTypeDef;

/**
  * @brief  DMA Peripheral Increment Definition
  */
typedef enum {
    DMA_PINC_DISABLE                = 0x00000000U,   /*!< Peripheral increment mode disable */
    DMA_PINC_ENABLE                 = 0x00000001U    /*!< Peripheral increment mode enable  */
} DMA_PerIncTypeDef;

/**
  * @brief  DMA Level Complete Definition
  */
typedef enum {
    HAL_DMA_FULL_TRANSFER     = 0x00U,    /*!< Full transfer     */
    HAL_DMA_HALF_TRANSFER     = 0x01U     /*!< Half Transfer     */
} HAL_DMA_LevelCompleteTypeDef;

/**
  * @brief  DMA Callback ID Definition
  */
typedef enum {
    HAL_DMA_XFER_CPLT_CB_ID          = 0x00U,    /*!< Full transfer     */
    HAL_DMA_XFER_HALFCPLT_CB_ID      = 0x01U,    /*!< Half transfer     */
    HAL_DMA_XFER_ERROR_CB_ID         = 0x02U,    /*!< Error             */
    HAL_DMA_XFER_ABORT_CB_ID         = 0x03U,    /*!< Abort             */
    HAL_DMA_XFER_ALL_CB_ID           = 0x04U     /*!< All               */
} HAL_DMA_CallbackIDTypeDef;

/* Legacy type definitions for backward compatibility */
typedef enum {
    DMA_TRANSFER_MEM_TO_MEM = 0,
    DMA_TRANSFER_MEM_TO_PER = 1,
    DMA_TRANSFER_PER_TO_MEM = 2,
    DMA_TRANSFER_PER_TO_PER = 3
} dma_transfer_type_t;

typedef struct {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t size;
    dma_transfer_type_t type;
    bool inc_src;
    bool inc_dst;
    bool interrupt_enable;
} dma_config_t;

/**
  * @brief  DMA Channel Configuration Structure definition
  */
typedef struct {
    DMA_DirectionTypeDef             Direction;         /*!< Specifies the transfer direction. */

    DMA_ModeTypeDef                  Mode;              /*!< Specifies the operation mode. */

    DMA_PriorityTypeDef              Priority;          /*!< Specifies the priority level. */

    DMA_MemDataAlignmentTypeDef      MemDataAlignment;  /*!< Specifies the memory data width. */

    DMA_PerDataAlignmentTypeDef      PeriphDataAlignment; /*!< Specifies the peripheral data width. */

    DMA_MemIncTypeDef                MemInc;            /*!< Specifies whether memory address incremented. */

    DMA_PerIncTypeDef                PeriphInc;         /*!< Specifies whether peripheral address incremented. */

} DMA_InitTypeDef;

/**
  * @brief  DMA Channel Handle Structure definition
  */
typedef struct __DMA_HandleTypeDef {
    DMA_Channel_TypeDef              *Instance;         /*!< Register base address                  */

    DMA_InitTypeDef                  Init;              /*!< DMA communication parameters           */

    HAL_LockTypeDef                  Lock;              /*!< DMA locking object                     */

    __IO HAL_DMA_StateTypeDef        State;             /*!< DMA transfer state                     */

    void                             *Parent;           /*!< Parent object state                    */

    void (* XferCpltCallback)(struct __DMA_HandleTypeDef * hdma);      /*!< DMA transfer complete callback      */

    void (* XferHalfCpltCallback)(struct __DMA_HandleTypeDef * hdma);  /*!< DMA Half transfer complete callback */

    void (* XferErrorCallback)(struct __DMA_HandleTypeDef * hdma);     /*!< DMA transfer error callback         */

    void (* XferAbortCallback)(struct __DMA_HandleTypeDef * hdma);     /*!< DMA transfer abort callback         */

    __IO uint32_t                    ErrorCode;         /*!< DMA Error code                          */

    uint32_t                         StreamBaseAddress; /*!< DMA Channel Base Address                */

    uint32_t                         ChannelIndex;      /*!< DMA Channel Index                       */

} DMA_HandleTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup DMA_Exported_Constants DMA Exported Constants
  * @{
  */

/** @defgroup DMA_Channel_definition DMA Channel Definition
  * @{
  */
#define DMA_CHANNEL_0                 0U    /*!< DMA Channel 0 */
#define DMA_CHANNEL_1                 1U    /*!< DMA Channel 1 */
#define DMA_CHANNEL_2                 2U    /*!< DMA Channel 2 */
#define DMA_CHANNEL_3                 3U    /*!< DMA Channel 3 */
#define DMA_CHANNEL_4                 4U    /*!< DMA Channel 4 */
#define DMA_CHANNEL_5                 5U    /*!< DMA Channel 5 */
#define DMA_CHANNEL_6                 6U    /*!< DMA Channel 6 */
#define DMA_CHANNEL_7                 7U    /*!< DMA Channel 7 */
/**
  * @}
  */

/** @defgroup DMA_Configuration_Register DMA Configuration Register
  * @{
  */
#define DMA_CHANNEL_ENABLE_Pos        (0U)
#define DMA_CHANNEL_ENABLE_Msk        (0x1UL << DMA_CHANNEL_ENABLE_Pos)
#define DMA_CHANNEL_ENABLE            DMA_CHANNEL_ENABLE_Msk
#define DMA_TRANSFER_COMPLETE_INT_Pos (1U)
#define DMA_TRANSFER_COMPLETE_INT_Msk (0x1UL << DMA_TRANSFER_COMPLETE_INT_Pos)
#define DMA_TRANSFER_COMPLETE_INT     DMA_TRANSFER_COMPLETE_INT_Msk
#define DMA_TRANSFER_ERROR_INT_Pos    (2U)
#define DMA_TRANSFER_ERROR_INT_Msk    (0x1UL << DMA_TRANSFER_ERROR_INT_Pos)
#define DMA_TRANSFER_ERROR_INT        DMA_TRANSFER_ERROR_INT_Msk
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/

/** @defgroup DMA_Exported_Macros DMA Exported Macros
  * @{
  */

/** @brief Reset DMA handle state
  * @param  __HANDLE__ DMA handle.
  * @retval None
  */
#define __HAL_DMA_RESET_HANDLE_STATE(__HANDLE__)  ((__HANDLE__)->State = HAL_DMA_STATE_RESET)

/** @brief  Enable the specified DMA Channel.
  * @param  __HANDLE__ DMA handle
  * @retval None
  */
#define __HAL_DMA_ENABLE(__HANDLE__)        ((__HANDLE__)->Instance->Configuration |= DMA_CHANNEL_ENABLE)

/** @brief  Disable the specified DMA Channel.
  * @param  __HANDLE__ DMA handle
  * @retval None
  */
#define __HAL_DMA_DISABLE(__HANDLE__)       ((__HANDLE__)->Instance->Configuration &= ~DMA_CHANNEL_ENABLE)

/** @brief  Get the DMA Channel pending flags.
  * @param  __HANDLE__ DMA handle
  * @param  __FLAG__ Get the specified flag.
  * @retval The state of FLAG (SET or RESET).
  */
#define __HAL_DMA_GET_FLAG(__HANDLE__, __FLAG__)   (((__HANDLE__)->Instance->Configuration & (__FLAG__)) == (__FLAG__))

/** @brief  Clear the DMA Channel pending flags.
  * @param  __HANDLE__ DMA handle
  * @param  __FLAG__ specifies the flag to clear.
  * @retval None
  */
#define __HAL_DMA_CLEAR_FLAG(__HANDLE__, __FLAG__)  ((__HANDLE__)->Instance->Configuration &= ~(__FLAG__))

/** @brief  Enable the specified DMA Channel interrupts.
  * @param  __HANDLE__ DMA handle
  * @param  __INTERRUPT__ specifies the DMA interrupt sources to be enabled or disabled.
  * @retval None
  */
#define __HAL_DMA_ENABLE_IT(__HANDLE__, __INTERRUPT__)   ((__HANDLE__)->Instance->Configuration |= (__INTERRUPT__))

/** @brief  Disable the specified DMA Channel interrupts.
  * @param  __HANDLE__ DMA handle
  * @param  __INTERRUPT__ specifies the DMA interrupt sources to be enabled or disabled.
  * @retval None
  */
#define __HAL_DMA_DISABLE_IT(__HANDLE__, __INTERRUPT__)  ((__HANDLE__)->Instance->Configuration &= ~(__INTERRUPT__))

/** @brief  Check whether the specified DMA Channel interrupt has occurred or not.
  * @param  __HANDLE__ DMA handle
  * @param  __INTERRUPT__ specifies the DMA interrupt source to check.
  * @retval The state of DMA_IT (SET or RESET).
  */
#define __HAL_DMA_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)  (((__HANDLE__)->Instance->Configuration & (__INTERRUPT__)) == (__INTERRUPT__))

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @addtogroup DMA_Exported_Functions
  * @{
  */

/** @addtogroup DMA_Exported_Functions_Group1
  * @{
  */

/* Initialization and de-initialization functions ****************************/
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMA_DeInit(DMA_HandleTypeDef *hdma);

/**
  * @}
  */

/** @addtogroup DMA_Exported_Functions_Group2
  * @{
  */

/* IO operation functions *****************************************************/
HAL_StatusTypeDef HAL_DMA_Start(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
HAL_StatusTypeDef HAL_DMA_Start_IT(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMA_Abort_IT(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMA_PollForTransfer(DMA_HandleTypeDef *hdma, HAL_DMA_LevelCompleteTypeDef CompleteLevel, uint32_t Timeout);
void HAL_DMA_IRQHandler(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMA_CleanCallbacks(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DMA_RegisterCallback(DMA_HandleTypeDef *hdma, HAL_DMA_CallbackIDTypeDef CallbackID, void (* pCallback)(DMA_HandleTypeDef *_hdma));
HAL_StatusTypeDef HAL_DMA_UnRegisterCallback(DMA_HandleTypeDef *hdma, HAL_DMA_CallbackIDTypeDef CallbackID);

/**
  * @}
  */

/** @addtogroup DMA_Exported_Functions_Group3
  * @{
  */

/* Peripheral State and Error functions **************************************/
HAL_DMA_StateTypeDef HAL_DMA_GetState(DMA_HandleTypeDef *hdma);
uint32_t             HAL_DMA_GetError(DMA_HandleTypeDef *hdma);

/**
  * @}
  */

/** @addtogroup DMA_Exported_Functions_Group4
  * @{
  */

/* Legacy functions for backward compatibility *******************************/
int dma_init(void);
void dma_cleanup(void);
int dma_allocate_channel(void);
int dma_free_channel(uint8_t channel);
bool dma_is_channel_available(uint8_t channel);
int dma_configure_channel(uint8_t channel, const dma_config_t *config);
int dma_start_transfer(uint8_t channel);
int dma_stop_transfer(uint8_t channel);
dma_channel_status_t dma_get_channel_status(uint8_t channel);
int dma_transfer_async(uint8_t channel, uint32_t src, uint32_t dst, uint32_t size, 
                      dma_transfer_type_t type, dma_callback_t callback);
int dma_transfer_sync(uint8_t channel, uint32_t src, uint32_t dst, uint32_t size, 
                     dma_transfer_type_t type);
void dma_interrupt_handler(void);
int dma_register_callback(uint8_t channel, dma_callback_t callback);

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

#endif // DMA_DRIVER_H
