/*
 * utils_http.c
 *
 *  Created on: Jun 12, 2018
 *      Author: jnevens
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <curl/curl.h>


typedef struct {
	char **response;
	size_t len;
} equest_download_arg;

static size_t http_request_download_cb(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	// printf("data received: %zuB\n", size * nmemb);
	equest_download_arg *arg = (equest_download_arg *)stream;
	size_t new_len = arg->len + size * nmemb;
	*arg->response = realloc(*arg->response, new_len + 1);
	memcpy(&((*arg->response)[arg->len]), ptr, size * nmemb);
	(*arg->response)[new_len] = '\0';
	arg->len = new_len;
	return size * nmemb;
}

bool http_request(const char *url, char **response)
{
	CURL *curl;
	CURLcode res;
	bool rv = false;
	equest_download_arg download_arg = {
			.response = response,
			.len = 0,
	};

	curl = curl_easy_init();
	if (!curl) {
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	// curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=val&foo=bar");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &download_arg);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_request_download_cb);
	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		goto cleanup;
	}

	rv = true;
cleanup:
	curl_easy_cleanup(curl);
	return rv;
}


