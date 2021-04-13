/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <jwt.h>
#include <getopt.h>
#include <libgen.h>

#if defined(_MSC_VER) && _MSC_VER + 0 <= 1800
/* Substitution is OK while return value is not used */
#define snprintf _snprintf
#endif

#define PORT            8888
#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   512

#define GET             0
#define POST            1

const char *askpage =
  "<html><body>\
                       What's your name, Sir?<br>\
                       <form action=\"/namepost\" method=\"post\">\
                       <input name=\"name\" type=\"text\">\
                       <input type=\"submit\" value=\" Send \"></form>\
                       </body></html>";

const char *greetingpage =
  "<html><body><h1>Welcome, %s!</center></h1></body></html>";

const char *errorpage =
  "<html><body>This doesn't seem to be right.</body></html>";

struct key_struct {
  char *key;
  size_t key_size;
};

struct certificate_struct {
  struct key_struct *key;
  jwt_valid_t *jwt_valid;
};

struct connection_cookie {
  char *token;
};



static enum MHD_Result
send_page (struct MHD_Connection *connection, const char *page, int err_code)
{
  enum MHD_Result ret;
  struct MHD_Response *response;

  printf("Return code: %d\n", err_code);
  response =
    MHD_create_response_from_buffer (strlen (page), (void *) page,
                                     MHD_RESPMEM_PERSISTENT);
  if (! response)
    return MHD_NO;

  ret = MHD_queue_response (connection, err_code, response);
  MHD_destroy_response (response);

  return ret;
}

static enum MHD_Result
get_cookie (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value)
{
  (void) cls;    /* Unused. Silent compiler warning. */
  (void) kind;   /* Unused. Silent compiler warning. */
  struct connection_cookie *cookie = (struct connection_cookie *)cls;
  if(0 == strcmp("token", key))
  {
    cookie->token = strdup(value);
  }
  // printf ("%s: %s\n", key, value);
  return MHD_YES;
}


// static void
// request_completed (void *cls, struct MHD_Connection *connection,
//                    void **con_cls, enum MHD_RequestTerminationCode toe)
// {
//   struct connection_info_struct *con_info = *con_cls;
//   (void) cls;         /* Unused. Silent compiler warning. */
//   (void) connection;  /* Unused. Silent compiler warning. */
//   (void) toe;         /* Unused. Silent compiler warning. */

//   if (NULL == con_info)
//     return;

//   if (con_info->connectiontype == POST)
//   {
//     MHD_destroy_post_processor (con_info->postprocessor);
//     if (con_info->answerstring)
//       free (con_info->answerstring);
//   }

//   free (con_info);
//   *con_cls = NULL;
// }


static enum MHD_Result
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  (void) url;               /* Unused. Silent compiler warning. */
  (void) version;           /* Unused. Silent compiler warning. */
  struct connection_cookie *cookie;
  struct key_struct *pub_key;
  struct certificate_struct *cer;
  jwt_valid_t *jwt_valid;
	jwt_t *jwt = NULL;
  int ret = 0;
  FILE *fp_pub_key;

  cer = (struct certificate_struct *)cls;
  pub_key = cer->key;
  jwt_valid = cer->jwt_valid;
  cookie = malloc (sizeof (struct connection_cookie));
  
  MHD_get_connection_values (connection, MHD_COOKIE_KIND, get_cookie,
                             cookie);

  if(NULL == cookie->token)
  {
    printf("No token\n");
    return send_page (connection, errorpage, MHD_HTTP_UNAUTHORIZED);
  }

  ret = jwt_decode(&jwt, cookie->token, pub_key->key, pub_key->key_size);
	if (ret != 0 || jwt == NULL) {
		// fprintf(stderr, "invalid jwt %s\n", cookie->token);
		goto return_fail;
	}

	/* Validate jwt */
	if (jwt_validate(jwt, jwt_valid) != 0) {
		// fprintf(stderr, "jwt failed to validate: %08x\n", jwt_valid_get_status(jwt_valid));
		// jwt_dump_fp(jwt, stderr, 1);
		goto return_fail;
	}

  if (0 == strcmp (method, "POST"))
  {
    return send_page (connection, greetingpage, MHD_HTTP_OK);
  }
  if (0 == strcmp (method, "GET"))
  {

    return send_page (connection, greetingpage, MHD_HTTP_BAD_REQUEST);
  }

