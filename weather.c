#define _GNU_SOURCE
#include <curl/curl.h>
#include <string.h> // strstr()
#include <stdlib.h> // memcpy() realloc()
#include <stdio.h>	// asprintf() printf()

#define CHUNK_SIZE 2048

typedef struct get_request
{
	unsigned char *buffer;
	size_t len;
	size_t buflen;
} get_request;

typedef struct weather
{
	int temperature;
	int humidity;
} Weather;

/* read the user's api key from plaintext 
ref https://cplusplus.com/reference/cstdio/fread/ */
int get_api_key(char k[33])
{
	FILE *file;
	file = fopen("key.txt", "r");
	if (file)
	{
		/* determine key size then send pointer back
		to start */
		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		rewind(file);
		/* read key into argument array k */
		size_t result = fread(k, 1, size, file);
		fclose(file);

		// printf("%s\n", k);

		return 0;
	}
	else
	{
		fputs("Key does not yet exist", stderr);
		return 1;
	}
}

/* write api key to plaintext. Called
when flag -k is passed */
int write_api_key(char *key)
{
	FILE *file;
	file = fopen("key.txt", "w");
	if (file)
	{
		fwrite(key, strlen(key), 1, file);
		fclose(file);
	}
}

/* callback function used by curl in get_zip and 
in weather_initialize */
size_t curl_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t realsize = size * nmemb;
	get_request *req = (get_request *)userdata;

	// printf("receive chunk of %zu bytes\n", realsize);

	while (req->buflen < req->len + realsize + 1)
	{
		req->buffer = realloc(req->buffer, req->buflen + CHUNK_SIZE);
		req->buflen += CHUNK_SIZE;
	}
	memcpy(&req->buffer[req->len], ptr, realsize);
	req->len += realsize;
	req->buffer[req->len] = 0;

	return realsize;
}

/* poll ipinfo.io for local area of computer */
void get_zip(char digits[6])
{
	char *url = "ipinfo.io";
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();

	get_request req = {.buffer = NULL, .len = 0, .buflen = 0};

	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "length: 20");
		headers = curl_slist_append(headers, "numbers: true");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		req.buffer = malloc(CHUNK_SIZE);
		req.buflen = CHUNK_SIZE;

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&req);

		res = curl_easy_perform(curl);
		/*printf("Result = %u\n", res);
		printf("Total received bytes: %zu\n", req.len);
		printf("Received data:/n%s\n", req.buffer);*/

		char *target = "postal";
		char *index = strstr(req.buffer, target) + 10;
		for (int i = 0; i < 5; i++)
			digits[i] = *index++;

		// printf("%s\n", digits);
	}
	curl_easy_cleanup(curl);
}

/* poll openweathermap.org for current weather */
void weather_initialize(Weather *w)
{
	char key[33] = {'\0'};
	get_api_key(key);

	char digits[6] = {'\0'};
	get_zip(digits);

	char *url;
	if (asprintf(&url,
				 "https://api.openweathermap.org/data/2.5/weather?zip=%s,us&APPID=%s",
				 digits,
				 key) < 0)
		fputs("Url string not formatted", stderr);

	// printf("%s\n", url);

	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();

	get_request req = {.buffer = NULL, .len = 0, .buflen = 0};

	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "length: 20");
		headers = curl_slist_append(headers, "numbers: true");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		req.buffer = malloc(CHUNK_SIZE);
		req.buflen = CHUNK_SIZE;

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&req);

		res = curl_easy_perform(curl);
		/*printf("Result = %u\n", res);
		printf("Total received bytes: %zu\n", req.len);
		printf("Received data:/n%s\n", req.buffer);*/

		char *t_target = "feels_like";
		char *h_target = "humidity";

		char *t_index = strstr(req.buffer, t_target) + 12;
		char temp[4] = {'\0'};
		for (int i = 0; i < 3; i++)
			temp[i] = *t_index++;

		char *h_index = strstr(req.buffer, h_target) + 10;
		char humi[3] = {'\0'};
		for (int i = 0; i < 2; i++)
			humi[i] = *h_index++;

		w->temperature = (int)((atoi(temp) - 273.15) * 1.8000 + 32.00);
		w->humidity = atoi(humi);
		// printf("%s %s\n", temp, humi);

		free(req.buffer);
	}

	curl_easy_cleanup(curl);
	free(url);
}

/*
int main() {
	write_api_key("");
        Weather w;
        weather_initialize(&w);
return 0;	
}
*/
