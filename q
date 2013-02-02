diff --git a/core/__kthreads/x8632/__korientationMain.cpp b/core/__kthreads/x8632/__korientationMain.cpp
index 881796d..e788928 100755
--- a/core/__kthreads/x8632/__korientationMain.cpp
+++ b/core/__kthreads/x8632/__korientationMain.cpp
@@ -31,9 +31,7 @@ void thread0(void)
 	for (;;)
 	{
 		__kprintf(NOTICE"Thread0: Hello.\n");
-		//asm volatile ("hlt\n\t");
-		taskTrib.wake(0x1);
-		taskTrib.dormant(0x2);
+		taskTrib.yield();
 	};
 }
 
@@ -41,7 +39,7 @@ extern "C" void __korientationInit(ubit32, multibootDataS *)
 {
 	error_t		ret;
 	uarch_t		devMask;
-	processId_t		tid;
+	processId_t	tid;
 
 	/* Zero out uninitialized sections, prepare kernel locking and place a
 	 * pointer to the BSP CPU Stream into the BSP CPU; then we can call all
@@ -124,7 +122,7 @@ extern "C" void __korientationInit(ubit32, multibootDataS *)
 			__KNULL,
 			taskC::ROUND_ROBIN,
 			0,
-			0,
+			SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT,
 			&tid),
 		ret);
 
@@ -138,8 +136,7 @@ void __korientationMain(void)
 	for (;;)
 	{
 		__kprintf(NOTICE ORIENT"Says hello.\n");
-		taskTrib.wake(0x2);
-		taskTrib.dormant(0x1);
+		taskTrib.yield();
 	};
 
 for (__kprintf(NOTICE ORIENT"Reached HLT in Orientation Main.\n");;) { asm volatile("hlt\n\t"); };
diff --git a/core/include/kernel/common/process.h b/core/include/kernel/common/process.h
index 05aac1f..9135d95 100755
--- a/core/include/kernel/common/process.h
+++ b/core/include/kernel/common/process.h
@@ -37,6 +37,9 @@
 #define SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET	(1<<5)
 #define SPAWNTHREAD_FLAGS_SCHEDPRIO_SET			(1<<6)
 
+// Set to prevent the thread from being scheduled automatically on creation.
+#define SPAWNTHREAD_FLAGS_DORMANT			(1<<8)
+
 #define SPAWNTHREAD_STATUS_NO_PIDS		0x1
 
 class processStreamC
diff --git a/core/include/kernel/common/taskTrib/taskStream.h b/core/include/kernel/common/taskTrib/taskStream.h
index d9e820f..5c90eef 100644
--- a/core/include/kernel/common/taskTrib/taskStream.h
+++ b/core/include/kernel/common/taskTrib/taskStream.h
@@ -48,6 +48,9 @@ public:
 	error_t schedule(taskC* task);
 	void dormant(taskC *task)
 	{
+		task->runState = taskC::STOPPED;
+		task->blockState = taskC::DORMANT;
+
 		switch (task->schedPolicy)
 		{
 		case taskC::ROUND_ROBIN:
@@ -61,41 +64,72 @@ public:
 		default:
 			return;
 		};
+	}
 
+	void block(taskC *task)
+	{
 		task->runState = taskC::STOPPED;
-		task->blockState = taskC::DORMANT;
-	}
+		task->blockState = taskC::BLOCKED;
 
+		switch (task->schedPolicy)
+		{
+		case taskC::ROUND_ROBIN:
+			roundRobinQ.remove(task, task->schedPrio->prio);
+			break;
+
+		case taskC::REAL_TIME:
+			realTimeQ.remove(task, task->schedPrio->prio);
+			break;
+
+		default:
+			return;
+		};
+	}
+		
 	void yield(taskC *task)
 	{
+		task->runState = taskC::RUNNABLE;
+
 		switch (task->schedPolicy)
 		{
 		case taskC::ROUND_ROBIN:
-			roundRobinQ.insert(task, task->schedPrio->prio);
+			roundRobinQ.insert(
+				task, task->schedPrio->prio,
+				task->schedOptions);
 			break;
 
 		case taskC::REAL_TIME:
-			realTimeQ.insert(task, task->schedPrio->prio);
+			realTimeQ.insert(
+				task, task->schedPrio->prio,
+				task->schedOptions);
 			break;
 
 		default:
 			return;
 		};
-
-		task->runState = taskC::RUNNABLE;
 	}
 
 	error_t wake(taskC *task)
 	{
-		if (task->runState != taskC::STOPPED) { return ERROR_SUCCESS; };
+		if (!(task->runState == taskC::STOPPED
+			&& task->blockState == taskC::DORMANT))
+		{
+			return ERROR_SUCCESS;
+		};
+
+		task->runState = taskC::RUNNABLE;
 
 		switch (task->schedPolicy)
 		{
 		case taskC::ROUND_ROBIN:
-			return roundRobinQ.insert(task, task->schedPrio->prio);
+			return roundRobinQ.insert(
+				task, task->schedPrio->prio,
+				task->schedOptions);
 
 		case taskC::REAL_TIME:
-			return realTimeQ.insert(task, task->schedPrio->prio);
+			return realTimeQ.insert(
+				task, task->schedPrio->prio,
+				task->schedOptions);
 
 		default:
 			return ERROR_CRITICAL;
diff --git a/core/kernel/common/process.cpp b/core/kernel/common/process.cpp
index fabeb21..d7308d8 100644
--- a/core/kernel/common/process.cpp
+++ b/core/kernel/common/process.cpp
@@ -157,7 +157,12 @@ error_t processStreamC::spawnThread(
 		execDomain, newTask->stack0, newTask->stack1);
 
 	newTask->context->setEntryPoint(entryPoint);
-	return taskTrib.schedule(newTask);
+
+	if (!__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_DORMANT)) {
+		return taskTrib.schedule(newTask);
+	} else {
+		return ERROR_SUCCESS;
+	};
 }
 
 taskC *processStreamC::allocateNewThread(processId_t newThreadId)
diff --git a/core/kernel/common/taskTrib/taskStream.cpp b/core/kernel/common/taskTrib/taskStream.cpp
index 580257a..e416caa 100644
--- a/core/kernel/common/taskTrib/taskStream.cpp
+++ b/core/kernel/common/taskTrib/taskStream.cpp
@@ -106,10 +106,7 @@ status_t taskStreamC::schedule(taskC *task)
 #endif
 	// Check CPU suitability to run task (FPU, other features).
 
-	// Validate scheduling parameters.
-	if (task->schedPrio->prio >= SCHEDPRIO_MAX_NPRIOS) {
-		return ERROR_UNSUPPORTED;
-	};
+	// Validate any scheduling parameters that need to be checked.
 
 	task->runState = taskC::RUNNABLE;
 	task->currentCpu = parentCpu;
diff --git a/core/kernel/common/taskTrib/taskTrib.cpp b/core/kernel/common/taskTrib/taskTrib.cpp
index 18a27ff..34ef08c 100644
--- a/core/kernel/common/taskTrib/taskTrib.cpp
+++ b/core/kernel/common/taskTrib/taskTrib.cpp
@@ -90,9 +90,10 @@ error_t taskTribC::dormant(taskC *task)
 	cpuStreamC	*currentCpu;
 
 	if (task->runState == taskC::UNSCHEDULED
-		|| task->blockState == taskC::DORMANT)
+		|| (task->runState == taskC::STOPPED
+			&& task->blockState == taskC::DORMANT))
 	{
-		// return ERROR_SUCCESS;
+		return ERROR_SUCCESS;
 	};
 
 	task->currentCpu->taskStream.dormant(task);
@@ -101,7 +102,8 @@ error_t taskTribC::dormant(taskC *task)
 	if (task->currentCpu != currentCpu)
 	{
 		/* Message the CPU to pre-empt and choose another thread.
-		 * Should take an argument for the thread to be pre-empted.
+		 * Should probably take an argument for the thread to be
+		 * pre-empted.
 		 **/
 	}
 	else