return_fail:
  return send_page (connection, errorpage, MHD_HTTP_UNAUTHORIZED);
}

void usage(const char *name)
{
	/* TODO Might want to support JWT input via stdin */
	printf("%s --key some-pub.pem --port some-port\n", name);
	printf("Options:\n"
			"  -k --key KEY  The key to use for verification\n"
			"  -p --port PORT  The port for server. Default 8888 \n"
  );
	exit(0);
}

int
main(int argc, char *argv[])
{
  struct MHD_Daemon *daemon;
  char *opt_key_name = NULL;
  char *optstr = "hk:p:c:";
  int port = PORT;
  int oc = 0, i;
  FILE *fp_pub_key;
  jwt_alg_t opt_alg = JWT_ALG_RS256;
  jwt_valid_t *jwt_valid;
  int ret, claims_count = 0;
  struct key_struct *pub_key;
	struct option opttbl[] = {
		{ "help",         no_argument,        NULL, 'h'         },
		{ "key",          required_argument,  NULL, 'k'         },
		{ "port",          optional_argument,  NULL, 'p'         },
    { "claim",        required_argument,  NULL, 'c'         },
		{ NULL, 0, 0, 0 },
	};

  char *k = NULL, *v = NULL;
	struct kv {
		char *key;
		char *val;
	} opt_claims[100];
	memset(opt_claims, 0, sizeof(opt_claims));

  while ((oc = getopt_long(argc, argv, optstr, opttbl, NULL)) != -1) {
		switch (oc) {
		case 'k':
      opt_key_name = strdup(optarg);
			break;

		case 'p':
			port = atoi(optarg);
			break;

		case 'h':
			usage(basename(argv[0]));
			return 0;
    case 'c':
			k = strtok(optarg, "=");
			if (k) {
				v = strtok(NULL, "=");
				if (v) {
					opt_claims[claims_count].key = strdup(k);
					opt_claims[claims_count].val = strdup(v);
					claims_count++;
				}
			}
			break;
		default: /* '?' */
			usage(basename(argv[0]));
			exit(EXIT_FAILURE);
		}
	}

  ret = jwt_valid_new(&jwt_valid, opt_alg);

	if (ret != 0 || jwt_valid == NULL) {
		fprintf(stderr, "failed to allocate jwt_valid\n");
		exit(EXIT_FAILURE);
	}

	jwt_valid_set_headers(jwt_valid, 1);
	jwt_valid_set_now(jwt_valid, time(NULL));
  for (i = 0; i < claims_count; i++) {
    printf("Add grant  %s %s\n", opt_claims[i].key, opt_claims[i].val);
		jwt_valid_add_grant(jwt_valid, opt_claims[i].key, opt_claims[i].val);
	}

  if (NULL == opt_key_name) {
		fprintf(stderr, "Please provide public key file\n");
		exit(EXIT_FAILURE);
  }
  
  pub_key = malloc(sizeof(struct key_struct));
  if (NULL == pub_key) {
    fprintf(stderr, "Can not allocate memory\n");
		exit(EXIT_FAILURE);
  }

  /* Load pub key */
	fp_pub_key = fopen(opt_key_name, "r");
  fseek(fp_pub_key, 0L, SEEK_END);
  pub_key->key_size = ftell(fp_pub_key);
  fseek(fp_pub_key, 0L, SEEK_SET);
  pub_key->key = malloc(sizeof(unsigned char) * pub_key->key_size);

  
  if (NULL == pub_key->key) {
    fprintf(stderr, "Can not allocate memory\n");
		exit(EXIT_FAILURE);
  }

	pub_key->key_size = fread(pub_key->key, 1, pub_key->key_size, fp_pub_key);
	fclose(fp_pub_key);
	fprintf(stderr, "pub key loaded %s (%zu)!\n", opt_key_name, pub_key->key_size);

  struct certificate_struct *certificate = malloc(sizeof(struct certificate_struct));
  if (NULL == certificate) {
    fprintf(stderr, "Can not allocate memory\n");
		exit(EXIT_FAILURE);
  }
  certificate->key = pub_key;
  certificate->jwt_valid = jwt_valid;
  daemon = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD,
                             port, NULL, NULL,
                             &answer_to_connection, certificate,
                             // Notify when request is 
                            //  MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                             NULL, MHD_OPTION_END);
  if (NULL == daemon)
    return 1;

  (void) getchar ();

  MHD_stop_daemon (daemon);

  return 0;
}
