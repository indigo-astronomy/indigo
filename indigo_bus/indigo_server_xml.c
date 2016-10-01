//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  0.0 PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "indigo_server_xml.h"

static int server_socket;
static struct sockaddr_in server_address;
static bool shutdown_initiated = false;
static int client_count = 0;
static indigo_server_xml_callback server_callback;

int indigo_server_xml_port = 7624;

static void start_worker_thread(indigo_client *protocol_adapter) {
  indigo_log("INDI Go server: worker thread started", indigo_server_xml_port);
  server_callback(++client_count);
  assert(protocol_adapter != NULL);
  indigo_xml_device_adapter_context *device_context = protocol_adapter->client_context;
  indigo_attach_client(protocol_adapter);
  indigo_xml_parse(device_context->input, NULL, protocol_adapter);
  indigo_detach_client(protocol_adapter);
  server_callback(--client_count);
  indigo_log("INDI Go server: worker thread finished", indigo_server_xml_port);
}

static void server_shutdown() {
  shutdown_initiated = true;
  close(server_socket);
}

indigo_result indigo_server_xml(indigo_server_xml_callback callback) {
  server_callback = callback;
  int client_socket;
  int reuse = 1;
  struct sockaddr_in client_name;
  unsigned int name_len = sizeof(client_name);
  server_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (server_socket == -1)
    indigo_error("INDI Go server: can't open server socket (%s)", strerror(errno));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(indigo_server_xml_port);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  if (setsockopt(server_socket, SOL_SOCKET,SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    indigo_error("INDI Go server: can't setsockopt for server socket (%s)", strerror(errno));
    return INDIGO_CANT_START_SERVER;
  }
  if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    indigo_error("INDI Go server: Can't bind server socket (%s)", strerror(errno));
    return INDIGO_CANT_START_SERVER;
  }
  if (listen(server_socket, 5) < 0) {
    indigo_error("INDI Go server: can't listen on server socket (%s)", strerror(errno));
    return INDIGO_CANT_START_SERVER;
  }
  indigo_log("INDI Go server: server started on %d", indigo_server_xml_port);
  atexit(server_shutdown);
  callback(client_count);
  while (1) {
    client_socket = accept(server_socket, (struct sockaddr *)&client_name, &name_len);
    if (client_socket == -1) {
      if (shutdown_initiated)
        break;
      indigo_error("INDI Go server: can't accept connection (%s)", strerror(errno));
    } else {
      indigo_client *protocol_adapter = xml_device_adapter(client_socket, client_socket);;
      pthread_t thread;
      if (pthread_create(&thread , NULL, (void *(*)(void *))&start_worker_thread, protocol_adapter) != 0)
        indigo_error("INDI Go server: can't create worker thread for connection (%s)", strerror(errno));
    }
  }
  return shutdown_initiated ? INDIGO_OK : INDIGO_FAILED;
}
