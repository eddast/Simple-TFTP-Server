/*  =================================================================
 *
 * 		T-409-TSAM Computer Networks
 * 		Reykjavik University
 * 		Programming Assignment 1: TFTP
 * 		Assignment Due: 11.09.2017
 * 		Authors: Arnar Freyr and Edda Steinunn
 *
    ================================================================= */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>






/**************************************************
 *      FUNCTION DECLARATIONS AND CONSTANTS
 **************************************************/


enum OP_code { 	RRQ = 1, WRQ, DATA, ACK, ERROR };	// Operation codes to define packets
FILE* resolve_packet (	char packet[],			// Defines packet and takes appropriate action
			unsigned short* blockno,
			unsigned short* potential_err,
			FILE* file,
                        char filename[],
			char mode[],
                        char folder[],
			char IP[] );
void terminate_prog( );					// Terminates program upon an error
FILE* resolve_rrq(	char filename[], char mode[],	// Resolves read request and opens file/s
			char packet[], char folder[] );
FILE* open_file	(	char filename[],		// Opens file given filename and folder
			char folder[] );
void parse_path(	char filename[],		// Gives absolute path given directory and folder
			char folder[],
			char full_path[]);
int bundle_data(	unsigned short blockno,		// Formats data appropriately from file into packet
			FILE* file, char packet[] );
void get_error_packet(	unsigned short err_code,	// Formats an error packet appropriately
			char err_msg[], char packet[] );
int blockno_acknowl( 	char packet[],			// Checks for echo of data packet blocknumber by ack packet
			unsigned short blockno );
int validate_filename( 	char filename[] );		// Checks if client stays in directory specified by server



/***********************************
 *	   MAIN FUNCTION
 ************************************/


int main ( int argc, char *argv[] ) {

	// Exactly two command line arguments need to be sent
	if ( argc == 3 ) {

		// Command line arguments valid: feedback
		printf( "Server listening on port: %s\n", argv[1]);

		// Initialization of recieved packet from server
		// Is 4B + 512B = 516B (header and data sections)
		// Block number to keep track of data packets consecutively
		char packet[516] = "\0"; char prev_packet[516] = "\0";
		unsigned short blockno = 1;

		// Program times out when value reaches 10 tries on same packet
		unsigned short timeout_cnter = 0;

		// Creating a socket using default protocol
		// Server initialized/null reseted
		// Then defining domain that server can communicate with
		int sockfd; struct sockaddr_in server, client;
		sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
		memset( &server, 0, sizeof( server ) );
		server.sin_family = AF_INET;

		// Converting arguments from host byte order to network byte order
		// Then binding address to socket using arguments
		server.sin_addr.s_addr = htonl( INADDR_ANY );
		server.sin_port = htons( atoi( argv[1] ) );
		bind( sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server) );

		// Initializing filename, mode and filestream
		char filename[50] = "\0"; char mode[50] = "\0"; FILE* file = NULL;

		int packet_size = 512;
		// Loop while transmitting
		while ( packet_size == 512) {

			 // Receive (at least) one byte less than declared
			 // Making room for the NULL-terminating character
			socklen_t len = (socklen_t) sizeof(client);
			ssize_t n = recvfrom	( sockfd, packet, sizeof(packet) - 1, 0,
						  (struct sockaddr *) &client, &len );
			packet[n] = '\0';

			// Potential error code
			// Defines an error if an error exists
			// Default error type: file not found
			unsigned short potential_err = 1;

			// Get type of packet
			// Calls function to resolve packet
			file = resolve_packet( packet, &blockno, &potential_err,
						file, filename, mode, argv[2],
						inet_ntoa( client.sin_addr ) );

			// If file is NULL, error occurred while reading file
			// Or a packet could not be resolved
			if ( file == NULL) {

				// Form an error message appropriate to error type
				char err_msg[512] = "\0";
				if ( potential_err == 1 ) {
					strcat( err_msg, "ERROR: File name and/or directory could not be resolved" );
				} else if ( potential_err == 3 ) {
					potential_err = 0;
					strcat ( err_msg, "ERROR: Uploading files is not supported on this TFTP Server");
				} else {
					strcat( err_msg, "ERROR: Unable to resolve packet" );
				}

				// Use code and message to bundle an error packet
				get_error_packet( potential_err, err_msg, packet );

				// Feedback printed for server
				printf("%s\n", err_msg);

				// Sends error packet to client, then terminates
				sendto( sockfd, packet, (size_t) packet_size + 4, 0,
					(struct sockaddr *) &client, len );
				terminate_prog( );
			}
			if ( potential_err != 2 ) {

				// Passes data into corresponding packet
				// IF data was previously sent
				packet_size = bundle_data( blockno, file, packet );
				*prev_packet = *packet;

				// No potential error, timeout value reset
				timeout_cnter = 0;
			} else {

				// If potential_err is 2, this means that we need to resent packet
				// So, packet becomes previous packet before sendoff
				*packet = *prev_packet; timeout_cnter++;

				// If we have tried to resend same packet 4 times WE GIVE UP ALL HOPE :(
				// Send error packet with appropriate message to client and terminate transfer
				if (timeout_cnter >= 4){

					get_error_packet( 0, "ERROR: Transfer timed out", packet );
					sendto( sockfd, packet, (size_t) packet_size + 4, 0,
                                              	(struct sockaddr *) &client, len );
					terminate_prog( );
				}
			}

			// Sends data packet to client
			sendto(	sockfd, packet, (size_t) packet_size + 4, 0,
             				(struct sockaddr *) &client, len );
		}

		// Feedback is love
		printf( "SUCCESS: File transferred\n" );
		if ( file ) { fclose( file ); }

	} else { printf( "ERROR: Incorrect number of parameters\n" ); terminate_prog( ); }

	return 0;
}






