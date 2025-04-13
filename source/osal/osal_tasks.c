#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "osal_cbtimer.h"
#include "ll.h"

/* Application */
#include "app/app.h"
/*********************************************************************
 * GLOBAL VARIABLES
 */

// The order in this table must be identical to the task initialization calls below in osalInitTask.
const pTaskEventHandlerFn tasksArr[] =
{
        LL_ProcessEvent,
//  Pulse_Measure_ProcessEvent,
#if defined(OSAL_CBTIMER_NUM_TASKS)
        OSAL_CBTIMER_PROCESS_EVENT(osal_CbTimerProcessEvent), // task 3
#endif
        // Add Proccess Events Here
};

const uint8 tasksCnt = sizeof(tasksArr) / sizeof(tasksArr[0]);
uint16 *tasksEvents;

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      osalInitTasks
 *
 * @brief   This function invokes the initialization function for each task.
 *
 * @param   void
 *
 * @return  none
 */
void osalInitTasks(void)
{
    uint8 taskID = 0;

    tasksEvents = (uint16 *)osal_mem_alloc(sizeof(uint16) * tasksCnt);
    osal_memset(tasksEvents, 0, (sizeof(uint16) * tasksCnt));

    LL_Init(taskID++);
#if defined(OSAL_CBTIMER_NUM_TASKS)
    /* Callback Timer Tasks */
    osal_CbTimerInit(taskID);
    taskID += OSAL_CBTIMER_NUM_TASKS;
#endif
    /*
      Application
    */

    app_init();

    while(1) app_update();
}

/*********************************************************************
*********************************************************************/