# Computer Networks: Programming Assignment I
#### By Arnar Freyr Sævarsson and Edda Steinunn Rúnarsdóttir <br/> <br/> <br/>



## Introduction
A simple **Trivial File Transfer Protocol** (TFTP) server was implemented in C programming language using socket API.
The server listens on a server-specified port. It can handle read requests and serve files via TFTP protocol as defined in RFC 1350. It only serves files from a server specified directory and user can request a file by name in that directory.
Implementation and testing of the server took around 25-30 hours and code compiles without memory errors or warnings. 


## Structure explained
Entire implementation is in the file _./src/tftpd.c_ and is structured with functions. An _Internet User Datagram Protocol_ (UDP) socket API is used in implementation as specified in the RCF 1350. File has commented header with assignment information and authors. File is structually divided into sections, each preceded by a commented header: <br/>

* **Functions and constant declarations**. Declarations include comments that define purpose for each declaration
* **Main function**. Contains all functionality executed by program.
* **Functions implementations**. Contain all logic for resolving requests.

Socket initializing, optimization and binding is located in the main function and is executed first along with initialization of empty packets. While server is still transmitting packets to client an iterative process is run of receiving packets of 516 bytes, resolving received packets and responding to client with packets. <br/>

Packets' operation codes are used to identify type of received packets and a process of resolving them is implemented in a function, _resolve_packet( )_. Appropriate actions are taken for each type of packet for each iteration.

* **A RRQ packet:** If a read request packet is received for a file, a file path validation is run. The validation returns either a valid or an invalid file (NULL file) back to the main function. An invalid file compells the main function to construct and transmit an error packet to client, terminating the transfer process. A valid file returns a _FILE *_ variable type into main, starting the transmission of data packets to the client. Which are formatted into DATA packets of 516 bytes (header + data), before transmission.
* **A WRQ packet:** If a write request packet is receieved a potential error value is set, thus compelling the main function to construct and transmit an error packet, prohibiting client to upload to server. This is due to our server not allowing client upload (factor explored in below section).
* **AN ACK packet:** Upon receiving an acknowledgement packet, server validates that the packet echoes latest data packet block number. If not, last packet was not recieved by client. Potential error value is set for main function to resend previous packet to client. If block number is echoed, value increases thus enabling transmission of next data packet in next iteration
* **AN ERROR packet:** An error packet sets potential error value compelling the main function to construct and transmit an error packet to client to terminate client's process. Server's transfer process is also terminated. 

A packet with an operation code that does not define any of the above packet types results in setting a potential error value, thus compelling the main function to construct and transmit an error packet to client. Many helper functions aid each process for better understandable structure. <br/>




## Implementation explained
Implementation design was largely adapted to the specification of the TFTP protocol behaviour as defined in RFC 1350, but differs from it because the design prohibits client from sending write requests to server. This changes handling of such request by the server and changes the usability of the mode string that are included in RRQ and WRQ packets.

* ### Handling of RRQ / WRQ's mode string
A mode in TFTP is used to specify how files are written <br/>
It supports three modes:

+ _netascii_: Modified version of ascii contains 8 bits instead of the regular 7
+ _octet_: raw 8 bit bytes
+ _mail netascii_: (obsolete and is not implemented) <br/>

The server has no need for the mode string in the RRQ recieved since write requests aren't allowed, we only read from a file to a message that is sent to the client. If the server implemented uploading files to the server (WRQ) it would have to handle the mode to write to the file on the server. The client specifies what mode to use when it sends a RRQ and the server recieves it, but doesn't handle it in any specific way. The client only uses it to specify how it interprets the data and writes it to the file.<br/>

* ### Upload requests from client to server
Upload requests (write requests) are prohibited on our TFTP server. Therefore, our server specifically handles such requests. Upon receiving a WRQ the server transmits an error packet to client. The error packet has the error code 0 (for an unspecified error as no other code properly defines the error) and an error message stating that such request is prohibited. This ensures client feedback and that transfer process terminates for client. Process is also terminated for server in this case and same feedback is printed for server.
