#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define BUFSIZE 100
#define ALPHABET_SIZE 26

enum BufferStatus {
    Empty = 0,
    Filled = 1
};

const char* MESSAGE_HEADER_EMPTY = "empty";
const char* MESSAGE_HEADER_FILLED = "filled";

int k;
int badApple;
int messageNode;
char message[BUFSIZE];

void signalHandler(int sig) {
    printf("Received signal %d, shutting down gracefully\n", sig);
    exit(0);
}

void receiveMessage(int readPipe, int nodeNum, int writePipe) {
    char buffer[BUFSIZE];
    int messageIntendedForNode = 0;
    int readResult = read(readPipe, buffer, BUFSIZE);
    if (readResult < 0) {
        perror("read");
        exit(1);
    }
    printf("Node %d received message: %s\n", nodeNum + 1, buffer);
    if (nodeNum == badApple) {
        // Modify the message
        for (int i = 0; i < BUFSIZE; i++) {
            buffer[i] = rand() % ALPHABET_SIZE + 'a';
        }
        printf("Node %d (bad apple) modified message to: %s\n", nodeNum + 1, buffer);
    }
    if (buffer[0] == nodeNum + '0') {
        // This node is the intended recipient
        messageIntendedForNode = 1;
        printf("Node %d is the intended recipient\n", nodeNum + 1);
        strcpy(buffer, MESSAGE_HEADER_EMPTY);
    }
    if (messageIntendedForNode == 0) {
        // Send message to the next node after a delay
        sleep(1);
        printf("Node %d sending message to node %d\n", nodeNum + 1, (nodeNum + 1) % k + 1);
        int writeResult = write(writePipe, buffer, BUFSIZE);
        if (writeResult < 0) {
            perror("write");
            exit(1);
        }
    }
}

int main(int argc, char *argv[]) {
    printf("Enter the value for k: ");
    scanf("%d", &k);
    int pipes[k][2];

    for (int i = 0; i < k; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    signal(SIGINT, signalHandler);

    for (int i = 0; i < k; i++) {
        int pid = fork();
        if (pid == 0) {
            // Child process
            while (1) {
                int readPipe = pipes[(i + k - 1) % k][0];
                int writePipe = pipes[i][1];
                receiveMessage(readPipe, i, writePipe);
            }
        } else if (pid > 0) {
            // Parent process
            continue;
        } else {
            // Error
            perror("fork");
            exit(1);
        }
    }

    // Assign one of the nodes to be a bad apple
    badApple = rand() % k;
    printf("Node %d is the bad apple\n", badApple + 1);

    printf("Enter the message to send: ");
    scanf("%s", message);
    printf("Enter the node to send the message to: ");
    scanf("%d", &messageNode);

    // Send the message
    write(pipes[0][1], message, BUFSIZE);

    // Wait for all child processes to finish
    for (int i = 0; i < k; i++) {
        wait(NULL);
    }

    return 0;
}

