# High-Performance C++ WebSocket Chat Server

A robust, multithreaded WebSocket chat application demonstrating advanced operating system and networking concepts, built in C++ on Linux using Boost.Asio/Beast and SQLite for persistent storage.

---

## Overview

This project implements a scalable, low-latency WebSocket server supporting user authentication, multiple chat rooms, and message persistence. The system is composed of:

- **Frontend:** React-based SPA featuring login, chat room listing, and real-time chat interfaces.
- **Backend:**  
  - C++ WebSocket server leveraging Boost.Asio and Beast for asynchronous, non-blocking socket I/O and event-driven reactor patterns.  
  - SQLite database integration with custom concurrency control ensuring data integrity under high load.  
  - Secure user authentication using SHA-256 hashed passwords and session management.

---

## Features

- **Multithreaded WebSocket Server:** Efficient thread-pool design handling simultaneous client connections with minimal latency.  
- **Robust Concurrency Control:** Custom file-lock detection and retry/backoff logic to avoid database contention in SQLite.  
- **Secure Authentication:** Password hashing and per-connection state tracking to maintain secure sessions.  
- **Persistent Chat Storage:** Messages saved and retrieved from SQLite with transactional integrity.  
- **Modern Frontend:** React app for seamless user experience with routing for login, dashboard, and chat rooms.

---

## Technical Highlights

- **Boost.Asio / Beast:** Deep integration of asynchronous I/O and HTTP/WebSocket protocols with event-driven design patterns.  
- **POSIX Sockets & Threading:** Expert use of raw socket APIs, mutexes, and condition variables for concurrency and synchronization.  
- **Database Locking Mechanism:** Handling SQLite `SQLITE_BUSY` states with custom retry strategies to prevent race conditions.  
- **Cryptographic Hashing:** Implementation of SHA-256 for password security aligned with OS-level security best practices.

---

## Getting Started

### Prerequisites

- Linux environment (Ubuntu or similar)
- C++17 compatible compiler
- Boost libraries (Asio, Beast)
- SQLite3 development headers
- Node.js and npm (for frontend)

### Build & Run Backend

```bash
mkdir build && cd build
cmake ..
make
./websocket_server
