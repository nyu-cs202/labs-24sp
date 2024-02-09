#pragma once

#include "EStore.h"
#include "TaskQueue.h"
#include "Request.h"

class RequestGenerator {
    private:
    TaskQueue* taskQueue;

    protected:
    int taskCount;

    virtual Task generateTask(EStore* store) = 0;

    public:
    RequestGenerator(TaskQueue* queue);
    virtual ~RequestGenerator();

    void enqueueTasks(int maxTasks, EStore* store);
    void enqueueStops(int num);
};

class SupplierRequestGenerator : public RequestGenerator {
    protected:
    virtual Task generateTask(EStore* store);

    public:
    SupplierRequestGenerator(TaskQueue* queue);
};

class CustomerRequestGenerator : public RequestGenerator {
    private:
    bool fineMode;

    protected:
    virtual Task generateTask(EStore* store);

    public:
    CustomerRequestGenerator(TaskQueue* queue, bool inFineMode);
};

