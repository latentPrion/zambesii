### **Kernel Design: Stack Management, Preemption, and Scheduler Operations**

This note explores the design of a kernel with **nested interrupts** (kernel-mode preemption) and **timeslice-based preemptive scheduling**. It focuses on stack management, thread state saving, and the conditions under which scheduler operations (schedops) should trigger immediate context switches. The analysis is structured into **user threads** and **kernel threads**, with a focus on **syscalls**, **IRQs**, and **scheduler types** (cooperative, passive preemptive, and timeslice-based).

---

## **1. Stack Management and Thread State Saving**

### **1.1 User Threads**

#### **Kernel Entry (Syscall or IRQ)**
- **Stack Switch**: The CPU automatically switches to the thread's **kernel stack** (stack0).
- **State Saving**: The CPU saves the **user thread state** (registers, program counter, stack pointer, etc.) onto the kernel stack. This is called the **lobby context**.
- **Execution**: The kernel handles the syscall or IRQ.

#### **Kernel Exit (No Preemption)**
- If the thread is **not preempted**, the kernel restores the **lobby context** and resumes user-mode execution.

#### **Kernel Preemption**
- If the thread is **preempted** in kernel mode, the kernel saves a **new snapshot** of the thread's state (registers, etc.) at the point of preemption.
- This snapshot is used to resume execution later, as the thread may still have pending kernel operations.

#### **Key Points**
- **Lobby Context**: Represents the state at kernel entry (user-to-kernel transition).
- **Kernel Snapshot**: Represents the state at preemption (kernel-to-kernel transition).

---

### **1.2 Kernel Threads**
- Kernel threads **do not have a lobby context** because they do not transition between user and kernel modes.
- **State Saving**: Kernel threads must explicitly save their state when preempted or during scheduler operations.

---

## **2. Preemption and Scheduler Invocation**

### **2.1 Preemption Rules**
- **Syscall Exits**: Always allow preemption (invoke scheduler).
- **IRQ Exits**: Only allow preemption if:
  - The IRQ is a **timeslice timer IRQ**.
  - The IRQ nesting level is **1** (i.e., not nested within another IRQ).
- **Preemption Disabling**: Use `preempt_disable()` to prevent preemption during critical sections. This increments a counter, and preemption is only allowed when the counter is **0**.

#### **IRQ Nesting**
- **Nesting Level**: Tracks the depth of nested IRQs.
- **Preemption Handling**: Preemption is only allowed at nesting level **1** to avoid binding IRQs to specific threads.

---

### **2.2 Scheduler Types**

#### **Timeslice Scheduler**
- **Invocation**: Triggered by the **timeslice timer IRQ**.
- **State Saving**: Always saves the **lobby context** (user or kernel) of the preempted thread.
- **Preemption**: Enforces timeslice-based preemption at syscall exits and IRQ exits (if nesting level is 1).

#### **Passive Preemptive Scheduler**
- **Invocation**: Triggered at **syscall exits**.
- **State Saving**: Explicitly saves a new snapshot of the thread's state (kernel mode).
- **Preemption**: Ensures the highest-priority thread with remaining timeslice is scheduled.

#### **Cooperative Scheduler**
- **Invocation**: Triggered explicitly by threads (e.g., via `yield()`).
- **State Saving**: Explicitly saves a new snapshot of the thread's state (kernel mode).
- **Preemption**: Immediate context switch to the next thread.

---

## **3. Scheduler Operations (Schedops)**

### **3.1 When to Effectuate Immediate Context Switches**

#### **General Rule**
- **Always effectuate schedops immediately**, except in **IRQ context**.

#### **Arguments for "Always"**
- **Simplicity**: A unified approach simplifies the design.
- **Predictability**: Ensures schedops are enforced promptly.
- **Edge Case**: Only one edge case (IRQ context) needs special handling.

#### **Arguments for "Sometimes"**
- **Efficiency**: Avoids redundant context switches (e.g., in passive preemptive mode).
- **Complexity**: Requires additional logic to defer schedops in specific cases.

---

### **3.2 Effectuating Schedops in Different Contexts**

#### **Syscall Context**
- **Cooperative Mode**: Always effectuate immediately.
- **Passive Preemptive Mode**: Effectuate at syscall exit (deferred).
- **Timeslice Mode**: Always effectuate immediately.

#### **IRQ Context**
- **Never effectuate schedops immediately**:
  - Saving a snapshot in IRQ context binds the IRQ to the thread, violating the async nature of IRQs.

#### **Kernel Threads**
- **Always effectuate schedops immediately**:
  - Kernel threads have no syscall exits, so deferred schedops are not possible.

---

## **4. Summary Tables**

### **4.1 Stack and State Management**

| **Thread Type** | **Kernel Entry**                     | **Preemption**                          | **State Saving**                          |
|------------------|--------------------------------------|------------------------------------------|-------------------------------------------|
| **User Thread**  | CPU saves lobby context on stack0.   | Save new snapshot if preempted in kernel. | Lobby context (user) or kernel snapshot.  |
| **Kernel Thread**| No lobby context.                   | Explicitly save kernel snapshot.         | Kernel snapshot only.                     |

---

### **4.2 Preemption Rules**

| **Context**      | **Preemption Allowed**               | **Conditions**                           |
|------------------|--------------------------------------|------------------------------------------|
| **Syscall Exit** | Yes                                  | Always.                                  |
| **IRQ Exit**     | Yes                                  | Only for timeslice timer IRQ at nesting level 1. |
| **Nested IRQ**   | No                                   | Preemption disabled (`preempt_disable()`). |

---

### **4.3 Schedop Effectuation**

| **Scheduler Type** | **Syscall Context**                  | **IRQ Context**                          | **Kernel Threads**                        |
|---------------------|--------------------------------------|------------------------------------------|-------------------------------------------|
| **Cooperative**     | Always immediately.                 | Never.                                   | Always immediately.                       |
| **Passive Preemptive** | Deferred (at syscall exit).       | Never.                                   | Always immediately.                       |
| **Timeslice**       | Always immediately.                 | Never.                                   | Always immediately.                       |

---

## **5. Key Design Principles**

1. **Lobby Context**: Represents the state at kernel entry and is used for resuming user threads.
2. **Kernel Snapshot**: Represents the state at preemption and is used for resuming kernel threads or preempted user threads.
3. **Preemption Rules**: Preemption is allowed at syscall exits and specific IRQ exits (timeslice timer at nesting level 1).
4. **Schedop Effectuation**: Always effectuate schedops immediately, except in IRQ context, to ensure prompt and predictable behavior.

---

## **6. Conclusion**

The design prioritizes **simplicity** and **predictability** by enforcing immediate schedop effectuation in most cases. Special handling is only required for **IRQ context**, where schedops are deferred to maintain the async nature of interrupts. This approach ensures robust and efficient kernel operation across different scheduler types and thread contexts.

