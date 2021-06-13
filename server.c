#include <arpa/inet.h>  //inet_addr
#include <limits.h>
#include <omp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>  //write

char cwd[FILENAME_MAX];

// structure to store the sequences
typedef struct {
  char *sequence;
  char *found_at_reference_address;
  int found_at;
  int length;
  int id;
} sequence_string;

// contar los
int countChar(char *clientMsg) {
  int iCount = 0;
  char ch;
  FILE *ptr = fopen(clientMsg, "r");
  if (ptr == NULL) return -1;
  while ((ch = fgetc(ptr)) != EOF) {
    iCount++;
  }
  iCount += 10;
  fclose(ptr);
  return iCount;
}

// porcentage de secuencias que cubre el genoma de referencia
double getPercentage(const char *reference) {
  int seqCoverage = 0;
  int sizeGenome = strlen(reference);

  if (sizeGenome == 0) return 0;

  for (int i = 0; i < sizeGenome; i++) {
    if (reference[i] == '-') seqCoverage++;
  }

  return ((double)seqCoverage / sizeGenome) * 100.0;
}

static void daemonize() {
  pid_t pid;
  /* Fork off the parent process */
  pid = fork();

  /* An error occurred */
  if (pid < 0) exit(EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0) exit(EXIT_SUCCESS);

  /* On success: The child process becomes session leader */
  if (setsid() < 0) exit(EXIT_FAILURE);

  /* Catch, ignore and handle signals */
  /*TODO: Implement a working signal handler */
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  /* Fork off for the second time*/
  pid = fork();

  /* An error occurred */
  if (pid < 0) exit(EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0) exit(EXIT_SUCCESS);

  /* Set new file permissions */
  umask(027);

  /* Change the working directory to the root directory */
  /* or another appropriated directory */
  if (chdir(getcwd(cwd, sizeof(cwd))) != 0) exit(EXIT_FAILURE);

  /* Close all open file descriptors */
  int x;
  for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
    close(x);
  }

  /* Open the log file */
  openlog("DNAAnalyzer", LOG_PID, LOG_DAEMON);
}

void kill_handler(int signal) {
  syslog(LOG_NOTICE, "Server terminated.");
  closelog();
  exit(0);
}

