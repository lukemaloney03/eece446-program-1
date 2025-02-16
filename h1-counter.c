/*
 * Group Members: Luke Maloney, Ezaz Ahamad Mohammad Abdul
 * Class: EECE 446 - Intro to Computer Networks
 * Semester: Spring 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define SERVER "www.ecst.csuchico.edu"
#define PORT "80"
#define REQUEST "GET /~kkredo/file.html HTTP/1.0\r\n\r\n"

/*
 * Lookup a host IP address and connect to it using service. Arguments match the first two
 * arguments to getaddrinfo(3).
 *
 * Returns a connected socket descriptor or -1 on error. Caller is responsible for closing
 * the returned socket.
 */
int lookup_and_connect(const char *host, const char *service);

int checkChunk(char *chunkSizeString);
int sendReq(int s);
void getAndProcess(int s, int chunkSize);

// Count occurrences of "<h1>" that are fully contained in buffer.
int countTags(char *buffer, ssize_t bytesInBuffer) {
    int tagCount = 0;
    char *searchPointer = buffer;
    while ((searchPointer = strstr(searchPointer, "<h1>")) != NULL) {
        // Only count if tag fits within buffer.
        if ((searchPointer - buffer) + 4 <= bytesInBuffer)
            tagCount++;
        searchPointer += 4;
    }
    return tagCount;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <chunk size>\n", argv[0]);
        return 1;
    }
    int chunkSize = checkChunk(argv[1]);
    if (chunkSize == -1)
        return 1;

    int s = lookup_and_connect(SERVER, PORT);
    if (s == -1)
        return 1;
    if (sendReq(s) == -1) {
        close(s);
        return 1;
    }
    getAndProcess(s, chunkSize);

    close(s);
    return 0;
}

// Check chunk size is within bounds.
int checkChunk(char *chunkSizeString) {
    int chunkSize = atoi(chunkSizeString);
    if (chunkSize <= 4 || chunkSize > 1000) {
        printf("Error: Chunk size should be between 5 and 1000 bytes.\n");
        return -1;
    }
    return chunkSize;
}

// Send HTTP request, making sure all bytes get sent.
int sendReq(int s) {
    size_t bytesSent = 0;
    size_t requestLength = strlen(REQUEST);
    // Continue sending until all bytes are sent.
    while (bytesSent < requestLength) {
        ssize_t sendResult = send(s, &REQUEST[bytesSent], requestLength - bytesSent, 0);
        if (sendResult == -1) {
            perror("send() error");
            return -1;
        }
        bytesSent += sendResult;
    }
    return 0;
}

// Get data in chunks, count <h1> tags, and track total bytes.
void getAndProcess(int s, int chunkSize) {
    char buffer[chunkSize + 1];
    int totalBytesReceived = 0, totalH1Tags = 0;
    int bytesRead;
    ssize_t bytesReceived;

    while (1) {
        bytesRead = 0;
        // Keep reading until we fill a chunk or connection closes.
        while (bytesRead < chunkSize) {
            bytesReceived = recv(s, buffer + bytesRead, chunkSize - bytesRead, 0);
            if (bytesReceived < 0) {
                perror("recv() error");
                return;
            }
            if (bytesReceived == 0)
                break;
            bytesRead += bytesReceived;
        }
        if (bytesRead == 0)
            break; // No more data
        totalBytesReceived += bytesRead;
        buffer[bytesRead] = '\0';
        totalH1Tags += countTags(buffer, bytesRead);
        if (bytesRead < chunkSize)
            break; // Last chunk
    }
    
    printf("Number of <h1> tags: %d\n", totalH1Tags);
    printf("Number of bytes: %d\n", totalBytesReceived);
}


int lookup_and_connect( const char *host, const char *service ) {
	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;

	/* Translate host name into peer's IP address */
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if ( ( s = getaddrinfo( host, service, &hints, &result ) ) != 0 ) {
		fprintf( stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror( s ) );
		return -1;
	}

	/* Iterate through the address list and try to connect */
	for ( rp = result; rp != NULL; rp = rp->ai_next ) {
		if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
			continue;
		}

		if ( connect( s, rp->ai_addr, rp->ai_addrlen ) != -1 ) {
			break;
		}

		close( s );
	}
	if ( rp == NULL ) {
		perror( "stream-talk-client: connect" );
		return -1;
	}
	freeaddrinfo( result );

	return s;
}
