# CalculatorThatStaysOnline

A lightweight, zero-framework HTTP/1.1 persistent-connection calculator server implemented in modern C++ (C++20) using raw POSIX sockets.

This project was built to address the fundamental networking challenge introduced with HTTP/1.1 keep-alive: **"Where does this request end and the next one begin?"**

---

## Table of Contents

- [The Challenge: HTTP/1.0 vs. HTTP/1.1](#the-challenge-http10-vs-http11)
- [Key Features](#key-features)
- [API Specification](#api-specification)
  - [Supported Operations](#supported-operations)
  - [Error Handling](#error-handling)
- [Architecture & Internal Design](#architecture--internal-design)
  - [Request Framing and Buffering](#request-framing-and-buffering)
  - [Project Structure](#project-structure)
- [Requirements & Dependencies](#requirements--dependencies)
- [Building the Project](#building-the-project)
- [Running the Server](#running-the-server)
- [Testing & Verification](#testing--verification)
  - [Python Verification Script (Single Socket Keep-Alive)](#python-verification-script-single-socket-keep-alive)
  - [Testing with `curl`](#testing-with-curl)
  - [Testing with `netcat` / `nc`](#testing-with-netcat--nc)

---

## The Challenge: HTTP/1.0 vs. HTTP/1.1

In **HTTP/1.0**, servers typically serviced a single request per TCP connection and closed the socket immediately after transmitting the response. Delimiting the response and the request boundary was simple: it ended at EOF upon socket termination.

In **HTTP/1.1**, persistent connections (`Connection: keep-alive`) are the default:
- A single TCP handshake is performed.
- Multiple HTTP requests and responses flow sequentially across the exact same socket.
- The server cannot rely on EOF to signal the end of a request.
- The server must read from the stream, locate the header delimiter (`\r\n\r\n`), inspect the `Content-Length` header if a body is present, consume *exactly* that number of body bytes, and preserve any leftover bytes in an internal client buffer for subsequent requests.

`CalculatorThatStaysOnline` implements this stream buffering and request framing from scratch without external dependencies.

---

## Key Features

- **No External Frameworks**: Built directly on POSIX socket APIs (`socket`, `bind`, `listen`, `accept`, `recv`, `send`).
- **Persistent Connection Support**: Keeps client connections open across multiple requests until the client closes the connection or disconnects.
- **Custom HTTP/1.1 Parser**:
  - Handles request lines (method, URI path, query string splitting, HTTP version).
  - Case-insensitive header parsing (e.g. `Host`, `Content-Length`).
  - Request boundary detection and stream framing.
- **RFC-Compliant Validations**:
  - Requires the mandatory HTTP/1.1 `Host` header.
  - Method validation (only `GET` is supported for calculations; other methods yield `405 Method Not Allowed`).
  - Path and query parameter validation.
- **Robust Socket Handling**:
  - `SO_REUSEADDR` / `SO_REUSEPORT` enabled for rapid restarts without `TIME_WAIT` binding conflicts.
  - `SIGPIPE` ignored via `signal(SIGPIPE, SIG_IGN)` and `MSG_NOSIGNAL` flag used during `send()` to prevent abrupt crashes if a client disconnects mid-transmission.

---

## API Specification

All arithmetic operations are accessed via HTTP `GET` requests with query parameters `a` and `b`.

### Supported Operations

| Method | Path | Query Parameters | Example Request | Expected Status | Example Body |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `GET` | `/add` | `a` (integer), `b` (integer) | `GET /add?a=2&b=3 HTTP/1.1` | `200 OK` | `5` |
| `GET` | `/sub` | `a` (integer), `b` (integer) | `GET /sub?a=10&b=4 HTTP/1.1` | `200 OK` | `6` |
| `GET` | `/mul` | `a` (integer), `b` (integer) | `GET /mul?a=6&b=7 HTTP/1.1` | `200 OK` | `42` |
| `GET` | `/div` | `a` (integer), `b` (integer) | `GET /div?a=9&b=3 HTTP/1.1` | `200 OK` | `3` |

### Error Handling

The server returns standard HTTP status codes for invalid inputs, missing headers, or unsupported methods:

| Scenario | Request Example | Status Code | Reason |
| :--- | :--- | :--- | :--- |
| **Division by Zero** | `GET /div?a=1&b=0 HTTP/1.1` | `400 Bad Request` | Division by zero is undefined. |
| **Non-Integer Parameter** | `GET /add?a=x&b=3 HTTP/1.1` | `400 Bad Request` | Parameters `a` and `b` must be valid signed integers. |
| **Missing Parameter** | `GET /add?a=2 HTTP/1.1` | `400 Bad Request` | Both `a` and `b` parameters are required. |
| **Missing `Host` Header** | `GET /add?a=2&b=3 HTTP/1.1` *(without `Host:`)* | `400 Bad Request` | Required by HTTP/1.1 specification. |
| **Unknown Operation** | `GET /pow?a=2&b=8 HTTP/1.1` | `404 Not Found` | `/pow` is not a recognized endpoint. |
| **Disallowed Method** | `POST /add HTTP/1.1` | `405 Method Not Allowed` | Only `GET` is supported for calculation routes. |

---

## Architecture & Internal Design

### Request Framing and Buffering

TCP is a continuous byte stream without application-level message boundaries. Incoming bytes can arrive in arbitrary chunk sizes, and a single `recv()` call might receive:
1. Only a partial header.
2. A complete request.
3. A complete request plus the beginning of the next pipelined request.

To handle this cleanly:
- `Server::m_client_buffer` accumulates raw bytes received from `recv()`.
- `HTTP::Parser::isRequestComplete()` inspects the buffer for `\r\n\r\n`. If found, it parses the `Content-Length` header (if present) to determine the exact byte length `N` of the full message.
- If the buffer contains at least `N` bytes, `Server::read_request()` extracts the first `N` bytes into an output request string and removes them from the buffer, leaving any remaining bytes (`N + 1`, etc.) for subsequent calls.

```
TCP Stream: [  Request 1 (Headers + Body)  ][  Request 2 (Headers) ... ]
             \____________________________/  \________________________/
                 Consumed by Parser            Kept in Client Buffer
```

### Project Structure

- [`main.cpp`](file:///home/epicman/Projects/repos/CalculatorThatStaysOnline/main.cpp): Connection loop, routing, query string validation, arithmetic execution, and response dispatch.
- [`server.h`](file:///home/epicman/Projects/repos/CalculatorThatStaysOnline/server.h) / [`server.cpp`](file:///home/epicman/Projects/repos/CalculatorThatStaysOnline/server.cpp): Encapsulates POSIX TCP socket creation, options, binding, listening, client connection acceptance, read buffering, and write loops.
- [`httpparser.h`](file:///home/epicman/Projects/repos/CalculatorThatStaysOnline/httpparser.h) / [`httpparser.cpp`](file:///home/epicman/Projects/repos/CalculatorThatStaysOnline/httpparser.cpp): HTTP message parser, request boundary checker, header and query parameter extractor.
- [`httpresponse.h`](file:///home/epicman/Projects/repos/CalculatorThatStaysOnline/httpresponse.h) / [`httpresponse.cpp`](file:///home/epicman/Projects/repos/CalculatorThatStaysOnline/httpresponse.cpp): Fluent builder for constructing well-formed HTTP/1.1 responses with status line, `Content-Type`, `Content-Length`, and payload.
- [`CMakeLists.txt`](file:///home/epicman/Projects/repos/CalculatorThatStaysOnline/CMakeLists.txt): CMake build configuration requiring C++20.

---

## Requirements & Dependencies

- **Operating System**: Linux or POSIX-compliant OS (uses `<sys/socket.h>`, `<arpa/inet.h>`, `<unistd.h>`).
- **Compiler**: Modern C++ compiler supporting C++20 (e.g. GCC 10+, Clang 11+).
- **Build System**: CMake 3.16 or newer and Make / Ninja.

---

## Building the Project

Use CMake to generate the build files and compile the executable:

```bash
# Configure the build directory
cmake -B build -S .

# Compile the target
cmake --build build
```

The resulting binary will be created at `build/CalculatorThatStaysOnline`.

---

## Running the Server

Run the executable directly. By default, it listens on port **8080**:

```bash
./build/CalculatorThatStaysOnline
```

You can optionally specify a custom port as the first argument:

```bash
./build/CalculatorThatStaysOnline 9000
```

---

## Testing & Verification

### Python Verification Script (Single Socket Keep-Alive)

To verify that the server satisfies the keep-alive requirement (1 TCP handshake, multiple requests over the same socket), run the following test script:

```python
import socket

PORT = 8080

requests = [
    ("GET /add?a=2&b=3 HTTP/1.1\r\nHost: localhost\r\n\r\n", "200", "5"),
    ("GET /sub?a=10&b=4 HTTP/1.1\r\nHost: localhost\r\n\r\n", "200", "6"),
    ("GET /mul?a=6&b=7 HTTP/1.1\r\nHost: localhost\r\n\r\n", "200", "42"),
    ("GET /div?a=9&b=3 HTTP/1.1\r\nHost: localhost\r\n\r\n", "200", "3"),
    ("GET /div?a=1&b=0 HTTP/1.1\r\nHost: localhost\r\n\r\n", "400", ""),
    ("GET /add?a=x&b=3 HTTP/1.1\r\nHost: localhost\r\n\r\n", "400", ""),
    ("GET /pow?a=2&b=8 HTTP/1.1\r\nHost: localhost\r\n\r\n", "404", ""),
    ("POST /add HTTP/1.1\r\nHost: localhost\r\n\r\n", "405", ""),
    ("GET /add?a=2&b=3 HTTP/1.1\r\n\r\n", "400", ""),  # Missing Host header
]

s = socket.create_connection(("localhost", PORT))

for req, expected_status, expected_body in requests:
    s.sendall(req.encode())
    
    # Read response
    resp = b""
    while b"\r\n\r\n" not in resp:
        chunk = s.recv(1024)
        if not chunk:
            raise RuntimeError("Socket closed prematurely by server!")
        resp += chunk
        
    header_part, body_part = resp.split(b"\r\n\r\n", 1)
    status_line = header_part.split(b"\r\n")[0].decode()
    
    # Check Content-Length to read remainder of body if needed
    for line in header_part.split(b"\r\n"):
        if line.lower().startswith(b"content-length:"):
            length = int(line.split(b":")[1].strip())
            while len(body_part) < length:
                body_part += s.recv(1024)
            break
            
    print(f"{req.splitlines()[0]} -> {status_line} (body: '{body_part.decode()}')")

s.close()
print("All requests completed successfully across a single persistent TCP socket!")
```

### Testing with `curl`

Test endpoints using `curl`:

```bash
# Valid calculations
curl -v "http://localhost:8080/add?a=15&b=27"
curl -v "http://localhost:8080/sub?a=100&b=42"
curl -v "http://localhost:8080/mul?a=7&b=8"
curl -v "http://localhost:8080/div?a=81&b=9"

# Division by zero (returns 400 Bad Request)
curl -v "http://localhost:8080/div?a=10&b=0"

# Missing parameter (returns 400 Bad Request)
curl -v "http://localhost:8080/add?a=10"

# Unknown route (returns 404 Not Found)
curl -v "http://localhost:8080/pow?a=2&b=8"

# Disallowed method (returns 405 Method Not Allowed)
curl -v -X POST "http://localhost:8080/add"
```

### Testing with `netcat` / `nc`

You can manually pipe HTTP requests through `nc`:

```bash
printf "GET /add?a=5&b=10 HTTP/1.1\r\nHost: localhost\r\n\r\nGET /mul?a=3&b=4 HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080
```
