#include "cmsis_os2.h"  // CMSIS RTOS header file

/*----------------------------------------------------------------------------
 *      Message Queue creation & usage
 *---------------------------------------------------------------------------*/

#define MSGQUEUE_OBJECTS 5  // number of Message Queue Objects

osMessageQueueId_t mid_MsgRx;  // message queue id

int Init_MsgQueue(void)
{
    mid_MsgRx = osMessageQueueNew(MSGQUEUE_OBJECTS, 4, NULL);
    if (mid_MsgRx == NULL) {
        ;  // Message Queue object not created, handle failure
    }
    return (0);
}
