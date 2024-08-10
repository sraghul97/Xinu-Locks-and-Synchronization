# Xinu-Locks-and-Synchronization

This repository contains the implementation of various synchronization mechanisms in the Xinu operating system as part of the ECE465/565 Operating Systems Design course. The project covers the following aspects:

1. **Atomic Test-and-Set Implementation**: Provides an assembly implementation of the `test_and_set` function using the XCHG x86 instruction.
2. **Spinlock Implementation**: Implements a basic spinlock using the `test_and_set` function.
3. **Lock with Sleep**: Implements a lock that minimizes busy waiting by putting processes to sleep, including primitives for park, unpark, and setpark.
4. **Active Lock with Deadlock Detection**: Enhances the lock implementation with automatic deadlock detection and reporting.
5. **Priority Inheritance Lock** (ECE565 Only): Implements a priority inheritance mechanism to handle priority inversion.

Additionally, the repository includes test cases to verify the correctness and performance of the implemented synchronization mechanisms. Detailed reports and documentation are provided to describe the implementation approach and test case outcomes.