int main() {
  int socket_desc, new_socket, c;
  struct sockaddr_in server, client;
  char clientMsg[1024] = {0};
  char serverReply[1024] = {0};
  char *serverReplyP;
  char *reference;
  char cOpt;
  sequence_string sequences_to_analyze[1024];
  int referenceSize;
  int sequenceSize;
  char *secuencia;
  int cantSeq = 0;
  int reportSize;

  printf("Starting daemonize\n");
  daemonize();
  syslog(LOG_NOTICE, "DNA Analyzer Server daemon running.");

  while (1) {
    // TODO: Insert daemon code here.
    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
      syslog(LOG_ERR, "Could not create socket");
      return 1;
    }

    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);

    // Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
      syslog(LOG_ERR, "connect error");
      return 1;
    }
    syslog(LOG_NOTICE, "bind done, server running");
    listen(socket_desc, 3);
    syslog(LOG_NOTICE, "Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    while ((new_socket = accept(socket_desc, (struct sockaddr *)&client,
                                (socklen_t *)&c))) {
      syslog(LOG_NOTICE, "Connection accepted");

      while (recv(new_socket, clientMsg, sizeof(clientMsg), 0) > 0) {
        // char clientMsg[1000] = "test_files/";
        int iLen = strlen(clientMsg);
        cOpt = clientMsg[iLen - 1];

        switch (cOpt) {
          case 'R':  // LOAD NEW REFERENCE
            clientMsg[iLen - 1] = '\0';
            // strcat(clientMsg, clientMsg);
            referenceSize = countChar(clientMsg);
            reference = malloc(referenceSize * sizeof(char));

            FILE *ptrR = fopen(clientMsg, "r");

            if (ptrR == NULL)
              syslog(LOG_ERR, "Could no open selected file\n");
            else {
              if (fgets(reference, referenceSize, ptrR) == NULL) {
                syslog(LOG_ERR, "Error trying to read from file");
                snprintf(serverReply, sizeof(serverReply),
                         "Reference could not be loaded");
              } else {
                snprintf(serverReply, sizeof(serverReply),
                         "Reference loaded!\n");
                send(new_socket, serverReply, sizeof(serverReply), 0);
              }
              fclose(ptrR);
            }

            break;

          case 'S':  // LOAD NEW SEQUENCES
            cantSeq = 0;
            clientMsg[iLen - 1] = '\0';

            sequenceSize = countChar(clientMsg);
            secuencia = malloc(sequenceSize * sizeof(char));

            FILE *ptrS = fopen(clientMsg, "r");

            if (ptrS == NULL)
              syslog(LOG_ERR, "Could no open selected file\n");
            else {
              while (fgets(secuencia, sequenceSize, ptrS)) {
                // AQUI HAY GUARDAMOS CADA secuencia EN UN STRUCT
                int read_len = strlen(secuencia);
                if (secuencia[read_len - 1] == '\n') {
                  secuencia[read_len - 1] = 0;
                  secuencia[read_len - 2] = 0;
                }
                int true_len = strlen(secuencia);
                // syslog(LOG_NOTICE, "strlen secuencia = %d", true_len);
                // syslog(LOG_NOTICE, "Secuencia[%d] = %s", cantSeq, secuencia);
                // syslog(LOG_NOTICE, "Secuencia[%d] = %s", cantSeq, secuencia);
                sequences_to_analyze[cantSeq].sequence =
                    strndup(secuencia, true_len);
                sequences_to_analyze[cantSeq].length = true_len;
                sequences_to_analyze[cantSeq].id = cantSeq;
                cantSeq++;
              }
              fclose(ptrS);
            }
            free(secuencia);

            int seq_found = 0;
            int seq_not_found = 0;

#pragma omp parallel for shared(reference) reduction(+ : seq_found)
            for (int i = 0; i < cantSeq; i++) {
              sequences_to_analyze[i].found_at_reference_address =
                  strstr(reference, sequences_to_analyze[i].sequence);
              if (sequences_to_analyze[i].found_at_reference_address != NULL) {
                sequences_to_analyze[i].found_at =
                    sequences_to_analyze[i].found_at_reference_address -
                    reference;
                seq_found = seq_found + 1;
              } else {
                sequences_to_analyze[i].found_at = -1;  //   SEQUENCE NOT FOUND
              }
            }

// Parallel code begins again
#pragma omp parallel for shared(reference)
            for (int i = 0; i < cantSeq; i++) {
              if (sequences_to_analyze[i].found_at_reference_address != NULL) {
                memset(reference + sequences_to_analyze[i].found_at, '-',
                       sequences_to_analyze[i].length * sizeof(char));
              }
            }

            // Sequential code begins again
            seq_not_found = cantSeq - seq_found;
            double percentage = getPercentage(reference);

            char sReport[100000];
            for (int i = 0; i < cantSeq; i++) {
              char sReportLine[99];  // Buffer for each line that must be
                                     // printed in report
              if (sequences_to_analyze[i].found_at == -1) {
                syslog(5, "Secuencia #%d no se encontro\n",
                       sequences_to_analyze[i].id);
                snprintf(sReportLine, sizeof(sReportLine),
                         "Secuencia #%d no se encontro\n",
                         sequences_to_analyze[i].id);
              } else {
                syslog(5, "Secuencia #%d a partir del caracter %d\n",
                       sequences_to_analyze[i].id,
                       sequences_to_analyze[i].found_at);
                snprintf(sReportLine, sizeof(sReportLine),
                         "Secuencia #%d a partir del caracter %d\n",
                         sequences_to_analyze[i].id,
                         sequences_to_analyze[i].found_at);
              }
              strcat(sReport, sReportLine);
              syslog(5, "sReportLine=%s", sReportLine);
            }
            syslog(5, "sReport:\n%s", sReport);

            // Reporte porcentaje
            char sReportLine[990];
            snprintf(sReportLine, sizeof(sReportLine),
                     "Las secuencias cubren el %f%% del genoma de referencia\n"
                     "%d secuencias mapeadas\n"
                     "%d secuencias no mapeadas\n",
                     percentage, seq_found, seq_not_found);
            strcat(sReport, sReportLine);

            reportSize = strlen(sReport);
            serverReplyP = malloc(reportSize * sizeof(char));

            snprintf(serverReplyP, reportSize, "\n%s", sReport);
            send(new_socket, serverReplyP, reportSize, 0);
            free(serverReplyP);
            for (int i = 0; i < cantSeq; i++) {
              free(sequences_to_analyze[cantSeq].sequence);
            }
            break;
          default:
            snprintf(serverReply, sizeof(serverReply),
                     "Sorry, please type '1' or '2', nothing else.\n");
            send(new_socket, serverReply, sizeof(serverReply), 0);
            break;
        }
      }
      free(reference);
    }
    if (new_socket < 0) {
      syslog(LOG_ERR, "accept failed");
      exit(1);
    }
    if (signal(SIGTERM, kill_handler) == SIG_ERR) {
      syslog(LOG_NOTICE, "Error terminating process.");
      exit(1);
    }
  }
}