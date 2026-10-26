// Copyright (c) 2020-2025 CloudMakers, s. r. o. & Rumen G. Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>
//      & Rumen G. Bogdanovski <rumenastro@gmail.com>

#ifndef EXAMPLE_PROCESS_H
#define EXAMPLE_PROCESS_H

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <indigo/indigo_client_xml.h>

typedef struct {
	indigo_device *adapter;
	atomic_bool stopped;
} example_parser;

static void *example_parse(void *data) {
	example_parser *parser = data;
	indigo_xml_parse(parser->adapter, NULL);
	atomic_store(&parser->stopped, true);
	return NULL;
}

static double example_elapsed(struct timespec start) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec - start.tv_sec + (now.tv_nsec - start.tv_nsec) / 1e9;
}

static int example_run_process(indigo_client *client, const char *executable, atomic_bool *finished, atomic_bool *failed) {
	/* A driver exit must be reported through the parser, rather than SIGPIPE. */
	struct sigaction ignore_pipe = { 0 }, previous_pipe;
	ignore_pipe.sa_handler = SIG_IGN;
	sigemptyset(&ignore_pipe.sa_mask);
	if (sigaction(SIGPIPE, &ignore_pipe, &previous_pipe) < 0) {
		indigo_error("Can't ignore SIGPIPE (%s)", strerror(errno));
		return EXIT_FAILURE;
	}
	int input[2], output[2];
	if (pipe(input) < 0) {
		indigo_error("Can't create local pipe (%s)", strerror(errno));
		return EXIT_FAILURE;
	}
	if (pipe(output) < 0) {
		indigo_error("Can't create local pipe (%s)", strerror(errno));
		close(input[0]);
		close(input[1]);
		return EXIT_FAILURE;
	}
	pid_t child = fork();
	if (child < 0) {
		indigo_error("Can't create driver process (%s)", strerror(errno));
		close(input[0]);
		close(input[1]);
		close(output[0]);
		close(output[1]);
		return EXIT_FAILURE;
	}
	if (child == 0) {
		if (dup2(output[0], STDIN_FILENO) < 0 || dup2(input[1], STDOUT_FILENO) < 0) {
			perror("Can't redirect driver pipes");
			_exit(127);
		}
		int pipes[] = { input[0], input[1], output[0], output[1] };
		for (unsigned i = 0; i < sizeof(pipes) / sizeof(pipes[0]); i++) {
			if (pipes[i] > STDOUT_FILENO) {
				close(pipes[i]);
			}
		}
		sigaction(SIGPIPE, &previous_pipe, NULL);
		execl(executable, "indigo_ccd_simulator", (char *)NULL);
		perror("Can't execute driver");
		_exit(127);
	}
	close(input[1]);
	close(output[0]);
	indigo_start();
	indigo_uni_handle *in = indigo_uni_create_file_handle(input[0], INDIGO_LOG_TRACE);
	indigo_uni_handle *out = indigo_uni_create_file_handle(output[1], INDIGO_LOG_TRACE);
	indigo_device *adapter = indigo_xml_client_adapter("indigo_ccd_simulator", "", in, out);
	indigo_attach_device(adapter);
	indigo_attach_client(client);
	example_parser parser = { .adapter = adapter, .stopped = ATOMIC_VAR_INIT(false) };
	pthread_t parser_thread;
	int thread_error = pthread_create(&parser_thread, NULL, example_parse, &parser);
	if (thread_error != 0) {
		indigo_error("Can't start XML parser (%s)", strerror(thread_error));
	}
	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);
	while (thread_error == 0 && !atomic_load(finished) && !atomic_load(&parser.stopped) && example_elapsed(start) < 120) {
		indigo_usleep(100000);
	}
	bool completed = thread_error == 0 && atomic_load(finished);
	if (!completed) {
		indigo_error("Driver operation failed or timed out");
	}
	/* Only normal shutdown owns the child; callbacks merely report completion. */
	int status = 0;
	pid_t waited;
	do {
		waited = waitpid(child, &status, WNOHANG);
	} while (waited < 0 && errno == EINTR);
	if (waited == 0) {
		kill(child, SIGTERM);
		clock_gettime(CLOCK_MONOTONIC, &start);
		do {
			indigo_usleep(100000);
			waited = waitpid(child, &status, WNOHANG);
		} while ((waited == 0 || (waited < 0 && errno == EINTR)) && example_elapsed(start) < 2);
		if (waited == 0 || (waited < 0 && errno == EINTR)) {
			completed = false;
			indigo_error("Driver ignored SIGTERM; forcing shutdown");
			kill(child, SIGKILL);
			do {
				waited = waitpid(child, &status, 0);
			} while (waited < 0 && errno == EINTR);
		}
	}
	if (thread_error == 0) {
		/* Reaping the driver closes its output pipe and wakes the parser at EOF. */
		pthread_join(parser_thread, NULL);
	}
	indigo_detach_client(client);
	indigo_detach_device(adapter);
	indigo_release_xml_client_adapter(adapter);
	indigo_uni_close(&in);
	indigo_uni_close(&out);
	indigo_stop();
	sigaction(SIGPIPE, &previous_pipe, NULL);
	if (waited < 0 || (WIFEXITED(status) && WEXITSTATUS(status) != 0) || (WIFSIGNALED(status) && WTERMSIG(status) != SIGTERM)) {
		indigo_error("Driver process failed");
		completed = false;
	}
	return completed && !atomic_load(failed) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif
