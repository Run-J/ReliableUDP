# Reliable UDP File Transfer System

## Overview
This project implements a **Reliable UDP-based File Transfer System**, designed to address the challenges of reliability, error detection, and retransmission when transferring files over UDP. UDP is inherently unreliable, meaning packets may be lost, arrive out of order, or get duplicated. Our implementation ensures data integrity and reliability using **packet sequencing, acknowledgments (ACKs), retransmission mechanisms, and error detection** via **MD5 checksum validation**.

---

## Features
- **Reliability Mechanism**: Uses sequence numbers and acknowledgments to track lost packets and ensure all data is received correctly.
- **Error Detection with MD5**: Verifies file integrity at the receiver by comparing MD5 checksums before saving the file.
- **Flow Control**: Implements dynamic flow control to adjust the sending rate based on network conditions (RTT-based flow control).
- **Timeout Handling**: Detects lost packets and triggers retransmission if acknowledgments are not received within a timeout window.
- **File Splitting & Reassembly**: Splits large files into **fixed-size blocks (256 bytes per packet)** for transmission and reconstructs them on the receiving side.
- **Performance Tracking**: Measures transfer time and calculates throughput in Mbps.

---

## How Reliable UDP Works
The system is built on top of UDP while adding reliability features typically found in TCP-like protocols.

### Packet Types
The protocol defines two types of packets:
- **Meta Packet (File Metadata)**: Contains filename, file size, total number of blocks, and an MD5 checksum.
- **Block Packet (File Data)**: Contains a sequence number and a data block.

<div align="center">
  <img src="https://github.com/user-attachments/assets/6f423d90-87b8-4d1d-a9c3-5bcb49a41d24" alt="PacketTypes">
</div>

### Protocol Flow
#### Client (Sender) Workflow:
1. The client reads the file, computes its MD5 checksum, and splits it into blocks.
2. Sends a **Meta Packet** containing file details and checksum.
3. Sends each **Block Packet**, tracking sequence numbers.
4. Waits for acknowledgments and retransmits lost packets if needed.
5. Once all blocks are sent and acknowledged, transmission is complete.

#### Server (Receiver) Workflow:
1. Listens for incoming **Meta Packets** and extracts file information.
2. Receives **Block Packets**, storing them in the correct sequence.
3. Acknowledges received packets, prompting the sender to move forward.
4. Upon receiving all blocks, computes an MD5 checksum on the assembled file.
5. Compares the computed MD5 with the sender’s MD5; if they match, the file is saved.
6. Reports errors if any packet is lost after multiple retransmissions or if the MD5 verification fails.

---

## Code Structure
| File                | Description |
|--------------------|--------------------------------------------|
| `ReliableUDP.cpp`    | Implements server and client logic. |
| `FileProcess.cpp/h`  | Handles file operations and MD5 verification. |
| `Protocol.h`         | Defines packet structures. |
| `Net.h`              | Provides networking utilities. |
| `md5.c/h`            | Implements MD5 checksum calculation. |

---

## Reliability Mechanisms in Detail
### Packet Acknowledgment & Retransmission
- Each **Block Packet** includes a **sequence number**.
- The receiver sends back **ACKs** to confirm successful reception.
- If an **ACK is not received within the timeout window**, the sender **retransmits** the missing packet.

### MD5-Based File Integrity Verification
- The **sender computes an MD5 checksum** of the original file before transmission.
- The **receiver computes the MD5** of the received file.
- If the **checksums match**, the file is considered intact; otherwise, an error is reported.

### Flow Control for Efficient Transfer
- Adjusts sending rate dynamically based on **Round Trip Time (RTT)**.
- Uses **Good/Bad Mode**:
  - If RTT is low, increases transmission speed (**Good Mode**).
  - If RTT exceeds a threshold, slows down transmission (**Bad Mode**).

### File Reconstruction Process
- The receiver keeps track of **all received blocks**.
- Once all blocks arrive, they are **reassembled into the original file**.
- The receiver verifies the file’s correctness before saving it.

---

## Performance Measurement
The system calculates **file transfer time** using:
```cpp
auto startTime = chrono::high_resolution_clock::now();
auto endTime = chrono::high_resolution_clock::now();
chrono::duration<double> transferTime = endTime - startTime;
```
Transfer speed is computed as:
```cpp
Transfer Speed (Mbps) = (File Size in Bytes * 8) / (Transfer Time in Seconds * 1,000,000);
```
### Example results from test cases:
| Filename        | Size (Bytes) | Transfer Time (s) | Transfer Speed (Mbps) |
|---------------|-------------|-------------------|-----------------------|
| Test.txt       | 2,115       | 1.521             | 0.011                 |
| Image.jpg      | 144,577     | 30.937            | 0.037                 |
| Report.pdf     | 1,821,573   | 551.778           | 0.026                 |

---

## How to Run
### Server Mode:
Run the program as a server:
```sh
./ReliableUDP
```

### Client Mode:
To send a file from a client to the server:
```sh
./ReliableUDP <Server_IP> <File_To_Transfer>
```
Example:
```sh
./ReliableUDP 192.168.1.100 example.txt
```

---

## Conclusion
This project enhances UDP with reliability mechanisms, making it suitable for **file transfer applications**. By implementing **packet acknowledgment, retransmission, flow control, and error detection with MD5**, we ensure successful and accurate file delivery.

