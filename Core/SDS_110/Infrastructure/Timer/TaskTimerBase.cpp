#include "TaskTimerBase.hpp"
#include "Infrastructure/Utils/DWT.hpp"

TaskTimerBase::TaskTimerBase(const char* taskName, uint16_t stackSize, UBaseType_t priority)
    : overrunFlag(false),
      taskRunning(false),
      timer(nullptr)
{
    xTaskCreate(taskEntry, taskName, stackSize, this, priority, &taskHandle);
}

void TaskTimerBase::attachTimer(HardwareTimer* t)
{
    timer = t;

    allowedTimeUs = timer->getRateHz();

    timer->setCallback([this]() {
        this->onTimerISR();
    });

}

void TaskTimerBase::start()
{
    overrunFlag.store(false);
    taskRunning.store(false);
    if (timer) timer->start();
}

void TaskTimerBase::stop()
{
    if (timer) timer->stop();
}

void TaskTimerBase::onTimerISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(taskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void TaskTimerBase::taskEntry(void* arg)
{
    static_cast<TaskTimerBase*>(arg)->taskLoop();
}

void TaskTimerBase::taskLoop()
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint32_t start = DWT->CYCCNT;
        onTask();
        uint32_t end = DWT->CYCCNT;

        if ((end - start) > allowedTimeUs)
            overrunFlag.store(true);
        else
            overrunFlag.store(false);
    }
}