/******************************************
 *       IMPLEMENTATION OF FUNCTIONS
 ******************************************/

// Takes in operation code and packet
// Determines how to resolve packet
FILE* resolve_packet (	char packet[],
			unsigned short* blockno,
			unsigned short* potential_err,
			FILE* file,
			char filename[],
			char mode[],
			char folder[],
			char IP[] ) {

	// Get operation code of sent packet for further action
	unsigned short OP_code = packet[1];

	switch ( OP_code ) {

		// DATA packets are irrelevant for a server
		// Thus we only focuss on RRQ, ACK and ERROR packets
		// Special case: WRQ - illegal
		case RRQ:
			// Read request means deciding whether file can be opened
			printf("Read request received from client: %s\n", IP);
			file  = resolve_rrq( filename, mode, packet, folder );
			break;
		case WRQ:
			// A write request yields an error
			// Sets potential error code so main will terminate
			printf("Write request received from client: %s\n", IP);
			file = NULL;
			(*potential_err) = 3;
			break;
		case ACK:
			// Only move on to next packet if ACK packet echoes blocknumber
			if ( blockno_acknowl( packet, blockno[0] ) ) { (*blockno)++; }
			else { (*potential_err) = 2; } break;

		case ERROR:
			// Print error code and message and terminate transfer
			printf("Error code %d: %s\n", packet[3], (packet + 4));

		// Any other OP code is undefined for a packet
		// Thus should result in termiation of the program
		default:
			(*potential_err) = 0; file = NULL; break;

	}

	// Returns either modified file or original parameter
	return file;
}

FILE* resolve_rrq( char filename[], char mode[], char packet[], char folder[] ) {

	// Start parse after op_code
	int file_idx = 2;

	// Filename goes from op code to next null terminator
	 // Then terminate string after filling in char array
	for ( ; packet[file_idx] != '\0'; file_idx++ ) {
		filename[file_idx - 2] = packet[file_idx];
	}
	filename[file_idx] = '\0';

	// Mode name goes from null terminator to the next
	// Then terminate string after filling in char array
	file_idx++; int i = 0;
	for ( ; packet[file_idx] != '\0'; file_idx++ ) {
		mode[i] = packet[file_idx]; i++;
	}
	mode[i] = '\0';

	// Filename must be in specified directory
	// Navigating out of directory is illegal
	FILE* file = NULL;
	if( validate_filename( filename ) ) {	

		// Generates valid open file if file can be opened
		file = open_file(filename, folder);
	}

	// Feedback that transmission is starting if file is valid
	if ( file ) { printf( "Starting transmission of packets\n" ); }

	return file;
}

// Opens file specified by command line arguments
// Terminates program if no corresponding file is found
FILE* open_file( char filename[], char folder[] ) {

	char full_path [100] = "\0";
	parse_path(filename, folder, full_path);
	printf("Path requested: \"%s\"\n", full_path);
	FILE *file = fopen( full_path, "r" );
	if ( file ) { return file; }
	else { return NULL; }
}

// Sends error packet to client
void get_error_packet( unsigned short err_code, char err_msg[], char packet[] ) {

	// Creating header for error packet
	packet[0] = 0; packet[1] = ERROR;	// OP code 2B
	packet[2] = 0; packet[3] = err_code;	// Error code 2B

	// Writing error message into appropriate index
	unsigned int i = 4;
	for ( ; err_msg[i - 4] != '\0'; i++ ) {
		packet[i] = err_msg[i - 4];
	}

	// NULL terminate packet
	packet[i] = '\0';
}

// Constructs a path - path of command line passed folder in parent directory
// Uses filename requested by client
void parse_path( char filename[], char folder[], char full_path[]) {

	// Constructing string: ../$FOLDER/$FILE
	strcat	( full_path,	"../" );
	strcat	( full_path,	folder );
	strcat	( full_path,	"/" );
	strcat	( full_path,	filename );
}

// Places data in appropriate format i.e. in a packet of <= 512B
// A packet of size  < 512B indicate that transmission is done
// Returns packet size
int bundle_data( unsigned short blockno, FILE* file, char packet[] ) {

	// Manipulating bits to convert 2B short into two characters
	char _one = blockno >> 8; char _two = blockno;

	// Formatting header to show operation code and blocknumber
	packet[0] = 0; packet[1] = DATA; 	// OP code: 2B
	packet[2] = _one; packet[3] = _two;	// Blockno: 2B

	// Read in data from header and filling packet (512B - 4B - '\0' = 507B)
	// Then using fseek to modify file so that it jumps 508B forward if end wasn't reached
	int read_elems = fread( packet + 4, 1, 512, file );

	// Return size of packet
	return read_elems;
}

// "Boolean" function, checking if blocknumber was acknowledged
int blockno_acknowl( char packet[], unsigned short blockno ) {

	// Manipulating bits so that first two bytes of packet fit into UShort
	unsigned short blockno_echo = ( packet[2] << 8 ) | ( packet[3] & 0xFF );
	if (blockno_echo == blockno) { return 1; }
	return 0;
}

// "Boolean" function, checks if user tries to navigate out of directory
// User needs to stay in directory specified by server
int validate_filename( char filename[] ) {
	if( filename ){
		if ( strstr(filename, "/") == NULL ) { return 1; }
	}
	return 0;
}

// Sends error to client and terminates transfer
void terminate_prog( ) { printf( "Transfer terminating\n" ); exit( 1 ); }