@@ -121,7 +123,18 @@ error_t taskTribC::dormant(taskC *task)
 
 error_t taskTribC::wake(taskC *task)
 {
-	return task->currentCpu->taskStream.wake(task);
+	if (task->runState != taskC::UNSCHEDULED
+		&& (task->runState != taskC::STOPPED
+			&& task->blockState != taskC::DORMANT))
+	{
+		return ERROR_SUCCESS;
+	};
+
+	if (task->runState == taskC::UNSCHEDULED) {
+		return schedule(task);
+	} else {
+		return task->currentCpu->taskStream.wake(task);
+	};
 }
 
 void taskTribC::yield(void)
@@ -134,7 +147,20 @@ void taskTribC::yield(void)
 	saveContextAndCallPull(
 		&currTask->currentCpu->sleepStack[
 			sizeof(currTask->currentCpu->sleepStack)]);
+}
 
-	// Unreachable.
+void taskTribC::block(void)
+{
+	taskC		*currTask;
+
+	/* After placing a task into a waitqueue, call this function to
+	 * place it into a "blocked" state.
+	 **/
+	currTask = cpuTrib.getCurrentCpuStream()->taskStream.currentTask;
+	currTask->currentCpu->taskStream.block(currTask);
+
+	saveContextAndCallPull(
+		&currTask->currentCpu->sleepStack[
+			sizeof(currTask->currentCpu->sleepStack)]);
 }
 
