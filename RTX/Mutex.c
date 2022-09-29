#include "cmsis_os2.h"  // CMSIS RTOS header file

/*----------------------------------------------------------------------------
 *      Mutex creation & usage
 *---------------------------------------------------------------------------*/

osMutexId_t mid_Mutex;  // mutex id

int Init_Mutex(void)
{

    mid_Mutex = osMutexNew(NULL);
    if (mid_Mutex == NULL) {
        ;  // Mutex object not created, handle failure
    }
    return (0);
}
