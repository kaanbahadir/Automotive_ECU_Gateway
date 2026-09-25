# Automotive ECU Telemetry & UDS Gateway Simulation

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Build & Test](https://img.shields.io/badge/CTest-7%2F7%20Passed-brightgreen.svg)]()
[![Standard](https://img.shields.io/badge/ISO-14229%20%7C%2011898-orange.svg)]()
[![Compliance](https://img.shields.io/badge/MISRA--C%2B%2B-Zero%20Heap%20Policy-success.svg)]()

A production-grade, AUTOSAR-inspired Automotive Electronic Control Unit (ECU) Telemetry and Unified Diagnostic Services (UDS) Gateway built with Modern C++17. Designed targeting ISO 26262 functional safety paradigms with strict zero-dynamic-memory allocation (`malloc`/`new`), lock-free circular FIFO buffers, and comprehensive hardware/OS abstraction layers.

---

## 🏛️ Layered Architecture (AUTOSAR Style)