/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * simple HTTP POST using the easy interface
 * </DESC>
 */
#include <stdio.h>
#include <curl/curl.h>
#include <getopt.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>

const char *COOKIE ="Cookie: token=";
char *opt_host = "localhost";
int opt_port = 8888;

struct token_struct {
  char *token;
  size_t token_len;
};

struct thread_info {
  struct token_struct *token;
  int thread_id;
};

void usage(const char *name) {
	/* TODO Might want to support JWT input via stdin */
	printf("%s -t some-tokens\n", name);
	printf("Options:\n"
			"  -t --token token_file  Token file\n");
	exit(0);
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
  return size * nmemb;
}

char *cookie_generate(struct token_struct *token) {
  size_t cookie_len = (strlen(COOKIE) + token->token_len);
  char *cookie = malloc(sizeof(char) * cookie_len);
  if (NULL == cookie) {
    fprintf(stderr, "Can not allocate memory\n");
		exit(EXIT_FAILURE);
  }
  sprintf (cookie, "%s%s", COOKIE, token->token);
  // printf("%s\n", cookie);
  return cookie;
}

int load_token_file(struct token_struct *token, char *file_name) {
  FILE *fp_token_file;
  // printf("Filename %s\n", file_name);
  /* Load pub key */
	fp_token_file = fopen(file_name, "r");
  if (NULL == fp_token_file) {
    fprintf(stderr, "File does not exist %s\n", file_name);
		exit(EXIT_FAILURE);
  }
  fseek(fp_token_file, 0L, SEEK_END);
  token->token_len = ftell(fp_token_file);
  fseek(fp_token_file, 0L, SEEK_SET);
  token->token = malloc(sizeof(unsigned char) * token->token_len);

  if (NULL == token->token) {
    fprintf(stderr, "Can not allocate memory\n");
		exit(EXIT_FAILURE);
  }

	token->token_len = fread(token->token, 1, token->token_len, fp_token_file);
	fclose(fp_token_file);
	fprintf(stderr, "token file loaded %s (%zu)!\n", file_name, token->token_len);
}

void *curl_post_request( void *ptr ) {
  CURL *curl;
  CURLcode res;
  int response_code;
  int thread_id;
  struct thread_info *thread_info = (struct thread_info *)ptr;
  struct token_struct *opt_tokens = thread_info->token;
  thread_id = thread_info->thread_id;
  /* get a curl handle */
  while(1) {
    curl = curl_easy_init();
    if(curl) {
      struct curl_slist *chunk = NULL;
      chunk = curl_slist_append(chunk, cookie_generate(opt_tokens));
      // chunk = curl_slist_append(chunk, "Cookie: token=eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2MTc4MDQ2ODAsImlzcyI6ImV4YW1wbGUuY29tIiwic3ViIjoidXNlcjAifQ.U297hsZ9Xmz2LDtmLuqaysx-MbOLip9TOojgjteiPbpQT3eRLQmE55IvYaRqtVXFbrOkUkhnn_ysAV_ltmW9TSFIKddtIP32yJUXzm7JtY4kL_On-9W0YepTkCxbiemrNIUHhLFY08UCXyw1zUyvsyIcAWgbrBE5kp4rACP5FZHS2fecPUUtLmJRWehlaQHqdaMC9FKcn2QOxpaWS4AhmWUBdA17l3GuSYC1glsVxF0Jx_WZ6Ot-K8vmnvpBVxaRXLTqKAK_roveC51Pcs5xz_BBcVuNTKWEt6OXuFCYWzyuY6BlmCO7QkOAeTSCLthypmF0PdEnBtODomTWkBLMxQ");
      /* First set the URL that is about to receive our POST. This URL can
        just as well be a https:// URL if that is what should receive the
        data. */
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      curl_easy_setopt(curl, CURLOPT_URL, opt_host);
      curl_easy_setopt(curl, CURLOPT_PORT, opt_port);
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      /* Now specify the POST data */
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

      /* Perform the request, res will get the return code */
      res = curl_easy_perform(curl);
      /* Check for errors */
      if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
      
      res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      if(res == CURLE_OK) {
          /* a redirect implies a 3xx response code */ 
          if (401 == response_code) {
            printf("[Thread %d] - Server rejected. HTTP code: 401.\n", thread_info->thread_id);
          } else if (200 == response_code) {
            printf("[Thread %d] - Server accepted. HTTP code: 200.\n", thread_info->thread_id);
          }
        }
      /* always cleanup */
      curl_easy_cleanup(curl);
      /* free the custom headers */
      curl_slist_free_all(chunk);
    }
    sleep(2);
  }
}

int main(int argc, char *argv[]) {

  int token_count = 0;
  int oc = 0;
  
  
  int response_code;

  int opt_thread_count = 5;
  pthread_t *threads;
  int thread_ret;
  struct thread_info *thread_infos;
  char *optstr = "hc:p:H:t:";
	struct option opttbl[] = {
		{ "help",         no_argument,        NULL, 'h'         },
    { "host",         no_argument,        NULL, 'H'         },
    { "port",         no_argument,        NULL, 'p'         },
    { "number",         no_argument,        NULL, 'n'         },
		{ "token",        required_argument,  NULL, 't'         },
		{ NULL, 0, 0, 0 },
	};

  struct token_struct opt_tokens[100];
	memset(opt_tokens, 0, sizeof(opt_tokens));

  while ((oc = getopt_long(argc, argv, optstr, opttbl, NULL)) != -1) {
		switch (oc) {
    case 'H':
      opt_host = strdup(optarg);
      break;
		case 't':
      load_token_file(&opt_tokens[token_count++], optarg);
			break;
    case 'p':
      opt_port = atoi(optarg);
      break;
    case 'n':
      opt_thread_count = atoi(optarg);
      break;
		case 'h':
			usage(basename(argv[0]));
			return 0;

		default: /* '?' */
			usage(basename(argv[0]));
			exit(EXIT_FAILURE);
		}
	}

  threads = malloc(sizeof(pthread_t ) * opt_thread_count);
  if (NULL == threads) {
    fprintf(stderr, "Can not allocate memory\n");
		exit(EXIT_FAILURE);
  }

  thread_infos = malloc(sizeof(struct thread_info) * opt_thread_count);
  if (NULL == thread_infos) {
    fprintf(stderr, "Can not allocate memory\n");
		exit(EXIT_FAILURE);
  }

  /* In windows, this will init the winsock stuff */
  curl_global_init(CURL_GLOBAL_ALL);
  
  // Create threads

  for(int i = 0; i < opt_thread_count; i++) {
    thread_infos[i].token = &opt_tokens[i % token_count];
    thread_infos[i].thread_id = i;
    thread_ret = pthread_create( &threads[i], NULL, curl_post_request, (void*) &thread_infos[i]);
  }
  getchar();
  for(int i = 0; i < opt_thread_count; i++) {
    pthread_join(threads[i], NULL );
  }
  curl_global_cleanup();
  return 0;
}
